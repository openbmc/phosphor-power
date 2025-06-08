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

#include <filesystem>
#include <fstream>
#include <string>
#include <utility>

#include <gtest/gtest.h>

using namespace phosphor::power::util;
namespace fs = std::filesystem;

TEST(TemporarySubDirectoryTests, DefaultConstructor)
{
    TemporarySubDirectory subdirectory{};

    fs::path path = subdirectory.getPath();
    EXPECT_FALSE(path.empty());
    EXPECT_TRUE(fs::exists(path));
    EXPECT_TRUE(fs::is_directory(path));

    fs::path parentDir = path.parent_path();
    EXPECT_EQ(parentDir, "/tmp");

    std::string baseName = path.filename();
    EXPECT_TRUE(baseName.starts_with("phosphor-power-"));
}

TEST(TemporarySubDirectoryTests, MoveConstructor)
{
    // Create first object and verify subdirectory exists
    TemporarySubDirectory subdirectory1{};
    EXPECT_FALSE(subdirectory1.getPath().empty());
    EXPECT_TRUE(fs::exists(subdirectory1.getPath()));

    // Save path to subdirectory
    fs::path path = subdirectory1.getPath();

    // Create second object by moving first object
    TemporarySubDirectory subdirectory2{std::move(subdirectory1)};

    // Verify first object now has an empty path
    EXPECT_TRUE(subdirectory1.getPath().empty());

    // Verify second object now owns same subdirectory and subdirectory exists
    EXPECT_EQ(subdirectory2.getPath(), path);
    EXPECT_TRUE(fs::exists(subdirectory2.getPath()));
}

TEST(TemporarySubDirectoryTests, MoveAssignmentOperator)
{
    // Test where works: object is moved
    {
        // Create first object and verify subdirectory exists
        TemporarySubDirectory subdirectory1{};
        EXPECT_FALSE(subdirectory1.getPath().empty());
        EXPECT_TRUE(fs::exists(subdirectory1.getPath()));

        // Save path to first subdirectory
        fs::path path1 = subdirectory1.getPath();

        // Create second object and verify subdirectory exists
        TemporarySubDirectory subdirectory2{};
        EXPECT_FALSE(subdirectory2.getPath().empty());
        EXPECT_TRUE(fs::exists(subdirectory2.getPath()));

        // Save path to second subdirectory
        fs::path path2 = subdirectory2.getPath();

        // Verify temporary subdirectories are different
        EXPECT_NE(path1, path2);

        // Move first object into the second
        subdirectory2 = std::move(subdirectory1);

        // Verify first object now has an empty path
        EXPECT_TRUE(subdirectory1.getPath().empty());

        // Verify second object now owns first subdirectory and subdirectory
        // exists
        EXPECT_EQ(subdirectory2.getPath(), path1);
        EXPECT_TRUE(fs::exists(path1));

        // Verify second subdirectory was deleted
        EXPECT_FALSE(fs::exists(path2));
    }

    // Test where does nothing: object moved into itself
    {
        // Create object and verify subdirectory exists
        TemporarySubDirectory subdirectory{};
        EXPECT_FALSE(subdirectory.getPath().empty());
        EXPECT_TRUE(fs::exists(subdirectory.getPath()));

        // Save path to subdirectory
        fs::path path = subdirectory.getPath();

// This is undefined behavior in C++, but suppress the warning
// to observe how the class handles it.
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-move"
#endif
        // Try to move object into itself; should do nothing
        subdirectory = static_cast<TemporarySubDirectory&&>(subdirectory);
#ifdef __clang__
#pragma clang diagnostic pop
#endif

        // Verify object still owns same subdirectory and subdirectory exists
        EXPECT_EQ(subdirectory.getPath(), path);
        EXPECT_TRUE(fs::exists(path));
    }

    // Test where fails: Cannot delete subdirectory
    {
        // Create first object and verify subdirectory exists
        TemporarySubDirectory subdirectory1{};
        EXPECT_FALSE(subdirectory1.getPath().empty());
        EXPECT_TRUE(fs::exists(subdirectory1.getPath()));

        // Save path to first subdirectory
        fs::path path1 = subdirectory1.getPath();

        // Create second object and verify subdirectory exists
        TemporarySubDirectory subdirectory2{};
        EXPECT_FALSE(subdirectory2.getPath().empty());
        EXPECT_TRUE(fs::exists(subdirectory2.getPath()));

        // Save path to second subdirectory
        fs::path path2 = subdirectory2.getPath();

        // Verify temporary subdirectories are different
        EXPECT_NE(path1, path2);

        // Change second subdirectory to unreadable so it cannot be removed
        fs::permissions(path2, fs::perms::none);

        try
        {
            // Try to move first object into the second; should throw exception
            subdirectory2 = std::move(subdirectory1);
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const std::exception& e)
        {
            // This is expected.  Exception message will vary.
        }

        // Change second subdirectory to readable/writable so it can be removed
        fs::permissions(path2, fs::perms::owner_all);

        // Verify first object has not changed and first subdirectory exists
        EXPECT_EQ(subdirectory1.getPath(), path1);
        EXPECT_TRUE(fs::exists(path1));

        // Verify second object has not changed and second subdirectory exists
        EXPECT_EQ(subdirectory2.getPath(), path2);
        EXPECT_TRUE(fs::exists(path2));
    }
}

TEST(TemporarySubDirectoryTests, Destructor)
{
    // Test where works: Subdirectory is deleted
    {
        fs::path path{};
        {
            TemporarySubDirectory subdirectory{};
            path = subdirectory.getPath();
            EXPECT_TRUE(fs::exists(path));
        }
        EXPECT_FALSE(fs::exists(path));
    }

    // Test where works: Subdirectory was already deleted
    {
        fs::path path{};
        {
            TemporarySubDirectory subdirectory{};
            path = subdirectory.getPath();
            EXPECT_TRUE(fs::exists(path));
            subdirectory.remove();
            EXPECT_FALSE(fs::exists(path));
        }
        EXPECT_FALSE(fs::exists(path));
    }

    // Test where fails: Cannot delete subdirectory: No exception thrown
    {
        fs::path path{};
        try
        {
            TemporarySubDirectory subdirectory{};
            path = subdirectory.getPath();
            EXPECT_TRUE(fs::exists(path));

            // Change subdirectory to unreadable so it cannot be removed
            fs::permissions(path, fs::perms::none);
        }
        catch (...)
        {
            ADD_FAILURE() << "Should not have caught exception.";
        }

        // Change subdirectory to readable/writable so it can be removed
        fs::permissions(path, fs::perms::owner_all);

        // Subdirectory should still exist
        EXPECT_TRUE(fs::exists(path));

        // Delete subdirectory
        fs::remove_all(path);
    }
}

TEST(TemporarySubDirectoryTests, Remove)
{
    // Test where works
    {
        // Create object and verify subdirectory exists
        TemporarySubDirectory subdirectory{};
        EXPECT_FALSE(subdirectory.getPath().empty());
        EXPECT_TRUE(fs::exists(subdirectory.getPath()));

        // Save path to subdirectory
        fs::path path = subdirectory.getPath();

        // Delete subdirectory
        subdirectory.remove();

        // Verify path is cleared and subdirectory does not exist
        EXPECT_TRUE(subdirectory.getPath().empty());
        EXPECT_FALSE(fs::exists(path));

        // Delete subdirectory again; should do nothing
        subdirectory.remove();
        EXPECT_TRUE(subdirectory.getPath().empty());
        EXPECT_FALSE(fs::exists(path));
    }

    // Test where fails
    {
        // Create object and verify subdirectory exists
        TemporarySubDirectory subdirectory{};
        EXPECT_FALSE(subdirectory.getPath().empty());
        EXPECT_TRUE(fs::exists(subdirectory.getPath()));

        // Save path to subdirectory
        fs::path path = subdirectory.getPath();

        // Change subdirectory to unreadable so it cannot be removed
        fs::permissions(path, fs::perms::none);

        try
        {
            // Try to delete subdirectory; should fail with exception
            subdirectory.remove();
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const std::exception& e)
        {
            // This is expected.  Exception message will vary.
        }

        // Change subdirectory to readable/writable so it can be deleted by
        // destructor
        fs::permissions(path, fs::perms::owner_all);
    }
}

TEST(TemporarySubDirectoryTests, GetPath)
{
    TemporarySubDirectory subdirectory{};
    EXPECT_FALSE(subdirectory.getPath().empty());
    EXPECT_EQ(subdirectory.getPath().parent_path(), "/tmp");
    EXPECT_TRUE(fs::exists(subdirectory.getPath()));
}
