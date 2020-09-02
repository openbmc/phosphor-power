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
#include "test_utils.hpp"

#include <errno.h>     // for errno
#include <fcntl.h>     // for fcntl()
#include <string.h>    // for memset(), size_t
#include <sys/types.h> // for lseek()
#include <unistd.h>    // for read(), write(), lseek(), fcntl(), close()

#include <exception>
#include <filesystem>

#include <gtest/gtest.h>

using namespace phosphor::power::regulators;
using namespace phosphor::power::regulators::test_utils;
namespace fs = std::filesystem;

/**
 * Returns whether the specified file descriptor is valid/open.
 *
 * @param[in] fd - File descriptor
 * @return true if descriptor is valid/open, false otherwise
 */
bool isValid(int fd)
{
    return (fcntl(fd, F_GETFL) != -1) || (errno != EBADF);
}

TEST(FFDCFileTests, Constructor)
{
    // Test where only the FFDCFormat parameter is specified
    {
        FFDCFile file{FFDCFormat::JSON};
        EXPECT_NE(file.getFileDescriptor(), -1);
        EXPECT_TRUE(isValid(file.getFileDescriptor()));
        EXPECT_EQ(file.getFormat(), FFDCFormat::JSON);
        EXPECT_FALSE(file.getPath().empty());
        EXPECT_TRUE(fs::exists(file.getPath()));
        EXPECT_EQ(file.getSubType(), 0);
        EXPECT_EQ(file.getVersion(), 0);
    }

    // Test where all constructor parameters are specified
    {
        FFDCFile file{FFDCFormat::Custom, 2, 3};
        EXPECT_NE(file.getFileDescriptor(), -1);
        EXPECT_TRUE(isValid(file.getFileDescriptor()));
        EXPECT_EQ(file.getFormat(), FFDCFormat::Custom);
        EXPECT_FALSE(file.getPath().empty());
        EXPECT_TRUE(fs::exists(file.getPath()));
        EXPECT_EQ(file.getSubType(), 2);
        EXPECT_EQ(file.getVersion(), 3);
    }

    // Note: The case where open() fails currently needs to be tested manually
}

TEST(FFDCFileTests, GetFileDescriptor)
{
    FFDCFile file{FFDCFormat::JSON};
    int fd = file.getFileDescriptor();
    EXPECT_NE(fd, -1);
    EXPECT_TRUE(isValid(fd));

    // Write some data to the file
    char buffer[] = "This is some sample data";
    size_t count = sizeof(buffer);
    EXPECT_EQ(write(fd, buffer, count), count);

    // Seek back to the beginning of the file
    EXPECT_EQ(lseek(fd, 0, SEEK_SET), 0);

    // Clear buffer
    memset(buffer, '\0', count);
    EXPECT_STREQ(buffer, "");

    // Read and verify file contents
    EXPECT_EQ(read(fd, buffer, count), count);
    EXPECT_STREQ(buffer, "This is some sample data");
}

TEST(FFDCFileTests, GetFormat)
{
    // Test where 'Text' was specified
    {
        FFDCFile file{FFDCFormat::Text};
        EXPECT_EQ(file.getFormat(), FFDCFormat::Text);
    }

    // Test where 'Custom' was specified
    {
        FFDCFile file{FFDCFormat::Custom, 2, 3};
        EXPECT_EQ(file.getFormat(), FFDCFormat::Custom);
    }
}

TEST(FFDCFileTests, GetPath)
{
    FFDCFile file{FFDCFormat::JSON};
    EXPECT_FALSE(file.getPath().empty());
    EXPECT_TRUE(fs::exists(file.getPath()));
}

TEST(FFDCFileTests, GetSubType)
{
    // Test where subType was not specified
    {
        FFDCFile file{FFDCFormat::JSON};
        EXPECT_EQ(file.getSubType(), 0);
    }

    // Test where subType was specified
    {
        FFDCFile file{FFDCFormat::Custom, 3, 2};
        EXPECT_EQ(file.getSubType(), 3);
    }
}

TEST(FFDCFileTests, GetVersion)
{
    // Test where version was not specified
    {
        FFDCFile file{FFDCFormat::JSON};
        EXPECT_EQ(file.getVersion(), 0);
    }

    // Test where version was specified
    {
        FFDCFile file{FFDCFormat::Custom, 2, 5};
        EXPECT_EQ(file.getVersion(), 5);
    }
}

TEST(FFDCFileTests, Remove)
{
    // Test where works
    {
        FFDCFile file{FFDCFormat::JSON};
        EXPECT_NE(file.getFileDescriptor(), -1);
        EXPECT_TRUE(isValid(file.getFileDescriptor()));
        EXPECT_FALSE(file.getPath().empty());
        EXPECT_TRUE(fs::exists(file.getPath()));

        int fd = file.getFileDescriptor();
        fs::path path = file.getPath();

        file.remove();
        EXPECT_EQ(file.getFileDescriptor(), -1);
        EXPECT_TRUE(file.getPath().empty());

        EXPECT_FALSE(isValid(fd));
        EXPECT_FALSE(fs::exists(path));
    }

    // Test where file was already removed
    {
        FFDCFile file{FFDCFormat::JSON};
        EXPECT_NE(file.getFileDescriptor(), -1);
        EXPECT_FALSE(file.getPath().empty());

        file.remove();
        EXPECT_EQ(file.getFileDescriptor(), -1);
        EXPECT_TRUE(file.getPath().empty());

        file.remove();
        EXPECT_EQ(file.getFileDescriptor(), -1);
        EXPECT_TRUE(file.getPath().empty());
    }

    // Test where closing the file fails
    {
        FFDCFile file{FFDCFormat::JSON};
        int fd = file.getFileDescriptor();
        EXPECT_TRUE(isValid(fd));

        EXPECT_EQ(close(fd), 0);
        EXPECT_FALSE(isValid(fd));

        try
        {
            file.remove();
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const std::exception& e)
        {
            EXPECT_NE(std::string{e.what()}.find("Unable to close FFDC file: "),
                      std::string::npos);
        }
    }

    // Test where deleting the file fails
    {
        FFDCFile file{FFDCFormat::JSON};
        fs::path path = file.getPath();
        EXPECT_TRUE(fs::exists(path));

        makeFileUnRemovable(path);
        try
        {
            file.remove();
            ADD_FAILURE() << "Should not have reached this line.";
        }
        catch (const std::exception& e)
        {
            // This is expected.  Exception message will vary.
        }
        makeFileRemovable(path);
    }
}
