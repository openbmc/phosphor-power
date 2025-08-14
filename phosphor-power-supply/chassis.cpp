#include "config.h"

#include "chassis.hpp"

#include <iostream>

using namespace phosphor::logging;
using namespace phosphor::power::util;
namespace phosphor::power::chassis
{

constexpr auto powerSystemsInputsObjPath =
    "/xyz/openbmc_project/power/power_supplies/chassis{}/psus";
constexpr auto IBMCFFPSInterface =
    "xyz.openbmc_project.Configuration.IBMCFFPSConnector";
constexpr auto chassisIdProp = "SlotNumber";
constexpr auto i2cBusProp = "I2CBus";
constexpr auto i2cAddressProp = "I2CAddress";
constexpr auto psuNameProp = "Name";
constexpr auto presLineName = "NamedPresenceGpio";
constexpr auto supportedConfIntf =
    "xyz.openbmc_project.Configuration.SupportedConfiguration";
const auto deviceDirPath = "/sys/bus/i2c/devices/";
const auto driverDirName = "/driver";

const auto entityMgrService = "xyz.openbmc_project.EntityManager";
const auto decoratorChassisId = "xyz.openbmc_project.Inventory.Decorator.Slot";

constexpr auto INPUT_HISTORY_SYNC_DELAY = 5;

Chassis::Chassis(sdbusplus::bus_t& bus, const std::string& chassisPath,
                 const sdeventplus::Event& e) :
    bus(bus), chassisPath(chassisPath),
    chassisPathUniqueId(getChassisPathUniqueId(chassisPath)),
    powerSystemInputs(
        bus, std::format(powerSystemsInputsObjPath, chassisPathUniqueId)),
    eventLoop(e)
{
    saveChassisName();
    getPSUConfiguration();
    getSupportedConfiguration();
    initPowerMonitoring();
}

void Chassis::getPSUConfiguration()
{
    auto depth = 0;

    try
    {
        if (chassisPathUniqueId == invalidObjectPathUniqueId)
        {
            lg2::error("Chassis does not have chassis ID: {CHASSISPATH}",
                       "CHASSISPATH", chassisPath);
            return;
        }
        auto connectorsSubTree = getSubTree(bus, "/", IBMCFFPSInterface, depth);
        for (const auto& [path, services] : connectorsSubTree)
        {
            if (chassisPathUniqueId == getParentEMUniqueId(bus, path))
            {
                // For each object in the array of objects, I want
                // to get properties from the service, path, and
                // interface.
                auto properties = getAllProperties(bus, path, IBMCFFPSInterface,
                                                   entityMgrService);
                getPSUProperties(properties);
            }
        }
    }
    catch (const sdbusplus::exception_t& e)
    {
        lg2::error("Failed while getting configuration - exception: {ERROR}",
                   "ERROR", e);
    }

    if (psus.empty())
    {
        // Interface or properties not found. Let the Interfaces Added callback
        // process the information once the interfaces are added to D-Bus.
        lg2::info("No power supplies to monitor");
    }
}

void Chassis::getPSUProperties(util::DbusPropertyMap& properties)
{
    std::string basePSUInvPath = chassisPath + "/motherboard/powersupply";

    // From passed in properties, I want to get: I2CBus, I2CAddress,
    // and Name. Create a power supply object, using Name to build the inventory
    // path.

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

    if (i2cbus && i2caddr && psuname && !psuname->empty())
    {
        std::string invpath = basePSUInvPath;
        invpath.push_back(psuname->back());
        std::string presline = "";

        lg2::debug("Inventory Path: {INVPATH}", "INVPATH", invpath);

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

        buildDriverName(*i2cbus, *i2caddr);
        lg2::debug(
            "make PowerSupply bus: {I2CBUS} addr: {I2CADDR} presline: {PRESLINE}",
            "I2CBUS", *i2cbus, "I2CADDR", *i2caddr, "PRESLINE", presline);

        auto psu = std::make_unique<PowerSupply>(
            bus, invpath, *i2cbus, *i2caddr, driverName, presline,
            std::bind(&Chassis::isPowerOn, this), chassisShortName);
        psus.emplace_back(std::move(psu));

        // Subscribe to power supply presence changes
        auto presenceMatch = std::make_unique<sdbusplus::bus::match_t>(
            bus,
            sdbusplus::bus::match::rules::propertiesChanged(invpath,
                                                            INVENTORY_IFACE),
            [this](auto& msg) { this->psuPresenceChanged(msg); });
        presenceMatches.emplace_back(std::move(presenceMatch));
    }
    if (psus.empty())
    {
        lg2::info("No power supplies to monitor");
    }
    else
    {
        populateDriverName();
    }
}

void Chassis::getSupportedConfiguration()
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

            if (chassisPathUniqueId == getParentEMUniqueId(bus, objPath))
            {
                auto properties = util::getAllProperties(
                    bus, objPath, supportedConfIntf, service);
                populateSupportedConfiguration(properties);
                break;
            }
        }
    }
    catch (const std::exception& e)
    {
        // Interface or property not found. Let the Interfaces Added callback
        // process the information once the interfaces are added to D-Bus.
        lg2::info("Interface or Property not found, error {ERROR}", "ERROR", e);
    }
}

void Chassis::populateSupportedConfiguration(
    const util::DbusPropertyMap& properties)
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

        SupportedPsuConfiguration supportedPsuConfig;
        propIt = properties.find("RedundantCount");
        if (propIt != properties.end())
        {
            const uint64_t* count = std::get_if<uint64_t>(&(propIt->second));
            if (count != nullptr)
            {
                supportedPsuConfig.powerSupplyCount = *count;
            }
        }
        propIt = properties.find("InputVoltage");
        if (propIt != properties.end())
        {
            const std::vector<uint64_t>* voltage =
                std::get_if<std::vector<uint64_t>>(&(propIt->second));
            if (voltage != nullptr)
            {
                supportedPsuConfig.inputVoltage = *voltage;
            }
        }

        // The PowerConfigFullLoad is an optional property, default it to false
        // since that's the default value of the power-config-full-load GPIO.
        supportedPsuConfig.powerConfigFullLoad = false;
        propIt = properties.find("PowerConfigFullLoad");
        if (propIt != properties.end())
        {
            const bool* fullLoad = std::get_if<bool>(&(propIt->second));
            if (fullLoad != nullptr)
            {
                supportedPsuConfig.powerConfigFullLoad = *fullLoad;
            }
        }

        supportedConfigs.emplace(*model, supportedPsuConfig);
    }
    catch (const std::exception& e)
    {
        lg2::info("populateSupportedConfiguration error {ERR}", "ERR", e);
    }
}

void Chassis::psuPresenceChanged(sdbusplus::message_t& msg)
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

void Chassis::buildDriverName(uint64_t i2cbus, uint64_t i2caddr)
{
    namespace fs = std::filesystem;
    std::stringstream ss;
    ss << std::hex << std::setw(4) << std::setfill('0') << i2caddr;
    std::string symLinkPath =
        deviceDirPath + std::to_string(i2cbus) + "-" + ss.str() + driverDirName;
    try
    {
        fs::path linkStrPath = fs::read_symlink(symLinkPath);
        driverName = linkStrPath.filename();
    }
    catch (const std::exception& e)
    {
        lg2::error(
            "Failed to find device driver {SYM_LINK_PATH}, error {ERROR_STR}",
            "SYM_LINK_PATH", symLinkPath, "ERROR_STR", e);
    }
}

void Chassis::populateDriverName()
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
                  [&driverName](auto& psu) { psu->setDriverName(driverName); });
}

uint64_t Chassis::getChassisPathUniqueId(const std::string& path)
{
    try
    {
        return getChassisInventoryUniqueId(bus, path);
    }
    catch (const sdbusplus::exception_t& e)
    {
        lg2::error(
            "Failed to find chassis path {CHASSIS_PATH} ID - exception: {ERROR}",
            "CHASSIS_PATH", path, "ERROR", e);
    }
    return invalidObjectPathUniqueId;
}

void Chassis::initPowerMonitoring()
{
    using namespace sdeventplus;

    validationTimer = std::make_unique<utility::Timer<ClockId::Monotonic>>(
        eventLoop, std::bind(&Chassis::validateConfig, this));
    attemptToCreatePowerConfigGPIO();

    // Subscribe to power state changes
    powerService = util::getService(POWER_OBJ_PATH, POWER_IFACE, bus);
    powerOnMatch = std::make_unique<sdbusplus::bus::match_t>(
        bus,
        sdbusplus::bus::match::rules::propertiesChanged(POWER_OBJ_PATH,
                                                        POWER_IFACE),
        [this](auto& msg) { this->powerStateChanged(msg); });
    // TODO initialize the chassis
}

void Chassis::validateConfig()
{
    if (!runValidateConfig || supportedConfigs.empty() || psus.empty())
    {
        return;
    }

    for (const auto& psu : psus)
    {
        if ((psu->hasInputFault() || psu->hasVINUVFault()) && psu->isPresent())
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

void Chassis::syncHistory()
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
                lg2::info("No synchronization GPIO found");
                syncHistoryGPIO = nullptr;
            }
        }
        if (syncHistoryGPIO)
        {
            const std::chrono::milliseconds delay{INPUT_HISTORY_SYNC_DELAY};
            lg2::info("Synchronize INPUT_HISTORY");
            syncHistoryGPIO->toggleLowHigh(delay);
            lg2::info("Synchronize INPUT_HISTORY completed");
        }
    }

    // Always clear synch history required after calling this function
    for (auto& psu : psus)
    {
        psu->clearSyncHistoryRequired();
    }
}

void Chassis::analyze()
{
    auto syncHistoryRequired =
        std::any_of(psus.begin(), psus.end(), [](const auto& psu) {
            return psu->isSyncHistoryRequired();
        });
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
    //
    // Note: TODO Check the chassis state when the power sequencer publishes
    // chassis power on and system power on
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
                // TODO check required PSU

                if (!requiredPSUsPresent)
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
                    std::format("{:#04x}", psu->getStatusWord());
                additionalData["STATUS_MFR"] =
                    std::format("{:#02x}", psu->getMFRFault());
                // If there are faults being reported, they possibly could be
                // related to a bug in the firmware version running on the power
                // supply. Capture that data into the error as well.
                additionalData["FW_VERSION"] = psu->getFWVersion();

                if (psu->hasCommFault())
                {
                    additionalData["STATUS_CML"] =
                        std::format("{:#02x}", psu->getStatusCML());
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
                        std::format("{:#02x}", psu->getStatusInput());

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
                        std::format("{:#02x}", psu->getStatusVout());

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
                        std::format("{:#02x}", psu->getStatusIout());

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
                        std::format("{:#02x}", psu->getStatusVout());

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
                        std::format("{:#02x}", psu->getStatusTemperature());
                    additionalData["STATUS_FANS_1_2"] =
                        std::format("{:#02x}", psu->getStatusFans12());

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
                        std::format("{:#02x}", psu->getStatusTemperature());

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

void Chassis::analyzeBrownout()
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
        //  Indicate that the system is in a brownout condition by creating an
        //  error log and setting the PowerSystemInputs status property to
        //  Fault.
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
        lg2::info(
            "Brownout detected, not present count: {NOT_PRESENT_COUNT}, AC fault count {AC_FAILED_COUNT}, pgood fault count: {PGOOD_FAILED_COUNT}",
            "NOT_PRESENT_COUNT", notPresentCount, "AC_FAILED_COUNT",
            acFailedCount, "PGOOD_FAILED_COUNT", pgoodFailedCount);

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
            // TODO Power State
        }
    }
}

void Chassis::createError(const std::string& faultName,
                          std::map<std::string, std::string>& additionalData)
{
    using namespace sdbusplus::xyz::openbmc_project;
    constexpr auto loggingObjectPath = "/xyz/openbmc_project/logging";
    constexpr auto loggingCreateInterface =
        "xyz.openbmc_project.Logging.Create";

    try
    {
        additionalData["_PID"] = std::to_string(getpid());

        auto service =
            util::getService(loggingObjectPath, loggingCreateInterface, bus);

        if (service.empty())
        {
            lg2::error("Unable to get logging manager service");
            return;
        }

        auto method = bus.new_method_call(service.c_str(), loggingObjectPath,
                                          loggingCreateInterface, "Create");

        auto level = Logging::server::Entry::Level::Error;
        method.append(faultName, level, additionalData);

        auto reply = bus.call(method);
        // TODO set power supply error
    }
    catch (const std::exception& e)
    {
        lg2::error(
            "Failed creating event log for fault {FAULT_NAME} due to error {ERROR}",
            "FAULT_NAME", faultName, "ERROR", e);
    }
}

void Chassis::attemptToCreatePowerConfigGPIO()
{
    try
    {
        powerConfigGPIO = createGPIO("power-config-full-load");
    }
    catch (const std::exception& e)
    {
        powerConfigGPIO = nullptr;
        lg2::info("GPIO not implemented in {CHASSIS}", "CHASSIS",
                  chassisShortName);
    }
}

void Chassis::supportedConfigurationInterfaceAdded(
    const util::DbusPropertyMap& properties)
{
    populateSupportedConfiguration(properties);
    updateMissingPSUs();
}

void Chassis::psuInterfaceAdded(util::DbusPropertyMap& properties)
{
    getPSUProperties(properties);
    updateMissingPSUs();
}

bool Chassis::hasRequiredPSUs(
    std::map<std::string, std::string>& additionalData)
{
    // ignore the following loop so code will compile
    for (const auto& pair : additionalData)
    {
        std::cout << "Key = " << pair.first
                  << " additionalData value = " << pair.second << "\n";
    }
    return true;

    // TODO validate having the required PSUs
}

void Chassis::updateMissingPSUs()
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
                lg2::info(
                    "D-Bus property {PSU_INVENTORY_PATH} access failure exception",
                    "PSU_INVENTORY_PATH", psuInventoryPath);
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

void Chassis::powerStateChanged(sdbusplus::message_t& msg)
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
            // TODO clear faults

            syncHistory();
            // TODO set power config

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
    lg2::info(
        "powerStateChanged: power on: {POWER_ON}, power fault occurring: {POWER_FAULT_OCCURRING}",
        "POWER_ON", powerOn, "POWER_FAULT_OCCURRING", powerFaultOccurring);
}

} // namespace phosphor::power::chassis
