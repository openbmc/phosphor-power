/*
// Copyright (c) 2019 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#include <sdbusplus/asio/object_server.hpp>
#include <utility.hpp>

const constexpr char* psuInterface =
    "/xyz/openbmc_project/inventory/system/powersupply/";

class ColdRedundancy
{
  public:
    ColdRedundancy(
        boost::asio::io_service& io,
        sdbusplus::asio::object_server& objectServer,
        std::shared_ptr<sdbusplus::asio::connection>& dbusConnection,
        std::vector<std::unique_ptr<sdbusplus::bus::match::match>>& matches);

    void
        createPSU(boost::asio::io_service& io,
                  sdbusplus::asio::object_server& objectServer,
                  std::shared_ptr<sdbusplus::asio::connection>& dbusConnection);

  private:
    uint8_t psOrder;
    uint8_t numberOfPSU = 0;

    sdbusplus::asio::object_server& objServer;
    std::shared_ptr<sdbusplus::asio::connection>& systemBus;

    boost::asio::steady_timer filterTimer;
};

class PowerSupply
{
  public:
    PowerSupply(
        std::string& name, uint8_t bus, uint8_t address, uint8_t order,
        const std::shared_ptr<sdbusplus::asio::connection>& dbusConnection);
    ~PowerSupply();
    std::string name;
    uint8_t order = 0;
    uint8_t bus;
    uint8_t address;
    PSUState state = PSUState::normal;
};
