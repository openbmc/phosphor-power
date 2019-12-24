/*
// Copyright (c) 2019 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#include "utility.hpp"

#include <boost/algorithm/string/predicate.hpp>
#include <phosphor-logging/elog-errors.hpp>

namespace CR
{

void getPSUEvent(const std::shared_ptr<sdbusplus::asio::connection>& conn,
                 const std::string& psuName, PSUState& state)
{
    state = PSUState::normal;
    // /State/Decorator/PSUx_OperationalStatus
    std::string pathStr = psuEventPath + psuName + "_OperationalStatus";

    conn->async_method_call(
        [&conn, &state](const boost::system::error_code ec,
                        const bool& result) {
            if (ec)
            {
                std::cerr << "Exception happened when get "
                             "functional property\n";
                return;
            }

            if (!result)
            {
                state = PSUState::acLost;
            }
            return;
        },
        "xyz.openbmc_project.PSUSensor", pathStr.c_str(),
        "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.State.Decorator.OperationalStatus", "functional");
}

} // namespace CR
