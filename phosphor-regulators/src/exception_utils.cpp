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

#include "exception_utils.hpp"

namespace phosphor::power::regulators::exception_utils
{

std::vector<std::string> getMessages(const std::exception& e)
{
    std::vector<std::string> messages{};
    internal::getMessages(e, messages);
    return messages;
}

namespace internal
{

void getMessages(const std::exception& e, std::vector<std::string>& messages)
{
    // If this exception is nested, get messages from inner exception(s)
    try
    {
        std::rethrow_if_nested(e);
    }
    catch (const std::exception& inner)
    {
        getMessages(inner, messages);
    }
    catch (...)
    {
    }

    // Append error message from this exception
    messages.emplace_back(e.what());
}

} // namespace internal

} // namespace phosphor::power::regulators::exception_utils
