#include "validator.hpp"

#include "model.hpp"
#include "types.hpp"
#include "utility.hpp"

#include <CLI/CLI.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/bus.hpp>

#include <iostream>

using namespace std;

// const auto PSUPATH =
//   "/xyz/openbmc_project/inventory/system/chassis/motherboard/powersupply0";

namespace validator
{
using namespace phosphor::power::util;
constexpr auto supportedConfIntf =
    "xyz.openbmc_project.Configuration.SupportedConfiguration";
const auto IBMCFFPSInterface =
    "xyz.openbmc_project.Configuration.IBMCFFPSConnector";
const auto objectPath = "/xyz/openbmc_project/inventory";

// bool PSUUpdateValidator::areAllPsuSameModel(const std::string& psuPath)
bool PSUUpdateValidator::areAllPsuSameModel()
{
    cout << "areAllPsuSameModel psuPath = " << psuPath << "\n";
    try
    {
        targetPsuModel = model::getModel(bus, psuPath);
        psuPaths = getPSUInventoryPaths(bus);
        for (const auto& path : psuPaths)
        {
            auto thisPsuModel = model::getModel(bus, path);
            // All PSUs must have same model
            if (targetPsuModel != thisPsuModel)
            {
                return false;
            }
        }
    }
    catch (const exception& e)
    {
        lg2::error("Failed to get all PSUs from EM, error {ERROR}", "ERROR", e);
        return false;
    }
    cout << "areAllPsuSameModel return true\n";
    return true;
}

bool PSUUpdateValidator::countPresentPsus()
{
    auto psuPaths = phosphor::power::util::getPSUInventoryPaths(bus);
    for (const auto& path : psuPaths)
    {
        auto present = false;
        try
        {
            auto service =
                phosphor::power::util::getService(path, INVENTORY_IFACE, bus);
            phosphor::power::util::getProperty(INVENTORY_IFACE, PRESENT_PROP,
                                               path, service, bus, present);
            if (present)
            {
                presentPsuCount++;
            }
        }
        catch (const std::exception& e)
        {
            lg2::error("Failed to get PSU present status, error {ERR} ", "ERR",
                       e);
            return false;
        }
    }
    cout << "present psus " << presentPsuCount << "\n";
    return true;
}

bool PSUUpdateValidator::getRequiredPsus()
{
    try
    {
        supportedObjects = phosphor::power::util::getSubTree(
            bus, objectPath, supportedConfIntf, 0);
    }
    catch (std::exception& e)
    {
        lg2::error("Failed to retrieve supported configuration");
        return false;
    }

    for (const auto& [objPath, services] : supportedObjects)
    {
        if (objPath.empty() || services.empty())
        {
            continue;
        }

        std::string service = services.begin()->first;
        try
        {
            properties = getAllProperties(bus, objPath, supportedConfIntf,
                                          services.begin()->first);
        }
        catch (const std::exception& e)
        {
            lg2::error("Failed to get all PSU {PSUPATH} property error: {ERR}",
                       "PSUPATH", objPath, "ERR", e);
            return false;
        }
        auto propertyModel = properties.find("SupportedModel");
        if (propertyModel == properties.end())
        {
            continue;
        }
        auto supportedModel =
            std::get_if<std::string>(&(propertyModel->second));
        if (supportedModel->c_str() != targetPsuModel)
        {
            continue;
        }

        auto redundantCountProp = properties.find("RedundantCount");
        if (redundantCountProp != properties.end())
        {
            redundantCount =
                *(std::get_if<uint64_t>(&(redundantCountProp->second)));
            break;
        }
    }
    cout << "redundant count is " << redundantCount << "\n";
    return true;
}

bool PSUUpdateValidator::validToUpdate()
{
    if (areAllPsuSameModel() && countPresentPsus() && getRequiredPsus())
    {
        cout << "presentcount = " << presentPsuCount << "\n";
        cout << "redundantCount = " << redundantCount << "\n";
        if (presentPsuCount >= redundantCount)
        {
            return true;
        }
    }
    return false;
}
} // namespace validator

#if 0
int main()
{
    auto bus = sdbusplus::bus::new_default();
    auto powerState = phosphor::power::util::isPoweredOn(bus, true);
    cout << "Power ON Status: " << powerState << "\n";
    std::string temp(PSUPATH);
    validator::PSUUpdateValidator myobj(bus, temp);

    if (myobj.validToUpdate())
    {
        cout << "\n\nValid to update \n\n";
    }

    else
    {
        cout << "\n\nNot Valid to update \n\n";
    }
}
#endif
