#pragma once

namespace phosphor
{
namespace power
{
namespace psu
{

/**
 * @class PowerSupply
 * Represents a PMBus power supply.
 */
class PowerSupply
{
    public:
        PowerSupply() = delete;
        PowerSupply(const PowerSupply&) = delete;
        PowerSupply& operator=(const PowerSupply&) = delete;
        PowerSupply(PowerSupply&&) = default;
        PowerSupply& operator=(PowerSupply&&) = default;
        ~PowerSupply() = default;

};

}
}
}
