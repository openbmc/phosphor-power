# Multi-Chassis Support for phosphor-psu-monitor

## Current State

Currently, **phosphor-psu-monitor** operates on a **single chassis**.

- It gets the PSUs and PSU configuration using interfaces hosted by **entity
  manager**
- The app uses **fixed path** appended with the PSU number to get the object
  path from **inventory manager**
- Example of current PSU path:
  ```
  /xyz/openbmc_project/inventory/system/chassis/motherboard/powersupply
  ```

## Goal: Support Multi-Chassis PSUs

To support **multi-chassis**, the **phosphor-psu-monitor** requires the
following change:

- Define Chassis class
- Modify the **PSUManager** class to instantiate Chassis class
- Find all PSUs associated with the chassis.
- Update the PSUs status on DBus with appropriate path i.e.

```
- /xyz/openbmc_project/inventory/system/chassis1/motherboard/powersupply1
```

## Changes Required

### 1. Define Chassis Class

<pre><code>class Chassis
{
	public:
		Chassis(sdbusplus::bus_t& bus, const std::string& name, const std::string& path);
		void addPowerSupply(const PowerSupply& psu);
		const std::vector<std::unique_ptr<PowerSupply>>& getPowerSupplies() const;

		.....
	private:
		std::vector<std::unique_ptr<PowerSupply>> powerSupplies;
		bool chassisState;
		...
}</pre>

### 2. Modify PSUManager Class

- Add method to retrieve list of chassis objects from **inventory manager**. The
  chassis list stored in vector of chassis.
- Retrieve the PSU configurations from **entity manager**
- Find all PSUs present in chassis (using **GetSubTree**) and Add them to vector in
  the chassis object.

### 3. Update Monitoring Logic

- Loop **per chassis** and handle PSU status:
  - Faults
  - Presence
  - Event logs

### 4. DBus Updates

- Update/publish **PSU sensors** on DBus.
- Ensure properties **reflect multi-chassis** support.

### 5. Error Handling

- Ensure any **error** references the **correct PSU inventory path**, e.g.,
  - `/xyz/openbmc_project/inventory/system/chassis0/powersupply0`
  - `/xyz/openbmc_project/inventory/system/chassis1/powersupply3`

### 6. Log Messages

- Update log messages to reflect **appropriate chassis**.

---

# Summary

**phosphor-psu-monitor** must be enhanced to dynamically:

- Discover chassis
- Monitor PSUs across all chassis
- Update DBus and error logs accordingly.
