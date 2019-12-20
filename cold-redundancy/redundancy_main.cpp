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

#include <cold_redundancy.hpp>
#include <sdbusplus/asio/object_server.hpp>

int main(int argc, char** argv)
{
    boost::asio::io_service io;
    auto systemBus = std::make_shared<sdbusplus::asio::connection>(io);
    std::vector<std::unique_ptr<sdbusplus::bus::match::match>> matches;

    systemBus->request_name("xyz.openbmc_project.PSURedundancy");
    sdbusplus::asio::object_server objectServer(systemBus);

    ColdRedundancy coldRedundancy(io, objectServer, systemBus, matches);

    return 0;
}
