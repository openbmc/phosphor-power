#pragma once

#include "../i2c_interface.hpp"

#include <gmock/gmock.h>

namespace i2c
{

class MockedI2CInterface : public I2CInterface
{
  public:
    virtual ~MockedI2CInterface() = default;

    MOCK_METHOD3(read,
                 void(uint8_t addr, uint8_t size, std::vector<uint8_t>& data));

    MOCK_METHOD3(write, bool(uint8_t addr, uint8_t size, const uint8_t* data));
};

std::unique_ptr<I2CInterface> Create(uint8_t /*busId*/, uint8_t /*devAddr*/)
{
    return std::make_unique<MockedI2CInterface>();
}

} // namespace i2c
