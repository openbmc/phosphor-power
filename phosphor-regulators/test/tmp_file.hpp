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
#pragma once

#include <errno.h>  // for errno
#include <stdio.h>  // for perror()
#include <stdlib.h> // for mkstemp()
#include <string.h> // for strerror()
#include <unistd.h> // for close(), unlink()

#include <stdexcept>
#include <string>

namespace phosphor::power::regulators
{

/**
 * @class TmpFile
 *
 * Temporary file.
 *
 * File is created by the constructor and deleted by the destructor.  The file
 * name can be obtained from getName().
 */
class TmpFile
{
  public:
    // Specify which compiler-generated methods we want
    TmpFile(const TmpFile&) = delete;
    TmpFile(TmpFile&&) = delete;
    TmpFile& operator=(const TmpFile&) = delete;
    TmpFile& operator=(TmpFile&&) = delete;

    /**
     * Constructor.
     *
     * Creates the temporary file.
     */
    TmpFile()
    {
        // Generate unique file name, create file, and open it.  The XXXXXX
        // characters are replaced by mkstemp() to make the file name unique.
        char fileNameTemplate[17] = "/tmp/temp-XXXXXX";
        int fd = mkstemp(fileNameTemplate);
        if (fd == -1)
        {
            // If mkstemp() fails, throw an exception.  No temporary file has
            // been created and calling getName() would not work.
            throw std::runtime_error{"Unable to create temporary file: " +
                                     std::string{strerror(errno)}};
        }

        // Close file
        if (close(fd) == -1)
        {
            // If close() fails, write a message to standard error but do not
            // throw an exception.  If an exception is thrown, the destructor
            // will not be run and the temporary file will not be deleted.
            perror("Unable to close temporary file");
        }

        // Save file name
        fileName = fileNameTemplate;
    }

    const std::string& getName()
    {
        return fileName;
    }

    ~TmpFile()
    {
        // Delete temporary file
        if (unlink(fileName.c_str()) == -1)
        {
            // If unlink() fails, write a message to standard error but do not
            // throw an exception.  Destructors must not throw exceptions.
            perror("Unable to delete temporary file");
        }
    }

  private:
    std::string fileName{};
};

} // namespace phosphor::power::regulators
