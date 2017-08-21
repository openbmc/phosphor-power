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
                "1e78a400.i2c-bus/i2c-11/11-0064"}
    }
};

}
}
