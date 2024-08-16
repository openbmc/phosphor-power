#pragma once

#include <sdbusplus/bus.hpp>
#include <sdbusplus/sdbus.hpp>

#include <chrono>

namespace phosphor::power::regulators::control
{

constexpr auto busName = "xyz.openbmc_project.Power.Regulators";
constexpr auto objPath = "/xyz/openbmc_project/power/regulators/manager";
constexpr auto interface = "xyz.openbmc_project.Power.Regulators.Manager";

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

    // Set timeout to 6 minutes; some regulator methods take over 5 minutes
    using namespace std::chrono_literals;
    sdbusplus::SdBusDuration timeout{6min};
    return bus.call(reqMsg, timeout);
}

} // namespace phosphor::power::regulators::control
