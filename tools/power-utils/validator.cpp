#include "validator.hpp"

#include "model.hpp"

#include <phosphor-logging/lg2.hpp>

#include <iostream>

namespace validator
{
using namespace phosphor::power::util;
constexpr auto supportedConfIntf =
    "xyz.openbmc_project.Configuration.SupportedConfiguration";
const auto objectPath = "/";

bool PSUUpdateValidator::areAllPsuSameModel()
{
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
                lg2::error(
                    "PSU models do not match, targetPsuModel= {TARGET}, thisPsuModel= {THISPSU}",
                    "TARGET", targetPsuModel, "THISPSU", thisPsuModel);
                return false;
            }
        }
    }
    catch (const std::exception& e)
    {
        lg2::error("Failed to get all PSUs from EM, error {ERROR}", "ERROR", e);
        return false;
    }
    return true;
}

bool PSUUpdateValidator::countPresentPsus()
{
    auto psuPaths = getPSUInventoryPaths(bus);
    for (const auto& path : psuPaths)
    {
        auto present = false;
        try
        {
            getProperty(INVENTORY_IFACE, PRESENT_PROP, path,
                        INVENTORY_MGR_IFACE, bus, present);
            if (present)
            {
                if (!isItFunctional(path))
                {
                    lg2::error("PSU {PATH} is not functional", "PATH", path);
                    return false;
                }

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
    return true;
}

bool PSUUpdateValidator::getRequiredPsus()
{
    try
    {
        supportedObjects = getSubTree(bus, objectPath, supportedConfIntf, 0);
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
            properties =
                getAllProperties(bus, objPath, supportedConfIntf, service);
        }
        catch (const std::exception& e)
        {
            lg2::error(
                "Failed to get all PSU {PSUPATH} properties error: {ERR}",
                "PSUPATH", objPath, "ERR", e);
            return false;
        }
        auto propertyModel = properties.find("SupportedModel");
        if (propertyModel == properties.end())
        {
            continue;
        }
        try
        {
            auto supportedModel = std::get<std::string>(propertyModel->second);
            if ((supportedModel.empty()) || (supportedModel != targetPsuModel))
            {
                continue;
            }
        }
        catch (const std::bad_variant_access& e)
        {
            lg2::error("Failed to get supportedModel, error: {ERR}", "ERR", e);
        }

        try
        {
            auto redundantCountProp = properties.find("RedundantCount");
            if (redundantCountProp != properties.end())
            {
                redundantCount = static_cast<int>(
                    std::get<uint64_t>(redundantCountProp->second));
                break;
            }
        }
        catch (const std::bad_variant_access& e)
        {
            lg2::error("Redundant type mismatch, error: {ERR}", "ERR", e);
        }
    }
    return true;
}

bool PSUUpdateValidator::isItFunctional(const std::string& path)
{
    try
    {
        bool isFunctional = false;
        getProperty(OPERATIONAL_STATE_IFACE, FUNCTIONAL_PROP, path,
                    INVENTORY_MGR_IFACE, bus, isFunctional);
        return isFunctional;
    }
    catch (const std::exception& e)
    {
        lg2::error("Failed to get PSU fault status, error {ERR} ", "ERR", e);
        return false;
    }
}

bool PSUUpdateValidator::validToUpdate()
{
    if (areAllPsuSameModel() && countPresentPsus() && getRequiredPsus())
    {
        if (presentPsuCount >= redundantCount)
        {
            return true;
        }
    }
    return false;
}
} // namespace validator
