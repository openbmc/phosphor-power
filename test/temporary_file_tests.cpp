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

#include <filesystem>
#include <fstream>
#include <string>
#include <utility>

#include <gtest/gtest.h>

using namespace phosphor::power::util;
namespace fs = std::filesystem;

/**
 * Modify the specified file so that fs::remove() fails with an exception.
 *
 * The file will be renamed and can be restored by calling makeFileRemovable().
 *
 * @param path path to the file
 */
inline void makeFileUnRemovable(const fs::path& path)
{
    // Rename the file to save its contents
    fs::path savePath{path.native() + ".save"};
    fs::rename(path, savePath);

    // Create a directory at the original file path
    fs::create_directory(path);

    // Create a file within the directory.  fs::remove() will throw an exception
    // if the path is a non-empty directory.
    std::ofstream childFile{path / "childFile"};
}

/**
 * Modify the specified file so that fs::remove() can successfully delete it.
 *
 * Undo the modifications from an earlier call to makeFileUnRemovable().
 *
 * @param path path to the file
 */
inline void makeFileRemovable(const fs::path& path)
{
    // makeFileUnRemovable() creates a directory at the file path.  Remove the
    // directory and all of its contents.
    fs::remove_all(path);

    // Rename the file back to the original path to restore its contents
    fs::path savePath{path.native() + ".save"};
    fs::rename(savePath, path);
}

TEST(TemporaryFileTests, DefaultConstructor)
{
    TemporaryFile file{};

    fs::path path = file.getPath();
    EXPECT_FALSE(path.empty());
    EXPECT_TRUE(fs::exists(path));
    EXPECT_TRUE(fs::is_regular_file(path));

    fs::path parentDir = path.parent_path();
    EXPECT_EQ(parentDir, "/tmp");

    std::string fileName = path.filename();
    std::string namePrefix = "phosphor-power-";
    EXPECT_EQ(fileName.compare(0, namePrefix.size(), namePrefix), 0);
}

TEST(TemporaryFileTests, MoveConstructor)
{
    // Create first TemporaryFile object and verify temporary file exists
    TemporaryFile file1{};
    EXPECT_FALSE(file1.getPath().empty());
    EXPECT_TRUE(fs::exists(file1.getPath()));

    // Save path to temporary file
    fs::path path = file1.getPath();

    // Create second TemporaryFile object by moving first object
    TemporaryFile file2{std::move(file1)};

    // Verify first object now has an empty path
    EXPECT_TRUE(file1.getPath().empty());

    // Verify second object now owns same temporary file and file exists
    EXPECT_EQ(file2.getPath(), path);
    EXPECT_TRUE(fs::exists(file2.getPath()));
}

TEST(TemporaryFileTests, MoveAssignmentOperator)
{
    // Test where works: TemporaryFile object is moved
    {
        // Create first TemporaryFile object and verify temporary file exists
        TemporaryFile file1{};
        EXPECT_FALSE(file1.getPath().empty());
        EXPECT_TRUE(fs::exists(file1.getPath()));

        // Save path to first temporary file
        fs::path path1 = file1.getPath();

        // Create second TemporaryFile object and verify temporary file exists
        TemporaryFile file2{};
        EXPECT_FALSE(file2.getPath().empty());
        EXPECT_TRUE(fs::exists(file2.getPath()));

        // Save path to second temporary file
        fs::path path2 = file2.getPath();

        // Verify temporary files are different
        EXPECT_NE(path1, path2);

        // Move first object into the second
        file2 = std::move(file1);

        // Verify first object now has an empty path
        EXPECT_TRUE(file1.getPath().empty());

        // Verify second object now owns first temporary file and file exists
        EXPECT_EQ(file2.getPath(), path1);
        EXPECT_TRUE(fs::exists(path1));

        // Verify second temporary file was deleted
        EXPECT_FALSE(fs::exists(path2));
    }

    // Test where does nothing: TemporaryFile object is moved into itself
    {
        // Create TemporaryFile object and verify temporary file exists
        TemporaryFile file{};
        EXPECT_FALSE(file.getPath().empty());
        EXPECT_TRUE(fs::exists(file.getPath()));

        // Save path to temporary file
        fs::path path = file.getPath();

// This is undefined behavior in C++, butsuppress the warning
// to observe how the class handles it
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-move"
#endif
        // Try to move object into itself; should do nothing
        file = static_cast<TemporaryFile&&>(file);
#ifdef __clang__
#pragma clang diagnostic pop
#endif

        // Verify object still owns same temporary file and file exists
        EXPECT_EQ(file.getPath(), path);
        EXPECT_TRUE(fs::exists(path));
    }

    // Test where fails: Cannot delete temporary file
    {
        // Create first TemporaryFile object and verify temporary file exists
        TemporaryFile file1{};
        EXPECT_FALSE(file1.getPath().empty());
        EXPECT_TRUE(fs::exists(file1.getPath()));

        // Save path to first temporary file
        fs::path path1 = file1.getPath();

        // Create second TemporaryFile object and verify temporary file exists
        TemporaryFile file2{};
        EXPECT_FALSE(file2.getPath().empty());
        EXPECT_TRUE(fs::exists(file2.getPath()));

        // Save path to second temporary file
        fs::path path2 = file2.getPath();

        // Verify temporary files are different
        EXPECT_NE(path1, path2);

        // Make second temporary file unremoveable
        makeFileUnRemovable(path2);

        try
        {
            // Try to move first object into the second; should throw exception
            file2 = std::move(file1);
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const std::exception& e)
        {
            // This is expected.  Exception message will vary.
        }

        // Verify first object has not changed and first temporary file exists
        EXPECT_EQ(file1.getPath(), path1);
        EXPECT_TRUE(fs::exists(path1));

        // Verify second object has not changed and second temporary file exists
        EXPECT_EQ(file2.getPath(), path2);
        EXPECT_TRUE(fs::exists(path2));

        // Make second temporary file removeable so destructor can delete it
        makeFileRemovable(path2);
    }
}

TEST(TemporaryFileTests, Destructor)
{
    // Test where works: Temporary file is deleted
    {
        fs::path path{};
        {
            TemporaryFile file{};
            path = file.getPath();
            EXPECT_TRUE(fs::exists(path));
        }
        EXPECT_FALSE(fs::exists(path));
    }

    // Test where works: Temporary file was already deleted
    {
        fs::path path{};
        {
            TemporaryFile file{};
            path = file.getPath();
            EXPECT_TRUE(fs::exists(path));
            file.remove();
            EXPECT_FALSE(fs::exists(path));
        }
        EXPECT_FALSE(fs::exists(path));
    }

    // Test where fails: Cannot delete temporary file: No exception thrown
    {
        fs::path path{};
        try
        {
            TemporaryFile file{};
            path = file.getPath();
            EXPECT_TRUE(fs::exists(path));
            makeFileUnRemovable(path);
        }
        catch (...)
        {
            ADD_FAILURE() << "Should not have caught exception.";
        }

        // Temporary file should still exist
        EXPECT_TRUE(fs::exists(path));

        // Make file removable and delete it
        makeFileRemovable(path);
        fs::remove(path);
    }
}

TEST(TemporaryFileTests, Remove)
{
    // Test where works
    {
        // Create TemporaryFile object and verify temporary file exists
        TemporaryFile file{};
        EXPECT_FALSE(file.getPath().empty());
        EXPECT_TRUE(fs::exists(file.getPath()));

        // Save path to temporary file
        fs::path path = file.getPath();

        // Delete temporary file
        file.remove();

        // Verify path is cleared and file does not exist
        EXPECT_TRUE(file.getPath().empty());
        EXPECT_FALSE(fs::exists(path));

        // Delete temporary file again; should do nothing
        file.remove();
        EXPECT_TRUE(file.getPath().empty());
        EXPECT_FALSE(fs::exists(path));
    }

    // Test where fails
    {
        // Create TemporaryFile object and verify temporary file exists
        TemporaryFile file{};
        EXPECT_FALSE(file.getPath().empty());
        EXPECT_TRUE(fs::exists(file.getPath()));

        // Make file unremovable
        makeFileUnRemovable(file.getPath());

        try
        {
            // Try to delete temporary file; should fail with exception
            file.remove();
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const std::exception& e)
        {
            // This is expected.  Exception message will vary.
        }

        // Make file removable again so it will be deleted by the destructor
        makeFileRemovable(file.getPath());
    }
}

TEST(TemporaryFileTests, GetPath)
{
    TemporaryFile file{};
    EXPECT_FALSE(file.getPath().empty());
    EXPECT_EQ(file.getPath().parent_path(), "/tmp");
    EXPECT_TRUE(fs::exists(file.getPath()));
}
