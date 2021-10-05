#include "psu_manager.hpp"

#include "utility.hpp"

#include <fmt/format.h>
#include <sys/types.h>
#include <unistd.h>

#include <regex>

using namespace phosphor::logging;

namespace phosphor::power::manager
{

constexpr auto IBMCFFPSInterface =
    "xyz.openbmc_project.Configuration.IBMCFFPSConnector";
constexpr auto i2cBusProp = "I2CBus";
constexpr auto i2cAddressProp = "I2CAddress";
constexpr auto psuNameProp = "Name";
constexpr auto presLineName = "NamedPresenceGpio";

constexpr auto supportedConfIntf =
    "xyz.openbmc_project.Configuration.SupportedConfiguration";

PSUManager::PSUManager(sdbusplus::bus::bus& bus, const sdeventplus::Event& e) :
    bus(bus)
{
    // Subscribe to InterfacesAdded before doing a property read, otherwise
    // the interface could be created after the read attempt but before the
    // match is created.
    entityManagerIfacesAddedMatch = std::make_unique<sdbusplus::bus::match_t>(
        bus,
        sdbusplus::bus::match::rules::interfacesAdded() +
            sdbusplus::bus::match::rules::sender(
                "xyz.openbmc_project.EntityManager"),
        std::bind(&PSUManager::entityManagerIfaceAdded, this,
                  std::placeholders::_1));
    getPSUConfiguration();
    getSystemProperties();

    using namespace sdeventplus;
    auto interval = std::chrono::milliseconds(1000);
    timer = std::make_unique<utility::Timer<ClockId::Monotonic>>(
        e, std::bind(&PSUManager::analyze, this), interval);

    validationTimer = std::make_unique<utility::Timer<ClockId::Monotonic>>(
        e, std::bind(&PSUManager::validateConfig, this));

    // Subscribe to power state changes
    powerService = util::getService(POWER_OBJ_PATH, POWER_IFACE, bus);
    powerOnMatch = std::make_unique<sdbusplus::bus::match_t>(
        bus,
        sdbusplus::bus::match::rules::propertiesChanged(POWER_OBJ_PATH,
                                                        POWER_IFACE),
        [this](auto& msg) { this->powerStateChanged(msg); });

    initialize();
}

void PSUManager::getPSUConfiguration()
{
    using namespace phosphor::power::util;
    auto depth = 0;
    auto objects = getSubTree(bus, "/", IBMCFFPSInterface, depth);

    psus.clear();

    // I should get a map of objects back.
    // Each object will have a path, a service, and an interface.
    // The interface should match the one passed into this function.
    for (const auto& [path, services] : objects)
    {
        auto service = services.begin()->first;

        if (path.empty() || service.empty())
        {
            continue;
        }

        // For each object in the array of objects, I want to get properties
        // from the service, path, and interface.
        auto properties =
            getAllProperties(bus, path, IBMCFFPSInterface, service);

        getPSUProperties(properties);
    }

    if (psus.empty())
    {
        // Interface or properties not found. Let the Interfaces Added callback
        // process the information once the interfaces are added to D-Bus.
        log<level::INFO>(fmt::format("No power supplies to monitor").c_str());
    }
}

void PSUManager::getPSUProperties(util::DbusPropertyMap& properties)
{
    // From passed in properties, I want to get: I2CBus, I2CAddress,
    // and Name. Create a power supply object, using Name to build the inventory
    // path.
    const auto basePSUInvPath =
        "/xyz/openbmc_project/inventory/system/chassis/motherboard/powersupply";
    uint64_t* i2cbus = nullptr;
    uint64_t* i2caddr = nullptr;
    std::string* psuname = nullptr;
    std::string* preslineptr = nullptr;

    for (const auto& property : properties)
    {
        try
        {
            if (property.first == i2cBusProp)
            {
                i2cbus = std::get_if<uint64_t>(&properties[i2cBusProp]);
            }
            else if (property.first == i2cAddressProp)
            {
                i2caddr = std::get_if<uint64_t>(&properties[i2cAddressProp]);
            }
            else if (property.first == psuNameProp)
            {
                psuname = std::get_if<std::string>(&properties[psuNameProp]);
            }
            else if (property.first == presLineName)
            {
                preslineptr =
                    std::get_if<std::string>(&properties[presLineName]);
            }
        }
        catch (std::exception& e)
        {}
    }

    if ((i2cbus) && (i2caddr) && (psuname) && (!psuname->empty()))
    {
        std::string invpath = basePSUInvPath;
        invpath.push_back(psuname->back());
        std::string presline = "";

        log<level::DEBUG>(fmt::format("Inventory Path: {}", invpath).c_str());

        if (nullptr != preslineptr)
        {
            presline = *preslineptr;
        }

        auto invMatch =
            std::find_if(psus.begin(), psus.end(), [&invpath](auto& psu) {
                return psu->getInventoryPath() == invpath;
            });
        if (invMatch != psus.end())
        {
            // This power supply has the same inventory path as the one with
            // information just added to D-Bus.
            // Changes to GPIO line name unlikely, so skip checking.
            // Changes to the I2C bus and address unlikely, as that would
            // require corresponding device tree updates.
            // Return out to avoid duplicate object creation.
            return;
        }

        log<level::DEBUG>(
            fmt::format("make PowerSupply bus: {} addr: {} presline: {}",
                        *i2cbus, *i2caddr, presline)
                .c_str());
        auto psu = std::make_unique<PowerSupply>(bus, invpath, *i2cbus,
                                                 *i2caddr, presline);
        psus.emplace_back(std::move(psu));
    }

    if (psus.empty())
    {
        log<level::INFO>(fmt::format("No power supplies to monitor").c_str());
    }
}

void PSUManager::populateSysProperties(const util::DbusPropertyMap& properties)
{
    try
    {
        auto propIt = properties.find("SupportedType");
        if (propIt == properties.end())
        {
            return;
        }
        const std::string* type = std::get_if<std::string>(&(propIt->second));
        if ((type == nullptr) || (*type != "PowerSupply"))
        {
            return;
        }

        propIt = properties.find("SupportedModel");
        if (propIt == properties.end())
        {
            return;
        }
        const std::string* model = std::get_if<std::string>(&(propIt->second));
        if (model == nullptr)
        {
            return;
        }

        sys_properties sys;
        propIt = properties.find("RedundantCount");
        if (propIt != properties.end())
        {
            const uint64_t* count = std::get_if<uint64_t>(&(propIt->second));
            if (count != nullptr)
            {
                sys.powerSupplyCount = *count;
            }
        }
        propIt = properties.find("InputVoltage");
        if (propIt != properties.end())
        {
            const std::vector<uint64_t>* voltage =
                std::get_if<std::vector<uint64_t>>(&(propIt->second));
            if (voltage != nullptr)
            {
                sys.inputVoltage = *voltage;
            }
        }

        supportedConfigs.emplace(*model, sys);
    }
    catch (std::exception& e)
    {}
}

void PSUManager::getSystemProperties()
{

    try
    {
        util::DbusSubtree subtree =
            util::getSubTree(bus, INVENTORY_OBJ_PATH, supportedConfIntf, 0);
        if (subtree.empty())
        {
            throw std::runtime_error("Supported Configuration Not Found");
        }

        for (const auto& [objPath, services] : subtree)
        {
            std::string service = services.begin()->first;
            if (objPath.empty() || service.empty())
            {
                continue;
            }
            auto properties = util::getAllProperties(
                bus, objPath, supportedConfIntf, service);
            populateSysProperties(properties);
        }
    }
    catch (std::exception& e)
    {
        // Interface or property not found. Let the Interfaces Added callback
        // process the information once the interfaces are added to D-Bus.
    }
}

void PSUManager::entityManagerIfaceAdded(sdbusplus::message::message& msg)
{
    try
    {
        sdbusplus::message::object_path objPath;
        std::map<std::string, std::map<std::string, util::DbusVariant>>
            interfaces;
        msg.read(objPath, interfaces);

        auto itIntf = interfaces.find(supportedConfIntf);
        if (itIntf != interfaces.cend())
        {
            populateSysProperties(itIntf->second);
        }

        itIntf = interfaces.find(IBMCFFPSInterface);
        if (itIntf != interfaces.cend())
        {
            log<level::INFO>(
                fmt::format("InterfacesAdded for: {}", IBMCFFPSInterface)
                    .c_str());
            getPSUProperties(itIntf->second);
        }

        // Call to validate the psu configuration if the power is on and both
        // the IBMCFFPSConnector and SupportedConfiguration interfaces have been
        // processed
        if (powerOn && !psus.empty() && !supportedConfigs.empty())
        {
            validationTimer->restartOnce(validationTimeout);
        }
    }
    catch (std::exception& e)
    {
        // Ignore, the property may be of a different type than expected.
    }
}

void PSUManager::powerStateChanged(sdbusplus::message::message& msg)
{
    int32_t state = 0;
    std::string msgSensor;
    std::map<std::string, std::variant<int32_t>> msgData;
    msg.read(msgSensor, msgData);

    // Check if it was the Present property that changed.
    auto valPropMap = msgData.find("state");
    if (valPropMap != msgData.end())
    {
        state = std::get<int32_t>(valPropMap->second);

        // Power is on when state=1. Clear faults.
        if (state)
        {
            powerOn = true;
            validationTimer->restartOnce(validationTimeout);
            clearFaults();
        }
        else
        {
            powerOn = false;
            runValidateConfig = true;
        }
    }
}

void PSUManager::createError(
    const std::string& faultName,
    const std::map<std::string, std::string>& additionalData)
{
    using namespace sdbusplus::xyz::openbmc_project;
    constexpr auto loggingObjectPath = "/xyz/openbmc_project/logging";
    constexpr auto loggingCreateInterface =
        "xyz.openbmc_project.Logging.Create";

    try
    {
        auto service =
            util::getService(loggingObjectPath, loggingCreateInterface, bus);

        if (service.empty())
        {
            log<level::ERR>("Unable to get logging manager service");
            return;
        }

        auto method = bus.new_method_call(service.c_str(), loggingObjectPath,
                                          loggingCreateInterface, "Create");

        auto level = Logging::server::Entry::Level::Error;
        method.append(faultName, level, additionalData);

        auto reply = bus.call(method);
    }
    catch (std::exception& e)
    {
        log<level::ERR>(
            fmt::format(
                "Failed creating event log for fault {} due to error {}",
                faultName, e.what())
                .c_str());
    }
}

void PSUManager::analyze()
{
    for (auto& psu : psus)
    {
        psu->analyze();
    }

    if (powerOn)
    {
        std::map<std::string, std::string> additionalData;
        auto requiredPSUsPresent = hasRequiredPSUs(additionalData);

        for (auto& psu : psus)
        {
            additionalData["_PID"] = std::to_string(getpid());
            // TODO: Fault priorities #918
            if (!psu->isFaultLogged() && !psu->isPresent())
            {
                if (!requiredPSUsPresent)
                {
                    // Create error for power supply missing.
                    additionalData["CALLOUT_INVENTORY_PATH"] =
                        psu->getInventoryPath();
                    additionalData["CALLOUT_PRIORITY"] = "H";
                    createError(
                        "xyz.openbmc_project.Power.PowerSupply.Error.Missing",
                        additionalData);
                }
                psu->setFaultLogged();
            }
            else if (!psu->isFaultLogged() && psu->isFaulted())
            {
                additionalData["STATUS_WORD"] =
                    std::to_string(psu->getStatusWord());
                additionalData["STATUS_MFR"] =
                    std::to_string(psu->getMFRFault());
                // If there are faults being reported, they possibly could be
                // related to a bug in the firmware version running on the power
                // supply. Capture that data into the error as well.
                additionalData["FW_VERSION"] = psu->getFWVersion();

                if ((psu->hasInputFault() || psu->hasVINUVFault()))
                {
                    /* The power supply location might be needed if the input
                     * fault is due to a problem with the power supply itself.
                     * Include the inventory path with a call out priority of
                     * low.
                     */
                    additionalData["CALLOUT_INVENTORY_PATH"] =
                        psu->getInventoryPath();
                    additionalData["CALLOUT_PRIORITY"] = "L";
                    createError("xyz.openbmc_project.Power.PowerSupply.Error."
                                "InputFault",
                                additionalData);
                    psu->setFaultLogged();
                }
                else if (psu->hasMFRFault())
                {
                    /* This can represent a variety of faults that result in
                     * calling out the power supply for replacement: Output
                     * OverCurrent, Output Under Voltage, and potentially other
                     * faults.
                     *
                     * Also plan on putting specific fault in AdditionalData,
                     * along with register names and register values
                     * (STATUS_WORD, STATUS_MFR, etc.).*/

                    additionalData["CALLOUT_INVENTORY_PATH"] =
                        psu->getInventoryPath();

                    createError(
                        "xyz.openbmc_project.Power.PowerSupply.Error.Fault",
                        additionalData);

                    psu->setFaultLogged();
                }
                else if (psu->hasCommFault())
                {
                    /* Attempts to communicate with the power supply have
                     * reached there limit. Create an error. */
                    additionalData["CALLOUT_DEVICE_PATH"] =
                        psu->getDevicePath();

                    createError(
                        "xyz.openbmc_project.Power.PowerSupply.Error.CommFault",
                        additionalData);

                    psu->setFaultLogged();
                }
            }
        }
    }
}

void PSUManager::validateConfig()
{
    if (!runValidateConfig || supportedConfigs.empty())
    {
        return;
    }

    std::map<std::string, std::string> additionalData;
    auto supported = hasRequiredPSUs(additionalData);
    if (supported)
    {
        runValidateConfig = false;
        return;
    }

    // Validation failed, create an error log.
    // Return without setting the runValidateConfig flag to false because
    // it may be that an additional supported configuration interface is
    // added and we need to validate it to see if it matches this system.
    createError("xyz.openbmc_project.Power.PowerSupply.Error.NotSupported",
                additionalData);
}

bool PSUManager::hasRequiredPSUs(
    std::map<std::string, std::string>& additionalData)
{
    // Check that all PSUs have the same model name. Initialize the model
    // variable with the first PSU name found, then use it as a base to compare
    // against the rest of the PSUs.
    std::string model{};
    for (const auto& psu : psus)
    {
        auto psuModel = psu->getModelName();
        if (psuModel.empty())
        {
            continue;
        }
        if (model.empty())
        {
            model = psuModel;
            continue;
        }
        if (psuModel != model)
        {
            additionalData["EXPECTED_MODEL"] = model;
            additionalData["ACTUAL_MODEL"] = psuModel;
            additionalData["CALLOUT_INVENTORY_PATH"] = psu->getInventoryPath();
            return false;
        }
    }

    auto presentCount =
        std::count_if(psus.begin(), psus.end(),
                      [](const auto& psu) { return psu->isPresent(); });

    // Validate the supported configurations. A system may support more than one
    // power supply model configuration. Since all configurations need to be
    // checked, the additional data would contain only the information of the
    // last configuration that did not match.
    std::map<std::string, std::string> tmpAdditionalData;
    for (const auto& config : supportedConfigs)
    {
        if (config.first != model)
        {
            continue;
        }
        if (presentCount != config.second.powerSupplyCount)
        {
            tmpAdditionalData.clear();
            tmpAdditionalData["EXPECTED_COUNT"] =
                std::to_string(config.second.powerSupplyCount);
            tmpAdditionalData["ACTUAL_COUNT"] = std::to_string(presentCount);
            continue;
        }

        bool voltageValidated = true;
        for (const auto& psu : psus)
        {
            if (!psu->isPresent())
            {
                // Only present PSUs report a valid input voltage
                continue;
            }

            double actualInputVoltage;
            int inputVoltage;
            psu->getInputVoltage(actualInputVoltage, inputVoltage);

            if (std::find(config.second.inputVoltage.begin(),
                          config.second.inputVoltage.end(),
                          inputVoltage) == config.second.inputVoltage.end())
            {
                tmpAdditionalData.clear();
                tmpAdditionalData["ACTUAL_VOLTAGE"] =
                    std::to_string(actualInputVoltage);
                for (const auto& voltage : config.second.inputVoltage)
                {
                    tmpAdditionalData["EXPECTED_VOLTAGE"] +=
                        std::to_string(voltage) + " ";
                }
                tmpAdditionalData["CALLOUT_INVENTORY_PATH"] =
                    psu->getInventoryPath();

                voltageValidated = false;
                break;
            }
        }
        if (!voltageValidated)
        {
            continue;
        }

        return true;
    }

    additionalData.insert(tmpAdditionalData.begin(), tmpAdditionalData.end());
    return false;
}

} // namespace phosphor::power::manager
