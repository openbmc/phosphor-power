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

#include "vpd_service.hpp"

namespace phosphor::power::regulators
{

std::string DBusVPDService::getValue(const std::string& /*inventoryPath*/,
                                     const std::string& /*keyword*/)
{
    // TODO: Need to add implementation.  Should check cache; if available
    // return that.  Otherwise get value from DBus and store in cache.
    return "";
}

} // namespace phosphor::power::regulators
