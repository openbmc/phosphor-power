#include "util.hpp"

#include <gpiod.hpp>

namespace phosphor::power::psu
{

const UtilBase& getUtils()
{
    static Util util;
    return util;
}

GPIOReader::GPIOReader(const std::string& namedGpio)
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
        throw std::runtime_error(std::string("Failed to find line: ") +
                                 e.what());
    }
}

std::unique_ptr<GPIOInterface>
    GPIOReader::createGPIO(const std::string& namedGpio)
{
    return std::make_unique<GPIOReader>(namedGpio);
}

std::string GPIOReader::getName() const
{
    return line.name();
}

int GPIOReader::read()
{
    using namespace phosphor::logging;

    int value = -1;

    if (!line)
    {
        log<level::ERR>("Failed line");
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
            log<level::ERR>(
                fmt::format("Failed to get_value of GPIO line: {}", e.what())
                    .c_str());
            line.release();
            throw;
        }

        line.release();
    }
    catch (const std::exception& e)
    {
        log<level::ERR>("Failed to request GPIO line",
                        entry("MSG=%s", e.what()));
        throw;
    }

    return value;
}

void GPIOReader::write(int value, std::bitset<32> flags)
{
    using namespace phosphor::logging;

    if (!line)
    {
        log<level::ERR>("Failed line");
        throw std::runtime_error{std::string{"Failed to find line"}};
    }

    try
    {
        line.request(
            {__FUNCTION__, gpiod::line_request::DIRECTION_OUTPUT, flags},
            value);

        line.release();
    }
    catch (std::exception& e)
    {
        log<level::ERR>("Failed to set GPIO line", entry("MSG=%s", e.what()),
                        entry("VALUE=%d", value));
        throw;
    }
}

std::unique_ptr<GPIOInterface> createGPIO(const std::string& namedGpio)
{
    return GPIOReader::createGPIO(namedGpio);
}

} // namespace phosphor::power::psu
