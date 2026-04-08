/**
 * Copyright © 2026 IBM Corporation
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

#include "services.hpp"

namespace phosphor::power::chassis
{

std::unique_ptr<GpioInterface> BMCServices::createGPIO(
    const std::string& name, GpioDirection direction, GpioPolarity polarity,
    std::optional<uint8_t> defaultValue)
{
    auto gpio = std::make_unique<Gpio>(name, direction, polarity, defaultValue);

    // Attempt findLine, ignore return as line can be requested later
    gpio->findLine();

    return gpio;
}

} // namespace phosphor::power::chassis
