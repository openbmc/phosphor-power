#include "util.hpp"

namespace phosphor::power::psu
{

const UtilBase& getUtils()
{
    static Util util;
    return util;
}

} // namespace phosphor::power::psu
