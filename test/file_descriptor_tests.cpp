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
#include "file_descriptor.hpp"

#include <errno.h>     // for errno
#include <fcntl.h>     // for open() and fcntl()
#include <sys/stat.h>  // for open()
#include <sys/types.h> // for open()
#include <unistd.h>    // for fcntl()

#include <utility>

#include <gtest/gtest.h>

using namespace phosphor::power::util;

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

/**
 * Creates an open file descriptor.
 *
 * Verifies the file descriptor is valid.
 *
 * @return file descriptor
 */
int createOpenFileDescriptor()
{
    int fd = open("/etc/hosts", O_RDONLY);
    EXPECT_NE(fd, -1);
    EXPECT_TRUE(isValid(fd));
    return fd;
}

TEST(FileDescriptorTests, DefaultConstructor)
{
    FileDescriptor descriptor;
    EXPECT_EQ(descriptor(), -1);
    EXPECT_FALSE(descriptor.operator bool());
}

TEST(FileDescriptorTests, IntConstructor)
{
    int fd = createOpenFileDescriptor();
    FileDescriptor descriptor{fd};
    EXPECT_EQ(descriptor(), fd);
    EXPECT_TRUE(descriptor.operator bool());
    EXPECT_TRUE(isValid(fd));
}

TEST(FileDescriptorTests, MoveConstructor)
{
    // Create first FileDescriptor object with open file descriptor
    int fd = createOpenFileDescriptor();
    FileDescriptor descriptor1{fd};
    EXPECT_EQ(descriptor1(), fd);
    EXPECT_TRUE(isValid(fd));

    // Create second FileDescriptor object, moving in the contents of the first
    FileDescriptor descriptor2{std::move(descriptor1)};

    // Verify descriptor has been moved out of first object
    EXPECT_EQ(descriptor1(), -1);

    // Verify descriptor has been moved into second object
    EXPECT_EQ(descriptor2(), fd);

    // Verify descriptor is still valid/open
    EXPECT_TRUE(isValid(fd));
}

TEST(FileDescriptorTests, MoveAssignmentOperator)
{
    // Test where move is valid
    {
        // Create first FileDescriptor object with open file descriptor
        int fd1 = createOpenFileDescriptor();
        FileDescriptor descriptor1{fd1};
        EXPECT_EQ(descriptor1(), fd1);
        EXPECT_TRUE(isValid(fd1));

        // Create second FileDescriptor object with open file descriptor
        int fd2 = createOpenFileDescriptor();
        FileDescriptor descriptor2{fd2};
        EXPECT_EQ(descriptor2(), fd2);
        EXPECT_TRUE(isValid(fd2));

        // Move second FileDescriptor object into the first
        descriptor1 = std::move(descriptor2);

        // Verify second file descriptor has been moved into first object
        EXPECT_EQ(descriptor1(), fd2);

        // Verify second file descriptor has been moved out of second object
        EXPECT_EQ(descriptor2(), -1);

        // Verify first file descriptor has been closed and is no longer valid
        EXPECT_FALSE(isValid(fd1));

        // Verify second file descriptor is still valid
        EXPECT_TRUE(isValid(fd2));
    }

    // Test where move is invalid: Attempt to move object into itself
    {
        // Create FileDescriptor object with open file descriptor
        int fd = createOpenFileDescriptor();
        FileDescriptor descriptor{fd};
        EXPECT_EQ(descriptor(), fd);
        EXPECT_TRUE(isValid(fd));

// This is undefined behavior in C++, but suppress the warning
// to observe how the class handles it.
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-move"
#endif
        // Try to move object into itself
        descriptor = static_cast<FileDescriptor&&>(descriptor);
#ifdef __clang__
#pragma clang diagnostic pop
#endif

        // Verify object still contains file descriptor
        EXPECT_EQ(descriptor(), fd);
        EXPECT_TRUE(isValid(fd));
    }
}

TEST(FileDescriptorTests, Destructor)
{
    // Test where file descriptor was never set
    {
        FileDescriptor descriptor;
        EXPECT_EQ(descriptor(), -1);
    }

    // Test where file descriptor was already closed
    {
        // Create FileDescriptor object with open file descriptor.  Close the
        // descriptor explicitly.
        int fd = createOpenFileDescriptor();
        {
            FileDescriptor descriptor{fd};
            EXPECT_EQ(descriptor(), fd);
            EXPECT_TRUE(isValid(fd));

            EXPECT_EQ(descriptor.close(), 0);
            EXPECT_EQ(descriptor(), -1);
            EXPECT_FALSE(isValid(fd));
        }
        EXPECT_FALSE(isValid(fd));
    }

    // Test where file descriptor needs to be closed
    {
        // Create FileDescriptor object with open file descriptor.  Destructor
        // will close descriptor.
        int fd = createOpenFileDescriptor();
        {
            FileDescriptor descriptor{fd};
            EXPECT_EQ(descriptor(), fd);
            EXPECT_TRUE(isValid(fd));
        }
        EXPECT_FALSE(isValid(fd));
    }
}

TEST(FileDescriptorTests, FunctionCallOperator)
{
    // Test where FileDescriptor object does not contain a valid file descriptor
    FileDescriptor descriptor{};
    EXPECT_EQ(descriptor(), -1);

    // Test where FileDescriptor object contains a valid file descriptor
    int fd = createOpenFileDescriptor();
    descriptor.set(fd);
    EXPECT_EQ(descriptor(), fd);
}

TEST(FileDescriptorTests, OperatorBool)
{
    // Test where FileDescriptor object does not contain a valid file descriptor
    FileDescriptor descriptor{};
    EXPECT_FALSE(descriptor.operator bool());
    if (descriptor)
    {
        ADD_FAILURE() << "Should not have reached this line.";
    }

    // Test where FileDescriptor object contains a valid file descriptor
    int fd = createOpenFileDescriptor();
    descriptor.set(fd);
    EXPECT_TRUE(descriptor.operator bool());
    if (!descriptor)
    {
        ADD_FAILURE() << "Should not have reached this line.";
    }

    // Test where file descriptor has been closed
    EXPECT_EQ(descriptor.close(), 0);
    EXPECT_FALSE(descriptor.operator bool());
    if (descriptor)
    {
        ADD_FAILURE() << "Should not have reached this line.";
    }
}

TEST(FileDescriptorTests, Close)
{
    // Test where object contains an open file descriptor
    int fd = createOpenFileDescriptor();
    FileDescriptor descriptor{fd};
    EXPECT_EQ(descriptor(), fd);
    EXPECT_TRUE(isValid(fd));
    EXPECT_EQ(descriptor.close(), 0);
    EXPECT_EQ(descriptor(), -1);
    EXPECT_FALSE(isValid(fd));

    // Test where object does not contain an open file descriptor
    EXPECT_EQ(descriptor(), -1);
    EXPECT_EQ(descriptor.close(), 0);
    EXPECT_EQ(descriptor(), -1);

    // Test where close() fails due to invalid file descriptor
    descriptor.set(999999);
    EXPECT_EQ(descriptor.close(), -1);
    EXPECT_EQ(errno, EBADF);
    EXPECT_EQ(descriptor(), -1);
}

TEST(FileDescriptorTests, Set)
{
    // Test where object does not contain an open file descriptor
    FileDescriptor descriptor{};
    EXPECT_EQ(descriptor(), -1);
    int fd1 = createOpenFileDescriptor();
    descriptor.set(fd1);
    EXPECT_EQ(descriptor(), fd1);
    EXPECT_TRUE(isValid(fd1));

    // Test where object contains an open file descriptor.  Should close
    // previous descriptor.
    EXPECT_EQ(descriptor(), fd1);
    EXPECT_TRUE(isValid(fd1));
    int fd2 = createOpenFileDescriptor();
    descriptor.set(fd2);
    EXPECT_EQ(descriptor(), fd2);
    EXPECT_FALSE(isValid(fd1));
    EXPECT_TRUE(isValid(fd2));

    // Test where -1 is specified.  Should close previous descriptor.
    EXPECT_EQ(descriptor(), fd2);
    EXPECT_TRUE(isValid(fd2));
    descriptor.set(-1);
    EXPECT_EQ(descriptor(), -1);
    EXPECT_FALSE(isValid(fd2));
}
