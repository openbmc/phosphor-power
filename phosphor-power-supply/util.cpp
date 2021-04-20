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
    line = gpiod::find_line(namedGpio);
}

std::unique_ptr<GPIOInterface>
    GPIOReader::createGPIO(const std::string& namedGpio)
{
    return std::make_unique<GPIOReader>(namedGpio);
}

int GPIOReader::read()
{
    using namespace phosphor::logging;

    int value = -1;

    if (!line)
    {
        log<level::ERR>("Failed line");
        return -1;
    }

    try
    {
        line.request({__FUNCTION__, gpiod::line_request::DIRECTION_INPUT,
                      gpiod::line_request::FLAG_ACTIVE_LOW});
        try
        {
            value = line.get_value();
            // TODO: Toy throw to test things work?
            // throw std::invalid_argument{"fake get_value() failure"};
        }
        catch (std::exception& e)
        {
            log<level::ERR>(
                fmt::format("Failed to get_value of GPIO line: {}", e.what())
                    .c_str());
            throw;
        }

        log<level::DEBUG>("release() line");
        line.release();
    }
    catch (std::exception& e)
    {
        log<level::ERR>("Failed to request GPIO line",
                        entry("MSG=%s", e.what()));
        throw;
    }

    return value;
}

std::unique_ptr<GPIOInterface> createGPIO(const std::string& namedGpio)
{
    return GPIOReader::createGPIO(namedGpio);
}

} // namespace phosphor::power::psu
