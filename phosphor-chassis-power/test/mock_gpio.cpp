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

#include "gpio.hpp"

#include <memory>
#include <string>

namespace phosphor::power::chassis
{

/**
 * @class MockGpio
 *
 * Mock implementation of GpioInterface for testing.
 */
class MockGpio : public GpioInterface
{
  public:
    MockGpio(const std::string& name, GpioDirection direction,
             GpioPolarity polarity) :
        name{name}, direction{direction}, polarity{polarity}
    {}

    virtual ~MockGpio() = default;

    virtual bool requestRead() override
    {
        requested = true;
        return true;
    }

    virtual bool requestWrite(int initialValue) override
    {
        requested = true;
        value = initialValue;
        return true;
    }

    virtual int getValue() override
    {
        return value;
    }

    virtual void setValue(int newValue) override
    {
        value = newValue;
    }

    virtual void release() override
    {
        requested = false;
    }

    virtual const std::string& getName() const override
    {
        return name;
    }

    virtual GpioDirection getDirection() const override
    {
        return direction;
    }

    virtual GpioPolarity getPolarity() const override
    {
        return polarity;
    }

  private:
    const std::string name;
    const GpioDirection direction;
    const GpioPolarity polarity;
    bool requested{false};
    int value{0};
};

std::unique_ptr<GpioInterface> createGPIO(
    const std::string& name, GpioDirection direction, GpioPolarity polarity)
{
    return std::make_unique<MockGpio>(name, direction, polarity);
}

} // namespace phosphor::power::chassis
