/**
 * Copyright © 2017 IBM Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "ucd90160.hpp"

//Separated out to facilitate possible future generation of file.

namespace witherspoon
{
namespace power
{

using namespace ucd90160;
using namespace std::string_literals;

const DeviceMap UCD90160::deviceMap
{
    {0, DeviceDefinition{
            "/sys/devices/platform/ahb/ahb:apb/ahb:apb:i2c@1e78a000/"
                "1e78a400.i2c-bus/i2c-11/11-0064",

            RailNames{
                "5.0VCS"s,
                "12.0V"s,
                "3.3V"s,
                "1.8V"s,
                "1.1V"s,
                "1.0V"s,
                "0.9V"s,
                "VDN-A"s,
                "VDN-B"s,
                "AVDD"s,
                "VIO-A"s,
                "VIO-B"s,
                "VDD-A"s,
                "VDD-B"s,
                "VCS-A"s,
                "VCS-B"s
            },

            GPIConfigs{
                GPIConfig{1, 8,  "PGOOD_5P0V"s, false,
                          extraAnalysisType::none},
                GPIConfig{2, 9,  "MEM_GOOD0"s, false,
                          extraAnalysisType::none},
                GPIConfig{3, 10, "MEM_GOOD1"s, false,
                          extraAnalysisType::none},
                GPIConfig{4, 14, "GPU_PGOOD"s, false,
                          extraAnalysisType::gpuPGOOD},
                GPIConfig{5, 17, "GPU_TH_OVERT"s, true,
                          extraAnalysisType::gpuOverTemp},
                GPIConfig{6, 11, "SOFTWARE_PGOOD"s, false,
                          extraAnalysisType::none}
            },

            GPIOAnalysis{
                {extraAnalysisType::gpuPGOOD,
                    GPIOGroup{
                        "/sys/devices/platform/ahb/ahb:apb/ahb:apb:i2c@"
                        "1e78a000/1e78a400.i2c-bus/i2c-11/11-0060",
                        gpio::Value::low,
                        [](auto& ucd, const auto& callout) { ucd.gpuPGOODError(callout); },
                        optionFlags::none,
                        GPIODefinitions{
                            GPIODefinition{8, "GPU0"s},
                            GPIODefinition{9, "GPU1"s},
                            GPIODefinition{10, "GPU2"s},
                            GPIODefinition{11, "GPU3"s},
                            GPIODefinition{12, "GPU4"s},
                            GPIODefinition{13, "GPU5"s}}
                    }},

                {extraAnalysisType::gpuOverTemp,
                    GPIOGroup{
                        "/sys/devices/platform/ahb/ahb:apb/ahb:apb:i2c@"
                        "1e78a000/1e78a400.i2c-bus/i2c-11/11-0060",
                        gpio::Value::low,
                        [](auto& ucd, const auto& callout) { ucd.gpuOverTempError(callout); },
                        optionFlags::shutdownOnFault,
                        GPIODefinitions{
                            GPIODefinition{2, "GPU0"s},
                            GPIODefinition{3, "GPU1"s},
                            GPIODefinition{4, "GPU2"s},
                            GPIODefinition{5, "GPU3"s},
                            GPIODefinition{6, "GPU4"s},
                            GPIODefinition{7, "GPU5"s}}
                    }}
            }
        }
    }
};

}
}
