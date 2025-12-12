#pragma once
#include <array>
/* const expressions shared in this repository */

constexpr auto ASSOCIATION_IFACE = "xyz.openbmc_project.Association";
constexpr auto LOGGING_IFACE = "xyz.openbmc_project.Logging.Entry";
constexpr auto INVENTORY_IFACE = "xyz.openbmc_project.Inventory.Item";
constexpr auto POWER_IFACE = "org.openbmc.control.Power";
constexpr auto INVENTORY_MGR_IFACE = "xyz.openbmc_project.Inventory.Manager";
constexpr auto ASSET_IFACE = "xyz.openbmc_project.Inventory.Decorator.Asset";
constexpr auto PSU_INVENTORY_IFACE =
    "xyz.openbmc_project.Inventory.Item.PowerSupply";
constexpr auto OPERATIONAL_STATE_IFACE =
    "xyz.openbmc_project.State.Decorator.OperationalStatus";
constexpr auto VERSION_IFACE = "xyz.openbmc_project.Software.Version";
constexpr auto AVAILABILITY_IFACE =
    "xyz.openbmc_project.State.Decorator.Availability";
constexpr auto ENABLE_IFACE = "xyz.openbmc_project.Object.Enable";
constexpr auto POWER_SYSTEM_INPUTS_IFACE =
    "xyz.openbmc_project.State.Decorator.PowerSystemInputs";
constexpr auto ASSOC_DEF_IFACE = "xyz.openbmc_project.Association.Definitions";
constexpr auto CHASSIS_IFACE = "xyz.openbmc_project.Inventory.Item.Chassis";
#ifdef IBM_VPD
constexpr auto DINF_IFACE = "com.ibm.ipzvpd.DINF";
constexpr auto VINI_IFACE = "com.ibm.ipzvpd.VINI";
#endif

constexpr auto ENDPOINTS_PROP = "endpoints";
constexpr auto MESSAGE_PROP = "Message";
constexpr auto RESOLVED_PROP = "Resolved";
constexpr auto PRESENT_PROP = "Present";
constexpr auto FUNCTIONAL_PROP = "Functional";
constexpr auto AVAILABLE_PROP = "Available";
constexpr auto ENABLED_PROP = "Enabled";
constexpr auto STATUS_PROP = "Status";
constexpr auto POWER_STATE_PROP = "state";
constexpr auto POWER_GOOD_PROP = "pgood";
constexpr auto ASSOC_PROP = "Associations";

constexpr auto INVENTORY_OBJ_PATH = "/xyz/openbmc_project/inventory";
constexpr auto POWER_OBJ_PATH = "/org/openbmc/control/power0";

constexpr auto INPUT_HISTORY = "input_history";

constexpr std::array<const char*, 1> psuEventInterface = {
    "xyz.openbmc_project.State.Decorator.OperationalStatus"};
