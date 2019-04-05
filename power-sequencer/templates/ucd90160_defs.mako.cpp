/* This is a generated file. */

#include "ucd90160.hpp"

namespace witherspoon
{
namespace power
{

using namespace ucd90160;
using namespace std::string_literals;

const DeviceMap UCD90160::deviceMap{
%for ucd_data in ucd90160s:
    {${ucd_data['index']},
     DeviceDefinition{
         "${ucd_data['path']}",

         RailNames{"5.0VCS"s, "12.0V"s, "3.3V"s, "1.8V"s, "1.1V"s, "1.0V"s,
                   "0.9V"s, "VDN-A"s, "VDN-B"s, "AVDD"s, "VIO-A"s, "VIO-B"s,
                   "VDD-A"s, "VDD-B"s, "VCS-A"s, "VCS-B"s},

         GPIConfigs{
             GPIConfig{1, 8, "PGOOD_5P0V"s, false, extraAnalysisType::none},
             GPIConfig{2, 9, "MEM_GOOD0"s, false, extraAnalysisType::none},
             GPIConfig{3, 10, "MEM_GOOD1"s, false, extraAnalysisType::none},
             GPIConfig{4, 14, "GPU_PGOOD"s, false, extraAnalysisType::gpuPGOOD},
             GPIConfig{5, 17, "GPU_TH_OVERT"s, true,
                       extraAnalysisType::gpuOverTemp},
             GPIConfig{6, 11, "SOFTWARE_PGOOD"s, false,
                       extraAnalysisType::none}},

         GPIOAnalysis{
             {extraAnalysisType::gpuPGOOD,
              GPIOGroup{
                  "/sys/devices/platform/ahb/ahb:apb/ahb:apb:bus@"
                  "1e78a000/1e78a400.i2c-bus/i2c-11/11-0060",
                  gpio::Value::low,
                  [](auto& ucd, const auto& callout) {
                      ucd.gpuPGOODError(callout);
                  },
                  optionFlags::none,
                  GPIODefinitions{
                      GPIODefinition{8,
                                     "/system/chassis/motherboard/gv100card0"s},
                      GPIODefinition{9,
                                     "/system/chassis/motherboard/gv100card1"s},
                      GPIODefinition{10,
                                     "/system/chassis/motherboard/gv100card2"s},
                      GPIODefinition{11,
                                     "/system/chassis/motherboard/gv100card3"s},
                      GPIODefinition{12,
                                     "/system/chassis/motherboard/gv100card4"s},
                      GPIODefinition{
                          13, "/system/chassis/motherboard/gv100card5"s}}}},

             {extraAnalysisType::gpuOverTemp,
              GPIOGroup{
                  "/sys/devices/platform/ahb/ahb:apb/ahb:apb:bus@"
                  "1e78a000/1e78a400.i2c-bus/i2c-11/11-0060",
                  gpio::Value::low,
                  [](auto& ucd,
                     const auto& callout) { ucd.gpuOverTempError(callout); },
                  optionFlags::shutdownOnFault,
                  GPIODefinitions{
                      GPIODefinition{2,
                                     "/system/chassis/motherboard/gv100card0"s},
                      GPIODefinition{3,
                                     "/system/chassis/motherboard/gv100card1"s},
                      GPIODefinition{4,
                                     "/system/chassis/motherboard/gv100card2"s},
                      GPIODefinition{5,
                                     "/system/chassis/motherboard/gv100card3"s},
                      GPIODefinition{6,
                                     "/system/chassis/motherboard/gv100card4"s},
                      GPIODefinition{
                          7, "/system/chassis/motherboard/gv100card5"s}}}}}}
    },
%endfor
};

} // namespace power
} // namespace witherspoon
