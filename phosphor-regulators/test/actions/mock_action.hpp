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
#pragma once

#include "action.hpp"
#include "action_environment.hpp"

#include <gmock/gmock.h>

namespace phosphor
{
namespace power
{
namespace regulators
{

class MockAction : public Action
{
  public:
    // Specify which compiler-generated methods we want
    MockAction() = default;
    MockAction(const MockAction&) = delete;
    MockAction(MockAction&&) = delete;
    MockAction& operator=(const MockAction&) = delete;
    MockAction& operator=(MockAction&&) = delete;
    virtual ~MockAction() = default;

    MOCK_METHOD(bool, execute, (ActionEnvironment & environment), (override));
};

} // namespace regulators
} // namespace power
} // namespace phosphor
