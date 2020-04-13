#pragma once

#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>

namespace phosphor::power::regulators::control
{

constexpr auto busName = "xyz.openbmc_project.Power.Regulators";
constexpr auto objPath = "/xyz/openbmc_project/power/regulators/manager";
constexpr auto interface = "xyz.openbmc_project.Power.Regulators.Manager";

using namespace phosphor::logging;

/**
 * @brief Call a dbus method
 * @param[in] method - Method name to call
 * @param[in] args - Any needed arguments to the method
 *
 * @return Response message from the method call
 */
template <typename... Args>
auto callMethod(const std::string& method, Args&&... args)
{
    auto bus = sdbusplus::bus::new_default();
    auto reqMsg =
        bus.new_method_call(busName, objPath, interface, method.c_str());
    reqMsg.append(std::forward<Args>(args)...);

    sdbusplus::message::message respMsg;
    try
    {
        respMsg = bus.call(reqMsg);
    }
    catch (const sdbusplus::exception::SdBusError&)
    {
        log<level::ERR>("Error on regulator control method call",
                        entry("METHOD=%s", method.c_str()));
    }
    return respMsg;
}

} // namespace phosphor::power::regulators::control
