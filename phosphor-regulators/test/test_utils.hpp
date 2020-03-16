/**
 * Copyright © 2020 IBM Corporation
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
#include "i2c_interface.hpp"

#include <memory>

namespace phosphor::power::regulators::test_utils
{

/**
 * Create an I2CInterface object with hard-coded bus and address values.
 *
 * @return I2CInterface object wrapped in a unique_ptr
 */
inline std::unique_ptr<i2c::I2CInterface> createI2CInterface()
{
    return i2c::create(1, 0x70, i2c::I2CInterface::InitialState::CLOSED);
}

} // namespace phosphor::power::regulators::test_utils
