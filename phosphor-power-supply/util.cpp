#include "util.hpp"

#include <gpiod.hpp>

namespace phosphor::power::psu
{

const UtilBase& getUtils()
{
    static Util util;
    return util;
}

GPIOInterface::GPIOInterface(const std::string& namedGpio)
{
    try
    {
        line = gpiod::find_line(namedGpio);
        if (!line)
        {
            throw std::runtime_error("Line does not exist: " + namedGpio);
        }
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error(
            std::string("Failed to find line: ") + e.what());
    }
}

std::unique_ptr<GPIOInterfaceBase> GPIOInterface::createGPIO(
    const std::string& namedGpio)
{
    return std::make_unique<GPIOInterface>(namedGpio);
}

std::string GPIOInterface::getName() const
{
    return line.name();
}

int GPIOInterface::read()
{
    int value = -1;

    if (!line)
    {
        lg2::error("Failed line in GPIOInterface::read()");
        throw std::runtime_error{std::string{"Failed to find line"}};
    }

    try
    {
        line.request({__FUNCTION__, gpiod::line_request::DIRECTION_INPUT,
                      gpiod::line_request::FLAG_ACTIVE_LOW});
        try
        {
            value = line.get_value();
        }
        catch (const std::exception& e)
        {
            lg2::error("Failed to get_value of GPIO line: {ERROR}", "ERROR", e);
            line.release();
            throw;
        }

        line.release();
    }
    catch (const std::exception& e)
    {
        lg2::error("Failed to request GPIO line: {MSG}", "MSG", e);
        throw;
    }

    return value;
}

void GPIOInterface::write(int value, std::bitset<32> flags)
{
    if (!line)
    {
        lg2::error("Failed line in GPIOInterface::write");
        throw std::runtime_error{std::string{"Failed to find line"}};
    }

    try
    {
        line.request({__FUNCTION__, gpiod::line_request::DIRECTION_OUTPUT,
                      flags},
                     value);

        line.release();
    }
    catch (std::exception& e)
    {
        lg2::error("Failed to set GPIO line, MSG={MSG}, VALUE={VALUE}", "MSG",
                   e, "VALUE", value);
        throw;
    }
}

void GPIOInterface::toggleLowHigh(const std::chrono::milliseconds& delay)
{
    auto flags = gpiod::line_request::FLAG_OPEN_DRAIN;
    write(0, flags);
    std::this_thread::sleep_for(delay);
    write(1, flags);
}

std::unique_ptr<GPIOInterfaceBase> createGPIO(const std::string& namedGpio)
{
    return GPIOInterface::createGPIO(namedGpio);
}

} // namespace phosphor::power::psu
