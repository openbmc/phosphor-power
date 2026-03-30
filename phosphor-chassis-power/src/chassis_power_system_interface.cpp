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

#include "chassis_power_system_interface.hpp"

namespace phosphor::power::chassis
{

ChassisPowerSystemInterface::ChassisPowerSystemInterface(
    sdbusplus::bus_t& bus, const char* path,
    PowerSystemInputs::Status initialStatus) :
    PowerSystemInputsObject{bus, path,
                            PowerSystemInputsObject::action::defer_emit}
{
    bool skipSignal{true};
    status(initialStatus, skipSignal);
    emit_object_added();
}

} // namespace phosphor::power::chassis
