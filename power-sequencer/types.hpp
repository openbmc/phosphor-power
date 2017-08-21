#pragma once

#include <map>
#include <string>
#include <tuple>
#include <vector>

namespace witherspoon
{
namespace power
{
namespace ucd90160
{

using RailNames = std::vector<std::string>;

constexpr auto pathField = 0;
constexpr auto railNamesField = 1;

using DeviceDefinition = std::tuple<std::string, RailNames>;

//Maps a device instance to its definition
using DeviceMap = std::map<size_t, DeviceDefinition>;

}
}
}
