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

#include <filesystem>
#include <utility>

namespace phosphor::power::util
{

/**
 * @class TemporarySubDirectory
 *
 * A temporary subdirectory in the file system.
 *
 * This class does NOT represent the system temporary directory (such as /tmp).
 * It represents a temporary subdirectory below that directory.
 *
 * The temporary subdirectory is created by the constructor.  The absolute path
 * to the subdirectory can be obtained using getPath().
 *
 * The temporary subdirectory can be deleted by calling remove().  Otherwise the
 * subdirectory will be deleted by the destructor.
 *
 * TemporarySubDirectory objects cannot be copied, but they can be moved.  This
 * enables them to be stored in containers like std::vector.
 */
class TemporarySubDirectory
{
  public:
    // Specify which compiler-generated methods we want
    TemporarySubDirectory(const TemporarySubDirectory&) = delete;
    TemporarySubDirectory& operator=(const TemporarySubDirectory&) = delete;

    /**
     * Constructor.
     *
     * Creates a temporary subdirectory below the system temporary directory
     * (such as /tmp).
     *
     * Throws an exception if the subdirectory cannot be created.
     */
    TemporarySubDirectory();

    /**
     * Move constructor.
     *
     * Transfers ownership of a temporary subdirectory.
     *
     * @param subdirectory TemporarySubDirectory object being moved
     */
    TemporarySubDirectory(TemporarySubDirectory&& subdirectory) :
        path{std::move(subdirectory.path)}
    {
        // Clear path in other object; after move path is in unspecified state
        subdirectory.path.clear();
    }

    /**
     * Move assignment operator.
     *
     * Deletes the temporary subdirectory owned by this object.  Then transfers
     * ownership of the temporary subdirectory owned by the other object.
     *
     * Throws an exception if an error occurs during the deletion.
     *
     * @param subdirectory TemporarySubDirectory object being moved
     */
    TemporarySubDirectory& operator=(TemporarySubDirectory&& subdirectory);

    /**
     * Destructor.
     *
     * Deletes the temporary subdirectory if necessary.
     */
    ~TemporarySubDirectory()
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
     * Deletes the temporary subdirectory.
     *
     * Does nothing if the subdirectory has already been deleted.
     *
     * Throws an exception if an error occurs during the deletion.
     */
    void remove();

    /**
     * Returns the absolute path to the temporary subdirectory.
     *
     * Returns an empty path if the subdirectory has been deleted.
     *
     * @return temporary subdirectory path
     */
    const std::filesystem::path& getPath() const
    {
        return path;
    }

  private:
    /**
     * Absolute path to the temporary subdirectory.
     *
     * Empty when subdirectory has been deleted.
     */
    std::filesystem::path path{};
};

} // namespace phosphor::power::util
