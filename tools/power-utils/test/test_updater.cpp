/**
 * Copyright © 2019 IBM Corporation
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
#include "../updater.hpp"
#include "../version.hpp"
#include "mocked_i2c_interface.hpp"

#include <filesystem>

#include <gtest/gtest.h>

namespace fs = std::filesystem;

using ::testing::_;
using ::testing::An;
using ::testing::Pointee;

namespace updater
{
namespace internal
{

std::string getDeviceName(std::string devPath);
std::pair<uint8_t, uint8_t> parseDeviceName(const std::string& devName);

} // namespace internal
} // namespace updater

using namespace updater;

class TestUpdater : public ::testing::Test
{
  public:
    using MockedI2CInterface = i2c::MockedI2CInterface;
    using I2CInterface = i2c::I2CInterface;

    TestUpdater()
    {
        setupDeviceSysfs();
    }
    ~TestUpdater()
    {
        fs::remove_all(tmpDir);
    }

    void setupDeviceSysfs()
    {
        auto tmpPath = fs::temp_directory_path();
        tmpDir = (tmpPath / "test_XXXXXX");
        if (!mkdtemp(tmpDir.data()))
        {
            throw "Failed to create temp dir";
        }
        // Create device path with symbol link
        realDevicePath = fs::path(tmpDir) / "devices/3-0068";
        devPath = fs::path(tmpDir) / "i2c";
        fs::create_directories(realDevicePath);
        fs::create_directories(realDevicePath / "driver");
        fs::create_directories(devPath);
        devPath /= "3-0068";
        fs::create_directory_symlink(realDevicePath, devPath);
    }

    MockedI2CInterface& getMockedI2c()
    {
        return *reinterpret_cast<MockedI2CInterface*>(updater->i2c.get());
    }

    std::shared_ptr<I2CInterface> stolenI2C;
    std::unique_ptr<Updater> updater;
    fs::path realDevicePath;
    fs::path devPath;
    std::string tmpDir;
    std::string psuInventoryPath = "/com/example/psu";
    std::string imageDir = "/tmp/image/xxx";
};

TEST_F(TestUpdater, ctordtor)
{
    updater = std::make_unique<Updater>(psuInventoryPath, devPath, imageDir);
}

TEST_F(TestUpdater, doUpdate)
{
    updater = std::make_unique<Updater>(psuInventoryPath, devPath, imageDir);
    updater->createI2CDevice();
    auto& i2c = getMockedI2c();

    EXPECT_CALL(i2c, write(0xf0, 12, _, I2CInterface::Mode::SMBUS));
    EXPECT_CALL(i2c, write(0xf1, An<uint8_t>()));
    EXPECT_CALL(i2c, read(0xf1, An<uint8_t&>()));
    updater->doUpdate();
}

TEST_F(TestUpdater, getDeviceName)
{
    auto ret = internal::getDeviceName("");
    EXPECT_TRUE(ret.empty());

    ret = internal::getDeviceName("/sys/bus/i2c/devices/3-0069");
    EXPECT_EQ("3-0069", ret);

    ret = internal::getDeviceName("/sys/bus/i2c/devices/3-0069/");
    EXPECT_EQ("3-0069", ret);
}

TEST_F(TestUpdater, parseDeviceName)
{
    auto [id, addr] = internal::parseDeviceName("3-0068");
    EXPECT_EQ(3, id);
    EXPECT_EQ(0x68, addr);

    std::tie(id, addr) = internal::parseDeviceName("11-0069");
    EXPECT_EQ(11, id);
    EXPECT_EQ(0x69, addr);

    EXPECT_THROW(internal::parseDeviceName("no-number"), std::invalid_argument);

    EXPECT_DEATH(internal::parseDeviceName("invalid"), "");
}
