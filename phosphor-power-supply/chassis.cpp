#include "config.h"

#include "chassis.hpp"

#include <filesystem>
#include <iostream>

using namespace phosphor::logging;
using namespace phosphor::power::util;
namespace phosphor::power::chassis
{

constexpr auto IBMCFFPSInterface =
    "xyz.openbmc_project.Configuration.IBMCFFPSConnector";
constexpr auto chassisIdStr = "SlotNumber";
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

Chassis::Chassis(sdbusplus::bus_t& bus, const std::string& chassisPath) :
    bus(bus), chassisPath(chassisPath), findChassisUniqueId()
{
    getPSUConfiguration();
}

void Chassis::getPSUConfiguration()
{
    using namespace phosphor::power::util;
    namespace fs = std::filesystem;
    auto depth = 0;

    try
    {
        if (chassisPathUniqueId == invalidObjectPathUniqueId)
        {
            lg2::error("Chassis does not have slot number: {CHASSISPATH}",
                       "CHASSISPATH", chassisPath);
            return;
        }
        auto connectorsSubTree = getSubTree(bus, "/", IBMCFFPSInterface, depth);
        for (const auto& [path, services] : connectorsSubTree)
        {
            uint64_t id;
            fs::path fspath(path);
            getProperty(decoratorChassisId, chassisIdStr, fspath.parent_path(),
                        entityMgrService, bus, id);
            if (id == static_cast<uint64_t>(chassisPathUniqueId))

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

    if ((i2cbus) && (i2caddr) && (psuname) && (!psuname->empty()))
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
            std::bind(
                std::mem_fn(&phosphor::power::chassis::Chassis::isPowerOn),
                this),
            chassisPath);
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
    // TBD
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

        supported_psu_configuration supportedPsuConfig;
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
                  [=](auto& psu) { psu->setDriverName(driverName); });
}

void Chassis::findChassisUniqueId()
{
    try
    {
        getProperty(decoratorChassisId, chassisIdStr, chassisPath,
                    INVENTORY_MGR_IFACE, bus, chassisPathUniqueId);
    }
    catch (const sdbusplus::exception_t& e)
    {
        lg2::error("Failed to chassis ID  - exception: {ERROR}", "ERROR", e);
    }
}

} // namespace phosphor::power::chassis
