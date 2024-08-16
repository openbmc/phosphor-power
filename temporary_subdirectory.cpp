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

#include "temporary_subdirectory.hpp"

#include <errno.h>  // for errno
#include <stdlib.h> // for mkdtemp()
#include <string.h> // for strerror()

#include <stdexcept>
#include <string>

namespace phosphor::power::util
{

namespace fs = std::filesystem;

TemporarySubDirectory::TemporarySubDirectory()
{
    // Build template path required by mkdtemp()
    std::string templatePath =
        fs::temp_directory_path() / "phosphor-power-XXXXXX";

    // Generate unique subdirectory name and create it.  The XXXXXX characters
    // are replaced by mkdtemp() to make the subdirectory name unique.
    char* retVal = mkdtemp(templatePath.data());
    if (retVal == nullptr)
    {
        throw std::runtime_error{
            std::string{"Unable to create temporary subdirectory: "} +
            strerror(errno)};
    }

    // Store path to temporary subdirectory
    path = templatePath;
}

TemporarySubDirectory&
    TemporarySubDirectory::operator=(TemporarySubDirectory&& subdirectory)
{
    // Verify not assigning object to itself (a = std::move(a))
    if (this != &subdirectory)
    {
        // Delete temporary subdirectory owned by this object
        remove();

        // Move subdirectory path from other object, transferring ownership
        path = std::move(subdirectory.path);

        // Clear path in other object; after move path is in unspecified state
        subdirectory.path.clear();
    }
    return *this;
}

void TemporarySubDirectory::remove()
{
    if (!path.empty())
    {
        // Delete temporary subdirectory from file system
        fs::remove_all(path);

        // Clear path to indicate subdirectory has been deleted
        path.clear();
    }
}

} // namespace phosphor::power::util
