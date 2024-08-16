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

#include "ffdc_file.hpp"

#include <errno.h>     // for errno
#include <fcntl.h>     // for open()
#include <string.h>    // for strerror()
#include <sys/stat.h>  // for open()
#include <sys/types.h> // for open()

#include <stdexcept>
#include <string>

namespace phosphor::power::regulators
{

FFDCFile::FFDCFile(FFDCFormat format, uint8_t subType, uint8_t version) :
    format{format}, subType{subType}, version{version}
{
    // Open the temporary file for both reading and writing
    int fd = open(tempFile.getPath().c_str(), O_RDWR);
    if (fd == -1)
    {
        throw std::runtime_error{
            std::string{"Unable to open FFDC file: "} + strerror(errno)};
    }

    // Store file descriptor in FileDescriptor object
    descriptor.set(fd);
}

void FFDCFile::remove()
{
    // Close file descriptor.  Does nothing if descriptor was already closed.
    // Returns -1 if close failed.
    if (descriptor.close() == -1)
    {
        throw std::runtime_error{
            std::string{"Unable to close FFDC file: "} + strerror(errno)};
    }

    // Delete temporary file.  Does nothing if file was already deleted.
    // Throws an exception if the deletion failed.
    tempFile.remove();
}

} // namespace phosphor::power::regulators
