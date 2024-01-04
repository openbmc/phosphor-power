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

#include <filesystem>
#include <utility>

namespace phosphor::power::util
{

/**
 * @class TemporaryFile
 *
 * A temporary file in the file system.
 *
 * The temporary file is created by the constructor.  The absolute path to the
 * file can be obtained using getPath().
 *
 * The temporary file can be deleted by calling remove().  Otherwise the file
 * will be deleted by the destructor.
 *
 * TemporaryFile objects cannot be copied, but they can be moved.  This enables
 * them to be stored in containers like std::vector.
 */
class TemporaryFile
{
  public:
    // Specify which compiler-generated methods we want
    TemporaryFile(const TemporaryFile&) = delete;
    TemporaryFile& operator=(const TemporaryFile&) = delete;

    /**
     * Constructor.
     *
     * Creates a temporary file in the temporary directory (normally /tmp).
     *
     * Throws an exception if the file cannot be created.
     */
    TemporaryFile();

    /**
     * Move constructor.
     *
     * Transfers ownership of a temporary file.
     *
     * @param file TemporaryFile object being moved
     */
    TemporaryFile(TemporaryFile&& file) : path{std::move(file.path)}
    {
        // Clear path in other object; after move path is in unspecified state
        file.path.clear();
    }

    /**
     * Move assignment operator.
     *
     * Deletes the temporary file owned by this object.  Then transfers
     * ownership of the temporary file owned by the other object.
     *
     * Throws an exception if an error occurs during the deletion.
     *
     * @param file TemporaryFile object being moved
     */
    TemporaryFile& operator=(TemporaryFile&& file);

    /**
     * Destructor.
     *
     * Deletes the temporary file if necessary.
     */
    ~TemporaryFile()
    {
        try
        {
            remove();
        }
        catch (...)
        {
            // Destructors should not throw exceptions
        }
    }

    /**
     * Deletes the temporary file.
     *
     * Does nothing if the file has already been deleted.
     *
     * Throws an exception if an error occurs during the deletion.
     */
    void remove();

    /**
     * Returns the absolute path to the temporary file.
     *
     * Returns an empty path if the file has been deleted.
     *
     * @return temporary file path
     */
    const std::filesystem::path& getPath() const
    {
        return path;
    }

  private:
    /**
     * Absolute path to the temporary file.
     *
     * Empty when file has been deleted.
     */
    std::filesystem::path path{};
};

} // namespace phosphor::power::util
