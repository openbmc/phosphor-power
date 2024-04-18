/**
 * Copyright © 2024 IBM Corporation
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
#pragma once

#include <format>
#include <iterator>
#include <span>
#include <string>

/**
 * @namespace format_utils
 *
 * Contains utility functions for formatting data.
 */
namespace phosphor::power::sequencer::format_utils
{

/**
 * Returns a string containing the elements in the specified container span.
 *
 * The string starts with "[", ends with "]", and the elements are separated by
 * ", ".  The individual elements are formatted using std::format.
 *
 * This function is a temporary solution until g++ implements the C++23
 * std::format() support for formatting ranges.
 *
 * @param spn span of elements to format within a container
 * @return formatted string containing the specified elements
 */
template <typename T>
std::string toString(const std::span<T>& spn)
{
    std::string str{"["};
    for (auto it = spn.cbegin(); it != spn.cend(); ++it)
    {
        str += std::format("{}", *it);
        if (std::distance(it, spn.cend()) > 1)
        {
            str += ", ";
        }
    }
    str += "]";
    return str;
}

} // namespace phosphor::power::sequencer::format_utils
