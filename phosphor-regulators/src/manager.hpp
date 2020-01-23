#pragma once

#include <sdbusplus/bus.hpp>
#include <xyz/openbmc_project/Power/Regulators/Manager/server.hpp>

namespace phosphor
{
namespace power
{
namespace regulators
{

constexpr auto busName = "xyz.openbmc_project.Power.Regulators";
constexpr auto objPath = "/xyz/openbmc_project/power/regulators/manager";

using ManagerObject = sdbusplus::server::object::object<
    sdbusplus::xyz::openbmc_project::Power::Regulators::server::Manager>;

class Manager : public ManagerObject
{
public:

    Manager() = delete;
    Manager(const Manager&) = delete;
    Manager(Manager&&) = delete;
    Manager& operator=(const Manager&) = delete;
    Manager& operator=(Manager&&) = delete;
    ~Manager() = default;

    Manager(sdbusplus::bus::bus& bus);

    virtual void configure();

    virtual void monitor();

private:

    sdbusplus::bus::bus& bus;
};

} // namespace regulators
} // namespace power
} // namespace phosphor
