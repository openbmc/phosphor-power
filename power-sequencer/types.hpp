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

constexpr auto gpiNumField = 0;
constexpr auto pinIDField = 1;
constexpr auto gpiNameField = 2;
constexpr auto pollField = 3;

using GPIConfig = std::tuple<size_t, size_t, std::string, bool>;

using GPIConfigs = std::vector<GPIConfig>;

using RailNames = std::vector<std::string>;

constexpr auto pathField = 0;
constexpr auto railNamesField = 1;
constexpr auto gpiConfigField = 2;

using DeviceDefinition = std::tuple<std::string, RailNames, GPIConfigs>;

//Maps a device instance to its definition
using DeviceMap = std::map<size_t, DeviceDefinition>;

}
}
}
