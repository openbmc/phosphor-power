/**
 * Copyright © 2021 IBM Corporation
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
#include "id_map.hpp"
#include "log_phase_fault_action.hpp"
#include "mock_services.hpp"
#include "phase_fault.hpp"

#include <gtest/gtest.h>

using namespace phosphor::power::regulators;

TEST(LogPhaseFaultActionTests, Constructor)
{
    LogPhaseFaultAction action{PhaseFaultType::n};
    EXPECT_EQ(action.getType(), PhaseFaultType::n);
}

TEST(LogPhaseFaultActionTests, Execute)
{
    // Test with "n" phase fault type
    {
        IDMap idMap{};
        MockServices services{};
        ActionEnvironment env{idMap, "", services};
        EXPECT_EQ(env.getPhaseFaults().size(), 0);

        LogPhaseFaultAction action{PhaseFaultType::n};
        EXPECT_EQ(action.execute(env), true);

        EXPECT_EQ(env.getPhaseFaults().size(), 1);
        EXPECT_EQ(env.getPhaseFaults().count(PhaseFaultType::n), 1);
    }

    // Test with "n+1" phase fault type
    {
        IDMap idMap{};
        MockServices services{};
        ActionEnvironment env{idMap, "", services};
        EXPECT_EQ(env.getPhaseFaults().size(), 0);

        LogPhaseFaultAction action{PhaseFaultType::n_plus_1};
        EXPECT_EQ(action.execute(env), true);

        EXPECT_EQ(env.getPhaseFaults().size(), 1);
        EXPECT_EQ(env.getPhaseFaults().count(PhaseFaultType::n_plus_1), 1);
    }
}

TEST(LogPhaseFaultActionTests, GetType)
{
    LogPhaseFaultAction action{PhaseFaultType::n_plus_1};
    EXPECT_EQ(action.getType(), PhaseFaultType::n_plus_1);
}

TEST(LogPhaseFaultActionTests, ToString)
{
    LogPhaseFaultAction action{PhaseFaultType::n_plus_1};
    EXPECT_EQ(action.toString(), "log_phase_fault: { type: n+1 }");
}
