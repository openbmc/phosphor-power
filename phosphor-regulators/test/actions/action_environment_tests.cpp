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
#include "action_environment.hpp"
#include "device.hpp"
#include "id_map.hpp"
#include "rule.hpp"

#include <cstddef> // for size_t
#include <exception>
#include <stdexcept>

#include <gtest/gtest.h>

using namespace phosphor::power::regulators;

TEST(ActionEnvironmentTests, Constructor)
{
    // Create IDMap
    IDMap idMap{};
    Device reg1{"regulator1"};
    idMap.addDevice(reg1);

    // Verify object state after constructor
    try
    {
        ActionEnvironment env{idMap, "regulator1"};
        EXPECT_EQ(env.getDevice().getID(), "regulator1");
        EXPECT_EQ(env.getDeviceID(), "regulator1");
        EXPECT_EQ(env.getRuleDepth(), 0);
        EXPECT_THROW(env.getVolts(), std::logic_error);
        EXPECT_EQ(env.hasVolts(), false);
    }
    catch (const std::exception& error)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }
}

TEST(ActionEnvironmentTests, DecrementRuleDepth)
{
    IDMap idMap{};
    ActionEnvironment env{idMap, ""};
    EXPECT_EQ(env.getRuleDepth(), 0);
    env.incrementRuleDepth();
    env.incrementRuleDepth();
    EXPECT_EQ(env.getRuleDepth(), 2);
    env.decrementRuleDepth();
    EXPECT_EQ(env.getRuleDepth(), 1);
    env.decrementRuleDepth();
    EXPECT_EQ(env.getRuleDepth(), 0);
    env.decrementRuleDepth();
    EXPECT_EQ(env.getRuleDepth(), 0);
}

TEST(ActionEnvironmentTests, GetDevice)
{
    // Create IDMap
    IDMap idMap{};
    Device reg1{"regulator1"};
    idMap.addDevice(reg1);

    ActionEnvironment env{idMap, "regulator1"};

    // Test where current device ID is in the IDMap
    try
    {
        Device& device = env.getDevice();
        EXPECT_EQ(device.getID(), "regulator1");
        EXPECT_EQ(&device, &reg1);
    }
    catch (const std::exception& error)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test where current device ID is not in the IDMap
    env.setDeviceID("regulator2");
    try
    {
        env.getDevice();
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& ia_error)
    {
        EXPECT_STREQ(ia_error.what(),
                     "Unable to find device with ID \"regulator2\"");
    }
    catch (const std::exception& error)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }
}

TEST(ActionEnvironmentTests, GetDeviceID)
{
    IDMap idMap{};
    ActionEnvironment env{idMap, ""};
    EXPECT_EQ(env.getDeviceID(), "");
    env.setDeviceID("regulator1");
    EXPECT_EQ(env.getDeviceID(), "regulator1");
}

TEST(ActionEnvironmentTests, GetRule)
{
    // Create IDMap
    IDMap idMap{};
    Rule setVoltageRule{"set_voltage_rule"};
    idMap.addRule(setVoltageRule);

    ActionEnvironment env{idMap, ""};

    // Test where rule ID is in the IDMap
    try
    {
        Rule& rule = env.getRule("set_voltage_rule");
        EXPECT_EQ(rule.getID(), "set_voltage_rule");
        EXPECT_EQ(&rule, &setVoltageRule);
    }
    catch (const std::exception& error)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test where rule ID is not in the IDMap
    try
    {
        env.getRule("set_voltage_rule2");
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& ia_error)
    {
        EXPECT_STREQ(ia_error.what(),
                     "Unable to find rule with ID \"set_voltage_rule2\"");
    }
    catch (const std::exception& error)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }
}

TEST(ActionEnvironmentTests, GetRuleDepth)
{
    IDMap idMap{};
    ActionEnvironment env{idMap, ""};
    EXPECT_EQ(env.getRuleDepth(), 0);
    env.incrementRuleDepth();
    EXPECT_EQ(env.getRuleDepth(), 1);
    env.incrementRuleDepth();
    EXPECT_EQ(env.getRuleDepth(), 2);
    env.decrementRuleDepth();
    EXPECT_EQ(env.getRuleDepth(), 1);
    env.decrementRuleDepth();
    EXPECT_EQ(env.getRuleDepth(), 0);
}

TEST(ActionEnvironmentTests, GetVolts)
{
    IDMap idMap{};
    ActionEnvironment env{idMap, ""};
    EXPECT_EQ(env.hasVolts(), false);

    // Test where a volts value has not been set
    try
    {
        env.getVolts();
    }
    catch (const std::logic_error& l_error)
    {
        EXPECT_STREQ(l_error.what(), "No volts value has been set.");
    }
    catch (const std::exception& error)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test where a volts value has been set
    env.setVolts(1.31);
    try
    {
        double volts = env.getVolts();
        EXPECT_EQ(volts, 1.31);
    }
    catch (const std::exception& error)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }
}

TEST(ActionEnvironmentTests, HasVolts)
{
    IDMap idMap{};
    ActionEnvironment env{idMap, ""};
    EXPECT_EQ(env.hasVolts(), false);
    env.setVolts(1.31);
    EXPECT_EQ(env.hasVolts(), true);
}

TEST(ActionEnvironmentTests, IncrementRuleDepth)
{
    IDMap idMap{};
    ActionEnvironment env{idMap, ""};
    EXPECT_EQ(env.getRuleDepth(), 0);

    // Test where rule depth has not exceeded maximum
    try
    {
        for (size_t i = 1; i <= env.maxRuleDepth; ++i)
        {
            env.incrementRuleDepth();
            EXPECT_EQ(env.getRuleDepth(), i);
        }
    }
    catch (const std::exception& error)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test where rule depth has exceeded maximum
    try
    {
        env.incrementRuleDepth();
    }
    catch (const std::runtime_error& r_error)
    {
        EXPECT_STREQ(r_error.what(), "Maximum rule depth exceeded.");
    }
    catch (const std::exception& error)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }
}

TEST(ActionEnvironmentTests, SetDeviceID)
{
    IDMap idMap{};
    Device reg1{"regulator1"};
    idMap.addDevice(reg1);
    ActionEnvironment env{idMap, "regulator1"};

    EXPECT_EQ(env.getDeviceID(), "regulator1");
    env.setDeviceID("regulator2");
    EXPECT_EQ(env.getDeviceID(), "regulator2");
}

TEST(ActionEnvironmentTests, SetVolts)
{
    try
    {
        IDMap idMap{};
        ActionEnvironment env{idMap, ""};
        EXPECT_EQ(env.hasVolts(), false);
        env.setVolts(2.35);
        EXPECT_EQ(env.hasVolts(), true);
        EXPECT_EQ(env.getVolts(), 2.35);
    }
    catch (const std::exception& error)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }
}
