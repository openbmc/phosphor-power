/**
 * Copyright © 2026 IBM Corporation
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

#include "chassis.hpp"
#include "mock_services.hpp"
#include "system.hpp"

#include <sdbusplus/bus.hpp>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

using namespace phosphor::power::chassis;

/**
 * Creates a Chassis object with the specified parameters.
 *
 * @param number Chassis number within the system.
 * @param services Shared pointer to services
 * @param presencePath Optional presence path for the chassis
 * @return Chassis object
 */
std::unique_ptr<Chassis> createChassis(
    unsigned int number, std::shared_ptr<Services> services,
    std::optional<std::string> presencePath = std::nullopt)
{
    std::vector<std::unique_ptr<Gpio>> gpios;
    return std::make_unique<Chassis>(number, std::move(services),
                                     std::move(presencePath), std::move(gpios));
}

/**
 * @class SystemTests
 *
 * Test fixture for System class tests requiring D-Bus.
 */
class SystemTests : public ::testing::Test
{
  public:
    SystemTests() :
        bus{sdbusplus::bus::new_default()},
        services{std::make_shared<MockServices>()}
    {}

  protected:
    sdbusplus::bus_t bus;
    std::shared_ptr<MockServices> services;
};

TEST_F(SystemTests, Constructor)
{
    // Test with single chassis
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(createChassis(1, services, "/dev/i2c-159"));
        System system{std::move(chassis), services};

        EXPECT_EQ(system.getChassis().size(), 1);
        EXPECT_EQ(system.getChassis()[0]->getNumber(), 1);
        EXPECT_EQ(system.getChassis()[0]->getPresencePath(), "/dev/i2c-159");
    }

    // Test with multiple chassis
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        chassis.emplace_back(createChassis(1, services, "/dev/i2c-159"));
        chassis.emplace_back(createChassis(2, services, "/dev/i2c-160"));
        chassis.emplace_back(createChassis(3, services, std::nullopt));
        System system{std::move(chassis), services};

        EXPECT_EQ(system.getChassis().size(), 3);
        EXPECT_EQ(system.getChassis()[0]->getNumber(), 1);
        EXPECT_EQ(system.getChassis()[0]->getPresencePath(), "/dev/i2c-159");
        EXPECT_EQ(system.getChassis()[1]->getNumber(), 2);
        EXPECT_EQ(system.getChassis()[1]->getPresencePath(), "/dev/i2c-160");
        EXPECT_EQ(system.getChassis()[2]->getNumber(), 3);
        EXPECT_FALSE(system.getChassis()[2]->getPresencePath().has_value());
    }

    // Test with empty chassis vector
    {
        std::vector<std::unique_ptr<Chassis>> chassis;
        System system{std::move(chassis), services};

        EXPECT_EQ(system.getChassis().size(), 0);
    }
}

TEST_F(SystemTests, GetChassis)
{
    std::vector<std::unique_ptr<Chassis>> chassis;
    chassis.emplace_back(createChassis(1, services, "/dev/i2c-159"));
    chassis.emplace_back(createChassis(3, services, "/dev/i2c-161"));
    chassis.emplace_back(createChassis(7, services, std::nullopt));
    System system{std::move(chassis), services};

    EXPECT_EQ(system.getChassis().size(), 3);
    EXPECT_EQ(system.getChassis()[0]->getNumber(), 1);
    EXPECT_EQ(system.getChassis()[0]->getPresencePath(), "/dev/i2c-159");
    EXPECT_EQ(system.getChassis()[1]->getNumber(), 3);
    EXPECT_EQ(system.getChassis()[1]->getPresencePath(), "/dev/i2c-161");
    EXPECT_EQ(system.getChassis()[2]->getNumber(), 7);
    EXPECT_FALSE(system.getChassis()[2]->getPresencePath().has_value());
}

TEST_F(SystemTests, InitializePowerSystemInputsStatus)
{
    using PowerSystemInputs = sdbusplus::server::xyz::openbmc_project::state::
        decorator::PowerSystemInputs;

    std::vector<std::unique_ptr<Chassis>> chassis;
    chassis.emplace_back(createChassis(1, services, "/dev/i2c-159"));
    chassis.emplace_back(createChassis(2, services, "/dev/i2c-160"));
    System system{std::move(chassis), services};

    // Verify interfaces are null before initialization
    EXPECT_EQ(system.getChassis()[0]->getPowerSystemInputsInterface(), nullptr);
    EXPECT_EQ(system.getChassis()[1]->getPowerSystemInputsInterface(), nullptr);

    // Initialize power system inputs status
    system.initializePowerSystemInputs(bus);

    // Verify interfaces were created with Good status
    auto& interface1 = system.getChassis()[0]->getPowerSystemInputsInterface();
    auto& interface2 = system.getChassis()[1]->getPowerSystemInputsInterface();
    ASSERT_NE(interface1, nullptr);
    ASSERT_NE(interface2, nullptr);
    EXPECT_EQ(interface1->status(), PowerSystemInputs::Status::Good);
    EXPECT_EQ(interface2->status(), PowerSystemInputs::Status::Good);
}
