# Multiple Chassis

## Overview

A BMC-based system can contain one or more chassis. A chassis is typically a
physical enclosure that contains system components such as CPUs, fans, power
supplies, and PCIe cards.

A chassis can be stand-alone, such as a tower or desktop. A chassis can also be
designed to be mounted in an equipment rack.

For the `phosphor-power-sequencer` application, the term "single chassis system"
means the system type has a maximum configuration of one chassis. If the system
type has a maximum configuration of multiple chassis, then it is considered a
"multiple chassis system" even if the current system only contains one chassis.

## Differences between single and multiple chassis systems

### System and chassis power state

In a single chassis system, the system and chassis power state are identical.
Both are either on or off.

In a multiple chassis system, each chassis has its own power state. Even if the
system is on, an individual chassis may be off. This can occur if that chassis
does not have input power or has a critical hardware problem.

See [Chassis Status](chassis_status.md) for more information.

### Chassis power good faults

If a single chassis system experiences a power good (pgood) fault, the system is
powered off.

If a multiple chassis system experiences a power good (pgood) fault in one
chassis, the system will be powered off and then powered on again without that
chassis.

See [Power Good Faults](pgood_faults.md) for more information.

### Brownouts

If a single chassis system that was powered on experiences a brownout, the
system will be powered off.

If a multiple chassis system that was powered on experiences a brownout in one
chassis, the system will be powered off and then powered on again without that
chassis.

See [Power Loss](power_loss.md) for more information.

### Blackouts

If a single chassis system that was powered on experiences a blackout, the
system loses all power. It will be completely off until utility power is
restored. It may then automatically power on again depending on the Auto Power
Restart settings.

If a multiple chassis system that was powered on experiences a blackout in one
chassis, that chassis loses all power. The system will be powered off and then
powered on again without that chassis.

See [Power Loss](power_loss.md) for more information.
