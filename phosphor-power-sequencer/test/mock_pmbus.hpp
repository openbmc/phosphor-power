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
#pragma once

#include "pmbus.hpp"

#include <filesystem>

#include <gmock/gmock.h>

namespace phosphor::pmbus
{

namespace fs = std::filesystem;

/**
 * @class MockPMBus
 *
 * Mock implementation of the PMBusBase interface.
 */
class MockPMBus : public PMBusBase
{
  public:
    // Specify which compiler-generated methods we want
    MockPMBus() = default;
    MockPMBus(const MockPMBus&) = delete;
    MockPMBus(MockPMBus&&) = delete;
    MockPMBus& operator=(const MockPMBus&) = delete;
    MockPMBus& operator=(MockPMBus&&) = delete;
    virtual ~MockPMBus() = default;

    MOCK_METHOD(uint64_t, read,
                (const std::string& name, Type type, bool errTrace),
                (override));
    MOCK_METHOD(std::string, readString, (const std::string& name, Type type),
                (override));
    MOCK_METHOD(std::vector<uint8_t>, readBinary,
                (const std::string& name, Type type, size_t length),
                (override));
    MOCK_METHOD(void, writeBinary,
                (const std::string& name, std::vector<uint8_t> data, Type type),
                (override));
    MOCK_METHOD(void, findHwmonDir, (), (override));
    MOCK_METHOD(const fs::path&, path, (), (const, override));
    MOCK_METHOD(std::string, insertPageNum,
                (const std::string& templateName, size_t page), (override));
    MOCK_METHOD(fs::path, getPath, (Type type), (override));
};

} // namespace phosphor::pmbus
