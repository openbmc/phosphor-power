#pragma once

#include <map>
#include <string>
#include <tuple>

namespace witherspoon
{
namespace power
{
namespace ucd90160
{

constexpr auto pathField = 0;

//Future commits will add more entries
using DeviceDefinition = std::tuple<std::string>;

//Maps a device instance to its definition
using DeviceMap = std::map<size_t, DeviceDefinition>;

}
}
}
