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

#include "temporary_file.hpp"

#include <errno.h>  // for errno
#include <stdlib.h> // for mkstemp()
#include <string.h> // for strerror()
#include <unistd.h> // for close()

#include <stdexcept>
#include <string>

namespace phosphor::power::util
{

namespace fs = std::filesystem;

TemporaryFile::TemporaryFile()
{
    // Build template path required by mkstemp()
    std::string templatePath =
        fs::temp_directory_path() / "phosphor-power-XXXXXX";

    // Generate unique file name, create file, and open it.  The XXXXXX
    // characters are replaced by mkstemp() to make the file name unique.
    int fd = mkstemp(templatePath.data());
    if (fd == -1)
    {
        throw std::runtime_error{
            std::string{"Unable to create temporary file: "} + strerror(errno)};
    }

    // Store path to temporary file
    path = templatePath;

    // Close file descriptor
    if (close(fd) == -1)
    {
        // Save errno value; will likely change when we delete temporary file
        int savedErrno = errno;

        // Delete temporary file.  The destructor won't be called because the
        // exception below causes this constructor to exit without completing.
        remove();

        throw std::runtime_error{
            std::string{"Unable to close temporary file: "} +
            strerror(savedErrno)};
    }
}

TemporaryFile& TemporaryFile::operator=(TemporaryFile&& file)
{
    // Verify not assigning object to itself (a = std::move(a))
    if (this != &file)
    {
        // Delete temporary file owned by this object
        remove();

        // Move temporary file path from other object, transferring ownership
        path = std::move(file.path);

        // Clear path in other object; after move path is in unspecified state
        file.path.clear();
    }
    return *this;
}

void TemporaryFile::remove()
{
    if (!path.empty())
    {
        // Delete temporary file from file system
        fs::remove(path);

        // Clear path to indicate file has been deleted
        path.clear();
    }
}

} // namespace phosphor::power::util
