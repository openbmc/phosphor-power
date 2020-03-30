#pragma once
#include <map>
#include <string>
#include <sdbusplus/sdbus.hpp>
#include <sdbusplus/server.hpp>
#include <systemd/sd-bus.h>
#include <tuple>
#include <variant>



namespace sdbusplus
{
namespace xyz
{
namespace openbmc_project
{
namespace Power
{
namespace Regulators
{
namespace server
{

class Manager
{
    public:
        /* Define all of the basic class operations:
         *     Not allowed:
         *         - Default constructor to avoid nullptrs.
         *         - Copy operations due to internal unique_ptr.
         *         - Move operations due to 'this' being registered as the
         *           'context' with sdbus.
         *     Allowed:
         *         - Destructor.
         */
        Manager() = delete;
        Manager(const Manager&) = delete;
        Manager& operator=(const Manager&) = delete;
        Manager(Manager&&) = delete;
        Manager& operator=(Manager&&) = delete;
        virtual ~Manager() = default;

        /** @brief Constructor to put object onto bus at a dbus path.
         *  @param[in] bus - Bus to attach to.
         *  @param[in] path - Path to attach at.
         */
        Manager(bus::bus& bus, const char* path);



        /** @brief Implementation for Configure
         *  Request to configure the regulators according to the machine's regulators configuration json.
         */
        virtual void configure(
            ) = 0;

        /** @brief Implementation for Monitor
         *  Begin to monitor the regulators according to the machine's regulators configuration json.
         *
         *  @param[in] enable - Used to enable or disable monitoring of the regulators.
         */
        virtual void monitor(
            bool enable) = 0;




        /** @brief Emit interface added */
        void emit_added()
        {
            _xyz_openbmc_project_Power_Regulators_Manager_interface.emit_added();
        }

        /** @brief Emit interface removed */
        void emit_removed()
        {
            _xyz_openbmc_project_Power_Regulators_Manager_interface.emit_removed();
        }

        static constexpr auto interface = "xyz.openbmc_project.Power.Regulators.Manager";

    private:

        /** @brief sd-bus callback for Configure
         */
        static int _callback_Configure(
            sd_bus_message*, void*, sd_bus_error*);

        /** @brief sd-bus callback for Monitor
         */
        static int _callback_Monitor(
            sd_bus_message*, void*, sd_bus_error*);


        static const vtable::vtable_t _vtable[];
        sdbusplus::server::interface::interface
                _xyz_openbmc_project_Power_Regulators_Manager_interface;
        sdbusplus::SdBusInterface *_intf;


};


} // namespace server
} // namespace Regulators
} // namespace Power
} // namespace openbmc_project
} // namespace xyz

namespace message
{
namespace details
{
} // namespace details
} // namespace message
} // namespace sdbusplus
