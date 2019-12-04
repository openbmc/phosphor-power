#pragma once

/* const expressions shared in this repository */

constexpr auto ASSOCIATION_IFACE = "xyz.openbmc_project.Association";
constexpr auto LOGGING_IFACE = "xyz.openbmc_project.Logging.Entry";
constexpr auto INVENTORY_IFACE = "xyz.openbmc_project.Inventory.Item";
constexpr auto POWER_IFACE = "org.openbmc.control.Power";
constexpr auto INVENTORY_MGR_IFACE = "xyz.openbmc_project.Inventory.Manager";
constexpr auto ASSET_IFACE = "xyz.openbmc_project.Inventory.Decorator.Asset";
constexpr auto VERSION_IFACE = "xyz.openbmc_project.Software.Version";
constexpr auto PSU_INVENTORY_IFACE =
    "xyz.openbmc_project.Inventory.Item.PowerSupply";

constexpr auto ENDPOINTS_PROP = "endpoints";
constexpr auto MESSAGE_PROP = "Message";
constexpr auto RESOLVED_PROP = "Resolved";
constexpr auto PRESENT_PROP = "Present";
constexpr auto VERSION_PURPOSE_PROP = "Purpose";

constexpr auto INVENTORY_OBJ_PATH = "/xyz/openbmc_project/inventory";
constexpr auto POWER_OBJ_PATH = "/org/openbmc/control/power0";

constexpr auto INPUT_HISTORY = "input_history";
