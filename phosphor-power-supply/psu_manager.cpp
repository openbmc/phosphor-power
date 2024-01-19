#include "config.h"

#include "psu_manager.hpp"

#include "utility.hpp"

#include <fmt/format.h>
#include <sys/types.h>
#include <unistd.h>

#include <xyz/openbmc_project/State/Chassis/server.hpp>

#include <algorithm>
#include <regex>
#include <set>

using namespace phosphor::logging;

namespace phosphor::power::manager
{
constexpr auto managerBusName = "xyz.openbmc_project.Power.PSUMonitor";
constexpr auto objectManagerObjPath =
    "/xyz/openbmc_project/power/power_supplies";
constexpr auto powerSystemsInputsObjPath =
    "/xyz/openbmc_project/power/power_supplies/chassis0/psus";

constexpr auto IBMCFFPSInterface =
    "xyz.openbmc_project.Configuration.IBMCFFPSConnector";
constexpr auto i2cBusProp = "I2CBus";
constexpr auto i2cAddressProp = "I2CAddress";
constexpr auto psuNameProp = "Name";
constexpr auto presLineName = "NamedPresenceGpio";

constexpr auto supportedConfIntf =
    "xyz.openbmc_project.Configuration.SupportedConfiguration";

const auto deviceDirPath = "/sys/bus/i2c/devices/";
const auto driverDirName = "/driver";

constexpr auto INPUT_HISTORY_SYNC_DELAY = 5;

PSUManager::PSUManager(sdbusplus::bus_t& bus, const sdeventplus::Event& e) :
    bus(bus), powerSystemInputs(bus, powerSystemsInputsObjPath),
    objectManager(bus, objectManagerObjPath),
    sensorsObjManager(bus, "/xyz/openbmc_project/sensors")
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

    // Request the bus name before the analyze() function, which is the one that
    // determines the brownout condition and sets the status d-bus property.
    bus.request_name(managerBusName);

    using namespace sdeventplus;
    auto interval = std::chrono::milliseconds(1000);
    timer = std::make_unique<utility::Timer<ClockId::Monotonic>>(
        e, std::bind(&PSUManager::analyze, this), interval);

    validationTimer = std::make_unique<utility::Timer<ClockId::Monotonic>>(
        e, std::bind(&PSUManager::validateConfig, this));

    try
    {
        powerConfigGPIO = createGPIO("power-config-full-load");
    }
    catch (const std::exception& e)
    {
        // Ignore error, GPIO may not be implemented in this system.
        powerConfigGPIO = nullptr;
    }

    // Subscribe to power state changes
    powerService = util::getService(POWER_OBJ_PATH, POWER_IFACE, bus);
    powerOnMatch = std::make_unique<sdbusplus::bus::match_t>(
        bus,
        sdbusplus::bus::match::rules::propertiesChanged(POWER_OBJ_PATH,
                                                        POWER_IFACE),
        [this](auto& msg) { this->powerStateChanged(msg); });

    initialize();
}

void PSUManager::initialize()
{
    try
    {
        // pgood is the latest read of the chassis pgood
        int pgood = 0;
        util::getProperty<int>(POWER_IFACE, "pgood", POWER_OBJ_PATH,
                               powerService, bus, pgood);

        // state is the latest requested power on / off transition
        auto method = bus.new_method_call(powerService.c_str(), POWER_OBJ_PATH,
                                          POWER_IFACE, "getPowerState");
        auto reply = bus.call(method);
        int state = 0;
        reply.read(state);

        if (state)
        {
            // Monitor PSUs anytime state is on
            powerOn = true;
            // In the power fault window if pgood is off
            powerFaultOccurring = !pgood;
            validationTimer->restartOnce(validationTimeout);
        }
        else
        {
            // Power is off
            powerOn = false;
            powerFaultOccurring = false;
            runValidateConfig = true;
        }
    }
    catch (const std::exception& e)
    {
        log<level::INFO>(
            fmt::format(
                "Failed to get power state, assuming it is off, error {}",
                e.what())
                .c_str());
        powerOn = false;
        powerFaultOccurring = false;
        runValidateConfig = true;
    }

    onOffConfig(phosphor::pmbus::ON_OFF_CONFIG_CONTROL_PIN_ONLY);
    clearFaults();
    updateMissingPSUs();
    setPowerConfigGPIO();

    log<level::INFO>(
        fmt::format("initialize: power on: {}, power fault occurring: {}",
                    powerOn, powerFaultOccurring)
            .c_str());
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
        auto properties = getAllProperties(bus, path, IBMCFFPSInterface,
                                           service);

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
        catch (const std::exception& e)
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

        auto invMatch = std::find_if(psus.begin(), psus.end(),
                                     [&invpath](auto& psu) {
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

        buildDriverName(*i2cbus, *i2caddr);
        log<level::DEBUG>(
            fmt::format("make PowerSupply bus: {} addr: {} presline: {}",
                        *i2cbus, *i2caddr, presline)
                .c_str());
        auto psu = std::make_unique<PowerSupply>(
            bus, invpath, *i2cbus, *i2caddr, driverName, presline,
            std::bind(
                std::mem_fn(&phosphor::power::manager::PSUManager::isPowerOn),
                this));
        psus.emplace_back(std::move(psu));

        // Subscribe to power supply presence changes
        auto presenceMatch = std::make_unique<sdbusplus::bus::match_t>(
            bus,
            sdbusplus::bus::match::rules::propertiesChanged(invpath,
                                                            INVENTORY_IFACE),
            [this](auto& msg) { this->presenceChanged(msg); });
        presenceMatches.emplace_back(std::move(presenceMatch));
    }

    if (psus.empty())
    {
        log<level::INFO>(fmt::format("No power supplies to monitor").c_str());
    }
    else
    {
        populateDriverName();
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

        // The PowerConfigFullLoad is an optional property, default it to false
        // since that's the default value of the power-config-full-load GPIO.
        sys.powerConfigFullLoad = false;
        propIt = properties.find("PowerConfigFullLoad");
        if (propIt != properties.end())
        {
            const bool* fullLoad = std::get_if<bool>(&(propIt->second));
            if (fullLoad != nullptr)
            {
                sys.powerConfigFullLoad = *fullLoad;
            }
        }

        supportedConfigs.emplace(*model, sys);
    }
    catch (const std::exception& e)
    {}
}

void PSUManager::getSystemProperties()
{
    try
    {
        util::DbusSubtree subtree = util::getSubTree(bus, INVENTORY_OBJ_PATH,
                                                     supportedConfIntf, 0);
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
    catch (const std::exception& e)
    {
        // Interface or property not found. Let the Interfaces Added callback
        // process the information once the interfaces are added to D-Bus.
    }
}

void PSUManager::entityManagerIfaceAdded(sdbusplus::message_t& msg)
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
            updateMissingPSUs();
        }

        itIntf = interfaces.find(IBMCFFPSInterface);
        if (itIntf != interfaces.cend())
        {
            log<level::INFO>(
                fmt::format("InterfacesAdded for: {}", IBMCFFPSInterface)
                    .c_str());
            getPSUProperties(itIntf->second);
            updateMissingPSUs();
        }

        // Call to validate the psu configuration if the power is on and both
        // the IBMCFFPSConnector and SupportedConfiguration interfaces have been
        // processed
        if (powerOn && !psus.empty() && !supportedConfigs.empty())
        {
            validationTimer->restartOnce(validationTimeout);
        }
    }
    catch (const std::exception& e)
    {
        // Ignore, the property may be of a different type than expected.
    }
}

void PSUManager::powerStateChanged(sdbusplus::message_t& msg)
{
    std::string msgSensor;
    std::map<std::string, std::variant<int>> msgData;
    msg.read(msgSensor, msgData);

    // Check if it was the state property that changed.
    auto valPropMap = msgData.find("state");
    if (valPropMap != msgData.end())
    {
        int state = std::get<int>(valPropMap->second);
        if (state)
        {
            // Power on requested
            powerOn = true;
            powerFaultOccurring = false;
            validationTimer->restartOnce(validationTimeout);
            clearFaults();
            syncHistory();
            setPowerConfigGPIO();
            setInputVoltageRating();
        }
        else
        {
            // Power off requested
            powerOn = false;
            powerFaultOccurring = false;
            runValidateConfig = true;
        }
    }

    // Check if it was the pgood property that changed.
    valPropMap = msgData.find("pgood");
    if (valPropMap != msgData.end())
    {
        int pgood = std::get<int>(valPropMap->second);
        if (!pgood)
        {
            // Chassis power good has turned off
            if (powerOn)
            {
                // pgood is off but state is on, in power fault window
                powerFaultOccurring = true;
            }
        }
    }
    log<level::INFO>(
        fmt::format(
            "powerStateChanged: power on: {}, power fault occurring: {}",
            powerOn, powerFaultOccurring)
            .c_str());
}

void PSUManager::presenceChanged(sdbusplus::message_t& msg)
{
    std::string msgSensor;
    std::map<std::string, std::variant<uint32_t, bool>> msgData;
    msg.read(msgSensor, msgData);

    // Check if it was the Present property that changed.
    auto valPropMap = msgData.find(PRESENT_PROP);
    if (valPropMap != msgData.end())
    {
        if (std::get<bool>(valPropMap->second))
        {
            // A PSU became present, force the PSU validation to run.
            runValidateConfig = true;
            validationTimer->restartOnce(validationTimeout);
        }
    }
}

void PSUManager::setPowerSupplyError(const std::string& psuErrorString)
{
    using namespace sdbusplus::xyz::openbmc_project;
    constexpr auto method = "setPowerSupplyError";

    try
    {
        // Call D-Bus method to inform pseq of PSU error
        auto methodMsg = bus.new_method_call(
            powerService.c_str(), POWER_OBJ_PATH, POWER_IFACE, method);
        methodMsg.append(psuErrorString);
        auto callReply = bus.call(methodMsg);
    }
    catch (const std::exception& e)
    {
        log<level::INFO>(
            fmt::format("Failed calling setPowerSupplyError due to error {}",
                        e.what())
                .c_str());
    }
}

void PSUManager::createError(const std::string& faultName,
                             std::map<std::string, std::string>& additionalData)
{
    using namespace sdbusplus::xyz::openbmc_project;
    constexpr auto loggingObjectPath = "/xyz/openbmc_project/logging";
    constexpr auto loggingCreateInterface =
        "xyz.openbmc_project.Logging.Create";

    try
    {
        additionalData["_PID"] = std::to_string(getpid());

        auto service = util::getService(loggingObjectPath,
                                        loggingCreateInterface, bus);

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
        setPowerSupplyError(faultName);
    }
    catch (const std::exception& e)
    {
        log<level::ERR>(
            fmt::format(
                "Failed creating event log for fault {} due to error {}",
                faultName, e.what())
                .c_str());
    }
}

void PSUManager::syncHistory()
{
    if (driverName != ACBEL_FSG032_DD_NAME)
    {
        if (!syncHistoryGPIO)
        {
            try
            {
                syncHistoryGPIO = createGPIO(INPUT_HISTORY_SYNC_GPIO);
            }
            catch (const std::exception& e)
            {
                // Not an error, system just hasn't implemented the synch gpio
                log<level::INFO>("No synchronization GPIO found");
                syncHistoryGPIO = nullptr;
            }
        }
        if (syncHistoryGPIO)
        {
            const std::chrono::milliseconds delay{INPUT_HISTORY_SYNC_DELAY};
            log<level::INFO>("Synchronize INPUT_HISTORY");
            syncHistoryGPIO->toggleLowHigh(delay);
            log<level::INFO>("Synchronize INPUT_HISTORY completed");
        }
    }

    // Always clear synch history required after calling this function
    for (auto& psu : psus)
    {
        psu->clearSyncHistoryRequired();
    }
}

void PSUManager::analyze()
{
    auto syncHistoryRequired = std::any_of(
        psus.begin(), psus.end(),
        [](const auto& psu) { return psu->isSyncHistoryRequired(); });
    if (syncHistoryRequired)
    {
        syncHistory();
    }

    for (auto& psu : psus)
    {
        psu->analyze();
    }

    analyzeBrownout();

    // Only perform individual PSU analysis if power is on and a brownout has
    // not already been logged
    if (powerOn && !brownoutLogged)
    {
        for (auto& psu : psus)
        {
            std::map<std::string, std::string> additionalData;

            if (!psu->isFaultLogged() && !psu->isPresent() &&
                !validationTimer->isEnabled())
            {
                std::map<std::string, std::string> requiredPSUsData;
                auto requiredPSUsPresent = hasRequiredPSUs(requiredPSUsData);
                if (!requiredPSUsPresent && isRequiredPSU(*psu))
                {
                    additionalData.merge(requiredPSUsData);
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
                // Add STATUS_WORD and STATUS_MFR last response, in padded
                // hexadecimal format.
                additionalData["STATUS_WORD"] =
                    fmt::format("{:#04x}", psu->getStatusWord());
                additionalData["STATUS_MFR"] = fmt::format("{:#02x}",
                                                           psu->getMFRFault());
                // If there are faults being reported, they possibly could be
                // related to a bug in the firmware version running on the power
                // supply. Capture that data into the error as well.
                additionalData["FW_VERSION"] = psu->getFWVersion();

                if (psu->hasCommFault())
                {
                    additionalData["STATUS_CML"] =
                        fmt::format("{:#02x}", psu->getStatusCML());
                    /* Attempts to communicate with the power supply have
                     * reached there limit. Create an error. */
                    additionalData["CALLOUT_DEVICE_PATH"] =
                        psu->getDevicePath();

                    createError(
                        "xyz.openbmc_project.Power.PowerSupply.Error.CommFault",
                        additionalData);

                    psu->setFaultLogged();
                }
                else if ((psu->hasInputFault() || psu->hasVINUVFault()))
                {
                    // Include STATUS_INPUT for input faults.
                    additionalData["STATUS_INPUT"] =
                        fmt::format("{:#02x}", psu->getStatusInput());

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
                else if (psu->hasPSKillFault())
                {
                    createError(
                        "xyz.openbmc_project.Power.PowerSupply.Error.PSKillFault",
                        additionalData);
                    psu->setFaultLogged();
                }
                else if (psu->hasVoutOVFault())
                {
                    // Include STATUS_VOUT for Vout faults.
                    additionalData["STATUS_VOUT"] =
                        fmt::format("{:#02x}", psu->getStatusVout());

                    additionalData["CALLOUT_INVENTORY_PATH"] =
                        psu->getInventoryPath();

                    createError(
                        "xyz.openbmc_project.Power.PowerSupply.Error.Fault",
                        additionalData);

                    psu->setFaultLogged();
                }
                else if (psu->hasIoutOCFault())
                {
                    // Include STATUS_IOUT for Iout faults.
                    additionalData["STATUS_IOUT"] =
                        fmt::format("{:#02x}", psu->getStatusIout());

                    createError(
                        "xyz.openbmc_project.Power.PowerSupply.Error.IoutOCFault",
                        additionalData);

                    psu->setFaultLogged();
                }
                else if (psu->hasVoutUVFault() || psu->hasPS12VcsFault() ||
                         psu->hasPSCS12VFault())
                {
                    // Include STATUS_VOUT for Vout faults.
                    additionalData["STATUS_VOUT"] =
                        fmt::format("{:#02x}", psu->getStatusVout());

                    additionalData["CALLOUT_INVENTORY_PATH"] =
                        psu->getInventoryPath();

                    createError(
                        "xyz.openbmc_project.Power.PowerSupply.Error.Fault",
                        additionalData);

                    psu->setFaultLogged();
                }
                // A fan fault should have priority over a temperature fault,
                // since a failed fan may lead to a temperature problem.
                // Only process if not in power fault window.
                else if (psu->hasFanFault() && !powerFaultOccurring)
                {
                    // Include STATUS_TEMPERATURE and STATUS_FANS_1_2
                    additionalData["STATUS_TEMPERATURE"] =
                        fmt::format("{:#02x}", psu->getStatusTemperature());
                    additionalData["STATUS_FANS_1_2"] =
                        fmt::format("{:#02x}", psu->getStatusFans12());

                    additionalData["CALLOUT_INVENTORY_PATH"] =
                        psu->getInventoryPath();

                    createError(
                        "xyz.openbmc_project.Power.PowerSupply.Error.FanFault",
                        additionalData);

                    psu->setFaultLogged();
                }
                else if (psu->hasTempFault())
                {
                    // Include STATUS_TEMPERATURE for temperature faults.
                    additionalData["STATUS_TEMPERATURE"] =
                        fmt::format("{:#02x}", psu->getStatusTemperature());

                    additionalData["CALLOUT_INVENTORY_PATH"] =
                        psu->getInventoryPath();

                    createError(
                        "xyz.openbmc_project.Power.PowerSupply.Error.Fault",
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
                // Only process if not in power fault window.
                else if (psu->hasPgoodFault() && !powerFaultOccurring)
                {
                    /* POWER_GOOD# is not low, or OFF is on */
                    additionalData["CALLOUT_INVENTORY_PATH"] =
                        psu->getInventoryPath();

                    createError(
                        "xyz.openbmc_project.Power.PowerSupply.Error.Fault",
                        additionalData);

                    psu->setFaultLogged();
                }
            }
        }
    }
}

void PSUManager::analyzeBrownout()
{
    // Count number of power supplies failing
    size_t presentCount = 0;
    size_t notPresentCount = 0;
    size_t acFailedCount = 0;
    size_t pgoodFailedCount = 0;
    for (const auto& psu : psus)
    {
        if (psu->isPresent())
        {
            ++presentCount;
            if (psu->hasACFault())
            {
                ++acFailedCount;
            }
            else if (psu->hasPgoodFault())
            {
                ++pgoodFailedCount;
            }
        }
        else
        {
            ++notPresentCount;
        }
    }

    // Only issue brownout failure if chassis pgood has failed, it has not
    // already been logged, at least one PSU has seen an AC fail, and all
    // present PSUs have an AC or pgood failure. Note an AC fail is only set if
    // at least one PSU is present.
    if (powerFaultOccurring && !brownoutLogged && acFailedCount &&
        (presentCount == (acFailedCount + pgoodFailedCount)))
    {
        // Indicate that the system is in a brownout condition by creating an
        // error log and setting the PowerSystemInputs status property to Fault.
        powerSystemInputs.status(
            sdbusplus::xyz::openbmc_project::State::Decorator::server::
                PowerSystemInputs::Status::Fault);

        std::map<std::string, std::string> additionalData;
        additionalData.emplace("NOT_PRESENT_COUNT",
                               std::to_string(notPresentCount));
        additionalData.emplace("VIN_FAULT_COUNT",
                               std::to_string(acFailedCount));
        additionalData.emplace("PGOOD_FAULT_COUNT",
                               std::to_string(pgoodFailedCount));
        log<level::INFO>(
            fmt::format(
                "Brownout detected, not present count: {}, AC fault count {}, pgood fault count: {}",
                notPresentCount, acFailedCount, pgoodFailedCount)
                .c_str());

        createError("xyz.openbmc_project.State.Shutdown.Power.Error.Blackout",
                    additionalData);
        brownoutLogged = true;
    }
    else
    {
        // If a brownout was previously logged but at least one PSU is not
        // currently in AC fault, determine if the brownout condition can be
        // cleared
        if (brownoutLogged && (acFailedCount < presentCount))
        {
            // Chassis only recognizes the PowerSystemInputs change when it is
            // off
            try
            {
                using PowerState = sdbusplus::xyz::openbmc_project::State::
                    server::Chassis::PowerState;
                PowerState currentPowerState;
                util::getProperty<PowerState>(
                    "xyz.openbmc_project.State.Chassis", "CurrentPowerState",
                    "/xyz/openbmc_project/state/chassis0",
                    "xyz.openbmc_project.State.Chassis0", bus,
                    currentPowerState);

                if (currentPowerState == PowerState::Off)
                {
                    // Indicate that the system is no longer in a brownout
                    // condition by setting the PowerSystemInputs status
                    // property to Good.
                    log<level::INFO>(
                        fmt::format(
                            "Brownout cleared, not present count: {}, AC fault count {}, pgood fault count: {}",
                            notPresentCount, acFailedCount, pgoodFailedCount)
                            .c_str());
                    powerSystemInputs.status(
                        sdbusplus::xyz::openbmc_project::State::Decorator::
                            server::PowerSystemInputs::Status::Good);
                    brownoutLogged = false;
                }
            }
            catch (const std::exception& e)
            {
                log<level::ERR>(
                    fmt::format("Error trying to clear brownout, error: {}",
                                e.what())
                        .c_str());
            }
        }
    }
}

void PSUManager::updateMissingPSUs()
{
    if (supportedConfigs.empty() || psus.empty())
    {
        return;
    }

    // Power supplies default to missing. If the power supply is present,
    // the PowerSupply object will update the inventory Present property to
    // true. If we have less than the required number of power supplies, and
    // this power supply is missing, update the inventory Present property
    // to false to indicate required power supply is missing. Avoid
    // indicating power supply missing if not required.

    auto presentCount =
        std::count_if(psus.begin(), psus.end(),
                      [](const auto& psu) { return psu->isPresent(); });

    for (const auto& config : supportedConfigs)
    {
        for (const auto& psu : psus)
        {
            auto psuModel = psu->getModelName();
            auto psuShortName = psu->getShortName();
            auto psuInventoryPath = psu->getInventoryPath();
            auto relativeInvPath =
                psuInventoryPath.substr(strlen(INVENTORY_OBJ_PATH));
            auto psuPresent = psu->isPresent();
            auto presProperty = false;
            auto propReadFail = false;

            try
            {
                presProperty = getPresence(bus, psuInventoryPath);
                propReadFail = false;
            }
            catch (const sdbusplus::exception_t& e)
            {
                propReadFail = true;
                // Relying on property change or interface added to retry.
                // Log an informational trace to the journal.
                log<level::INFO>(
                    fmt::format("D-Bus property {} access failure exception",
                                psuInventoryPath)
                        .c_str());
            }

            if (psuModel.empty())
            {
                if (!propReadFail && (presProperty != psuPresent))
                {
                    // We already have this property, and it is not false
                    // set Present to false
                    setPresence(bus, relativeInvPath, psuPresent, psuShortName);
                }
                continue;
            }

            if (config.first != psuModel)
            {
                continue;
            }

            if ((presentCount < config.second.powerSupplyCount) && !psuPresent)
            {
                setPresence(bus, relativeInvPath, psuPresent, psuShortName);
            }
        }
    }
}

void PSUManager::validateConfig()
{
    if (!runValidateConfig || supportedConfigs.empty() || psus.empty())
    {
        return;
    }

    for (const auto& psu : psus)
    {
        if ((psu->hasInputFault() || psu->hasVINUVFault()))
        {
            // Do not try to validate if input voltage fault present.
            validationTimer->restartOnce(validationTimeout);
            return;
        }
    }

    std::map<std::string, std::string> additionalData;
    auto supported = hasRequiredPSUs(additionalData);
    if (supported)
    {
        runValidateConfig = false;
        double actualVoltage;
        int inputVoltage;
        int previousInputVoltage = 0;
        bool voltageMismatch = false;

        for (const auto& psu : psus)
        {
            if (!psu->isPresent())
            {
                // Only present PSUs report a valid input voltage
                continue;
            }
            psu->getInputVoltage(actualVoltage, inputVoltage);
            if (previousInputVoltage && inputVoltage &&
                (previousInputVoltage != inputVoltage))
            {
                additionalData["EXPECTED_VOLTAGE"] =
                    std::to_string(previousInputVoltage);
                additionalData["ACTUAL_VOLTAGE"] =
                    std::to_string(actualVoltage);
                voltageMismatch = true;
            }
            if (!previousInputVoltage && inputVoltage)
            {
                previousInputVoltage = inputVoltage;
            }
        }
        if (!voltageMismatch)
        {
            return;
        }
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
    std::string model{};
    if (!validateModelName(model, additionalData))
    {
        return false;
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

        // Number of power supplies present should equal or exceed the expected
        // count
        if (presentCount < config.second.powerSupplyCount)
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

unsigned int PSUManager::getRequiredPSUCount()
{
    unsigned int requiredCount{0};

    // Verify we have the supported configuration and PSU information
    if (!supportedConfigs.empty() && !psus.empty())
    {
        // Find PSU models.  They should all be the same.
        std::set<std::string> models{};
        std::for_each(psus.begin(), psus.end(), [&models](const auto& psu) {
            if (!psu->getModelName().empty())
            {
                models.insert(psu->getModelName());
            }
        });

        // If exactly one model was found, find corresponding configuration
        if (models.size() == 1)
        {
            const std::string& model = *(models.begin());
            auto it = supportedConfigs.find(model);
            if (it != supportedConfigs.end())
            {
                requiredCount = it->second.powerSupplyCount;
            }
        }
    }

    return requiredCount;
}

bool PSUManager::isRequiredPSU(const PowerSupply& psu)
{
    // Get required number of PSUs; if not found, we don't know if PSU required
    unsigned int requiredCount = getRequiredPSUCount();
    if (requiredCount == 0)
    {
        return false;
    }

    // If total PSU count <= the required count, all PSUs are required
    if (psus.size() <= requiredCount)
    {
        return true;
    }

    // We don't currently get information from EntityManager about which PSUs
    // are required, so we have to do some guesswork.  First check if this PSU
    // is present.  If so, assume it is required.
    if (psu.isPresent())
    {
        return true;
    }

    // This PSU is not present.  Count the number of other PSUs that are
    // present.  If enough other PSUs are present, assume the specified PSU is
    // not required.
    unsigned int psuCount =
        std::count_if(psus.begin(), psus.end(),
                      [](const auto& psu) { return psu->isPresent(); });
    if (psuCount >= requiredCount)
    {
        return false;
    }

    // Check if this PSU was previously present.  If so, assume it is required.
    // We know it was previously present if it has a non-empty model name.
    if (!psu.getModelName().empty())
    {
        return true;
    }

    // This PSU was never present.  Count the number of other PSUs that were
    // previously present.  If including those PSUs is enough, assume the
    // specified PSU is not required.
    psuCount += std::count_if(psus.begin(), psus.end(), [](const auto& psu) {
        return (!psu->isPresent() && !psu->getModelName().empty());
    });
    if (psuCount >= requiredCount)
    {
        return false;
    }

    // We still haven't found enough PSUs.  Sort the inventory paths of PSUs
    // that were never present.  PSU inventory paths typically end with the PSU
    // number (0, 1, 2, ...).  Assume that lower-numbered PSUs are required.
    std::vector<std::string> sortedPaths;
    std::for_each(psus.begin(), psus.end(), [&sortedPaths](const auto& psu) {
        if (!psu->isPresent() && psu->getModelName().empty())
        {
            sortedPaths.push_back(psu->getInventoryPath());
        }
    });
    std::sort(sortedPaths.begin(), sortedPaths.end());

    // Check if specified PSU is close enough to start of list to be required
    for (const auto& path : sortedPaths)
    {
        if (path == psu.getInventoryPath())
        {
            return true;
        }
        if (++psuCount >= requiredCount)
        {
            break;
        }
    }

    // PSU was not close to start of sorted list; assume not required
    return false;
}

bool PSUManager::validateModelName(
    std::string& model, std::map<std::string, std::string>& additionalData)
{
    // Check that all PSUs have the same model name. Initialize the model
    // variable with the first PSU name found, then use it as a base to compare
    // against the rest of the PSUs and get its inventory path to use as callout
    // if needed.
    model.clear();
    std::string modelInventoryPath{};
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
            modelInventoryPath = psu->getInventoryPath();
            continue;
        }
        if (psuModel != model)
        {
            if (supportedConfigs.find(model) != supportedConfigs.end())
            {
                // The base model is supported, callout the mismatched PSU. The
                // mismatched PSU may or may not be supported.
                additionalData["EXPECTED_MODEL"] = model;
                additionalData["ACTUAL_MODEL"] = psuModel;
                additionalData["CALLOUT_INVENTORY_PATH"] =
                    psu->getInventoryPath();
            }
            else if (supportedConfigs.find(psuModel) != supportedConfigs.end())
            {
                // The base model is not supported, but the mismatched PSU is,
                // callout the base PSU.
                additionalData["EXPECTED_MODEL"] = psuModel;
                additionalData["ACTUAL_MODEL"] = model;
                additionalData["CALLOUT_INVENTORY_PATH"] = modelInventoryPath;
            }
            else
            {
                // The base model and the mismatched PSU are not supported or
                // could not be found in the supported configuration, callout
                // the mismatched PSU.
                additionalData["EXPECTED_MODEL"] = model;
                additionalData["ACTUAL_MODEL"] = psuModel;
                additionalData["CALLOUT_INVENTORY_PATH"] =
                    psu->getInventoryPath();
            }
            model.clear();
            return false;
        }
    }
    return true;
}

void PSUManager::setPowerConfigGPIO()
{
    if (!powerConfigGPIO)
    {
        return;
    }

    std::string model{};
    std::map<std::string, std::string> additionalData;
    if (!validateModelName(model, additionalData))
    {
        return;
    }

    auto config = supportedConfigs.find(model);
    if (config != supportedConfigs.end())
    {
        // The power-config-full-load is an open drain GPIO. Set it to low (0)
        // if the supported configuration indicates that this system model
        // expects the maximum number of power supplies (full load set to true).
        // Else, set it to high (1), this is the default.
        auto powerConfigValue =
            (config->second.powerConfigFullLoad == true ? 0 : 1);
        auto flags = gpiod::line_request::FLAG_OPEN_DRAIN;
        powerConfigGPIO->write(powerConfigValue, flags);
    }
}

void PSUManager::buildDriverName(uint64_t i2cbus, uint64_t i2caddr)
{
    namespace fs = std::filesystem;
    std::stringstream ss;
    ss << std::hex << std::setw(4) << std::setfill('0') << i2caddr;
    std::string symLinkPath = deviceDirPath + std::to_string(i2cbus) + "-" +
                              ss.str() + driverDirName;
    try
    {
        fs::path linkStrPath = fs::read_symlink(symLinkPath);
        driverName = linkStrPath.filename();
    }
    catch (const std::exception& e)
    {
        log<level::ERR>(fmt::format("Failed to find device driver {}, error {}",
                                    symLinkPath, e.what())
                            .c_str());
    }
}

void PSUManager::populateDriverName()
{
    std::string driverName;
    // Search in PSUs for driver name
    std::for_each(psus.begin(), psus.end(), [&driverName](auto& psu) {
        if (!psu->getDriverName().empty())
        {
            driverName = psu->getDriverName();
        }
    });
    // Assign driver name to all PSUs
    std::for_each(psus.begin(), psus.end(),
                  [=](auto& psu) { psu->setDriverName(driverName); });
}
} // namespace phosphor::power::manager
