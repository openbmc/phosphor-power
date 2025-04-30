# Multi-Chassis Support for phosphor-psu-monitor

## Current State

Currently, **phosphor-psu-monitor** operates on a **single chassis**. The 
**phosphor-psu-monitor** uses the **entity manager** with fixed path associated
with single chassis.

## Goal: Support Multi-Chassis PSUs

To support **multi-chassis PSUs**, the **phosphor-psu-monitor** requires the following change: 
- Find all available chassis in the system. 
- Instantiate PSUManager object for each chassis
- Update status of each PSU associated with the chassis on DBus.

## Changes Required

### 1. Dynamic Chassis Discovery

- Retrieve all available **chassis** from **inventory manager**  with the assumption of the path defined as follow:
  ```
  /xyz/openbmc_project/inventory/system/chassis1
  /xyz/openbmc_project/inventory/system/chassis2
  ... 
  ```

### 2. Iterate Through Vector of Chassis

  - Use getSubTree to find powersupply object connected to the chassis (as 
    **children**)
  - Replace **hardcoded PSU paths** 
    ```
    /xyz/openbmc_project/inventory/system/chassis/motherboard/powersupply
    ```
        
    with appropriate **chassis**
    ```
    /xyz/openbmc_project/inventory/system/chassis2/motherboard/powersupply
    ```

### 3. Update The PSU Monitoring Logic

- Modify the **PSU** path with the new **path** and handle PSU status:
  - Faults
  - Presence
  - Event logs

### 4. DBus Updates

- Update/publish **PSU paths** on DBus.
- Ensure properties **reflect multi-chassis support**.

### 5. Error Handling

- Ensure any **error** references the **correct PSU inventory path**, e.g.,
  - `/xyz/openbmc_project/inventory/system/chassis1/powersupply0`
  - `/xyz/openbmc_project/inventory/system/chassis1/powersupply3`

### 6. Log Messages

- Update log messages to reflect **appropriate chassis**.

# Summary

**phosphor-psu-monitor** must be enhanced to dynamically:
- Discover chassis
- Monitor PSUs across all chassis
- Update DBus and error logs accordingly.
