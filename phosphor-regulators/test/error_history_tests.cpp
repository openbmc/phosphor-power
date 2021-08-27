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
#include "error_history.hpp"

#include <gtest/gtest.h>

using namespace phosphor::power::regulators;

TEST(ErrorHistoryTests, ErrorType)
{
    EXPECT_EQ(static_cast<int>(ErrorType::internal), 3);
    EXPECT_EQ(static_cast<int>(ErrorType::numTypes), 8);
}

TEST(ErrorHistoryTests, Constructor)
{
    ErrorHistory history{};
    EXPECT_FALSE(history.wasLogged(ErrorType::configFile));
    EXPECT_FALSE(history.wasLogged(ErrorType::dbus));
    EXPECT_FALSE(history.wasLogged(ErrorType::i2c));
    EXPECT_FALSE(history.wasLogged(ErrorType::internal));
    EXPECT_FALSE(history.wasLogged(ErrorType::pmbus));
    EXPECT_FALSE(history.wasLogged(ErrorType::writeVerification));
    EXPECT_FALSE(history.wasLogged(ErrorType::phaseFaultN));
    EXPECT_FALSE(history.wasLogged(ErrorType::phaseFaultNPlus1));
}

TEST(ErrorHistoryTests, Clear)
{
    ErrorHistory history{};

    history.setWasLogged(ErrorType::configFile, true);
    history.setWasLogged(ErrorType::dbus, true);
    history.setWasLogged(ErrorType::i2c, true);
    history.setWasLogged(ErrorType::internal, true);
    history.setWasLogged(ErrorType::pmbus, true);
    history.setWasLogged(ErrorType::writeVerification, true);
    history.setWasLogged(ErrorType::phaseFaultN, true);
    history.setWasLogged(ErrorType::phaseFaultNPlus1, true);

    EXPECT_TRUE(history.wasLogged(ErrorType::configFile));
    EXPECT_TRUE(history.wasLogged(ErrorType::dbus));
    EXPECT_TRUE(history.wasLogged(ErrorType::i2c));
    EXPECT_TRUE(history.wasLogged(ErrorType::internal));
    EXPECT_TRUE(history.wasLogged(ErrorType::pmbus));
    EXPECT_TRUE(history.wasLogged(ErrorType::writeVerification));
    EXPECT_TRUE(history.wasLogged(ErrorType::phaseFaultN));
    EXPECT_TRUE(history.wasLogged(ErrorType::phaseFaultNPlus1));

    history.clear();

    EXPECT_FALSE(history.wasLogged(ErrorType::configFile));
    EXPECT_FALSE(history.wasLogged(ErrorType::dbus));
    EXPECT_FALSE(history.wasLogged(ErrorType::i2c));
    EXPECT_FALSE(history.wasLogged(ErrorType::internal));
    EXPECT_FALSE(history.wasLogged(ErrorType::pmbus));
    EXPECT_FALSE(history.wasLogged(ErrorType::writeVerification));
    EXPECT_FALSE(history.wasLogged(ErrorType::phaseFaultN));
    EXPECT_FALSE(history.wasLogged(ErrorType::phaseFaultNPlus1));
}

TEST(ErrorHistoryTests, SetWasLogged)
{
    ErrorHistory history{};
    EXPECT_FALSE(history.wasLogged(ErrorType::dbus));
    history.setWasLogged(ErrorType::dbus, true);
    EXPECT_TRUE(history.wasLogged(ErrorType::dbus));
    history.setWasLogged(ErrorType::dbus, false);
    EXPECT_FALSE(history.wasLogged(ErrorType::dbus));
}

TEST(ErrorHistoryTests, WasLogged)
{
    ErrorHistory history{};
    EXPECT_FALSE(history.wasLogged(ErrorType::pmbus));
    history.setWasLogged(ErrorType::pmbus, true);
    EXPECT_TRUE(history.wasLogged(ErrorType::pmbus));
    history.setWasLogged(ErrorType::pmbus, false);
    EXPECT_FALSE(history.wasLogged(ErrorType::pmbus));
}
