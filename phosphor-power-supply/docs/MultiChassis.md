# Multi-Chassis Support for phosphor-psu-monitor

## Current State

Currently, **phosphor-psu-monitor** operates on a **single chassis**.

- It gets the PSUs device access information (such as PSU I2CBus and I2CAddress)
  and allowed PSU configurations using interfaces hosted by **entity manager**
- The phosphor-psu-monitor uses a **fixed object path**, appended with the PSU
  number, to access **inventory manager**.
- Example of current PSU path:

```text
/xyz/openbmc_project/inventory/system/chassis/motherboard/powersupply0

```

## Goal: Support Multi-Chassis PSUs

To support **multi-chassis**, the **phosphor-psu-monitor** requires the
following changes:

- Define Chassis class
- Modify the **PSUManager** class to instantiate one **Chassis class** per
  physical chassis.
- Find all PSUs associated with each chassis.
- Update the PSUs status on DBus with appropriate path i.e.

```text
/xyz/openbmc_project/inventory/system/chassis1/motherboard/powersupply1
```

## Changes Required

### 1. Define Chassis Class

- Chassis class constructor take parameters D-Bus connection object, chassis
  number, and object path
- Vector of powerSupply instances
- Functions to get and add PSUs
- Function to get chassis status

### 2. Modify PSUManager Class

- Add method to retrieve a list of chassis objects from the **inventory
  manager** and store them in a vector of chassis. Each chassis object listens
  for D-BUS events to track chassis PGOOD Status.
- Retrieve the PSU configurations from **entity manager**
- Find all PSUs associated with chassis from **inventory manager** (using
  [`getSubTree`](https://github.com/openbmc/phosphor-power/blob/master/utility.hpp))
  and add them to vector in the chassis object. **getSubTree** is a function
  interacts with the D-Bus object mapper to retrieve all objects under a
  specified path that implement interface to certain depth.

### 3. Update Monitoring Logic

- The PSUManager object loops over all **chassis** that are present and have
  standby power. The **phosphor-psu-monitor** app monitors all PSUs associated
  with the chassis to:
  - Monitor Presence
  - Create appropriate journal entries
  - Create PELs for faults if the chassis is powered on

### 4. DBus Updates

- Update/publish **PSU sensors, presence and VPD information** on D-Bus.
- Ensure properties **reflect multi-chassis** support.

Note: The phosphor-psu-monitor logic should remain the same when adding support
for multi chassis.

### 5. PowerSystemInputs Interface

- TBD

### 6. Error Handling

- Ensure any **error** references the **correct PSU inventory path**, e.g.,
  - `/xyz/openbmc_project/inventory/system/chassis0/powersupply0`
  - `/xyz/openbmc_project/inventory/system/chassis1/powersupply3`

### 7. Log Messages

- Update journal messages to reflect **appropriate chassis**.

---

## Summary

**phosphor-psu-monitor** must be enhanced to dynamically:

- Discover chassis, and maintain state of each chassis and react accordingly.
- Monitor PSUs across all chassis
- Update DBus and error logs accordingly.
