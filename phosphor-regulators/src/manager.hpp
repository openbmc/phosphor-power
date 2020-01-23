#pragma once

#include <interfaces/manager.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>

namespace phosphor
{
namespace power
{
namespace regulators
{

constexpr auto busName = "xyz.openbmc_project.Power.Regulators";
constexpr auto objPath = "/xyz/openbmc_project/power/regulators/manager";

using ManagerObject = sdbusplus::server::object::object<
    phosphor::power::regulators::interface::Manager>;

class Manager : public ManagerObject
{
  public:
    Manager() = delete;
    Manager(const Manager&) = delete;
    Manager(Manager&&) = delete;
    Manager& operator=(const Manager&) = delete;
    Manager& operator=(Manager&&) = delete;
    ~Manager() = default;

    /**
     * Constructor
     * Creates a manager of the handling regulators.
     *
     * @param[in] bus - the dbus bus
     */
    Manager(sdbusplus::bus::bus& bus);

    /**
     * @brief Overridden manager object's configure method
     */
    void configure() override;

    /**
     * @brief Overridden manager object's monitor method
     *
     * @param[in] enable - Enable or disable regulator monitoring
     */
    void monitor(bool enable) override;

  private:
    /**
     * The dbus bus
     */
    sdbusplus::bus::bus& bus;
};

} // namespace regulators
} // namespace power
} // namespace phosphor
