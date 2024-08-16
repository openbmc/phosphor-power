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
#include "action.hpp"
#include "chassis.hpp"
#include "configuration.hpp"
#include "device.hpp"
#include "i2c_interface.hpp"
#include "mock_action.hpp"
#include "phase_fault_detection.hpp"
#include "presence_detection.hpp"
#include "rail.hpp"
#include "rule.hpp"

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace phosphor::power::regulators::test_utils
{

namespace fs = std::filesystem;

/**
 * Create an I2CInterface object with hard-coded bus and address values.
 *
 * @return I2CInterface object wrapped in a unique_ptr
 */
inline std::unique_ptr<i2c::I2CInterface> createI2CInterface()
{
    return i2c::create(1, 0x70, i2c::I2CInterface::InitialState::CLOSED);
}

/**
 * Creates a Device object with the specified ID.
 *
 * Creates Rail objects within the Device if railIDs is specified.
 *
 * @param id device ID
 * @param railIDs rail IDs (optional)
 * @return Device object
 */
inline std::unique_ptr<Device> createDevice(
    const std::string& id, const std::vector<std::string>& railIDs = {})
{
    // Create Rails (if any)
    std::vector<std::unique_ptr<Rail>> rails{};
    for (const std::string& railID : railIDs)
    {
        rails.emplace_back(std::make_unique<Rail>(railID));
    }

    // Create Device
    bool isRegulator = true;
    std::string fru =
        "/xyz/openbmc_project/inventory/system/chassis/motherboard/reg1";
    std::unique_ptr<i2c::I2CInterface> i2cInterface = createI2CInterface();
    std::unique_ptr<PresenceDetection> presenceDetection{};
    std::unique_ptr<Configuration> configuration{};
    std::unique_ptr<PhaseFaultDetection> phaseFaultDetection{};
    return std::make_unique<Device>(
        id, isRegulator, fru, std::move(i2cInterface),
        std::move(presenceDetection), std::move(configuration),
        std::move(phaseFaultDetection), std::move(rails));
}

/**
 * Creates a Rule object with the specified ID.
 *
 * @param id rule ID
 * @return Rule object
 */
inline std::unique_ptr<Rule> createRule(const std::string& id)
{
    // Create actions
    std::vector<std::unique_ptr<Action>> actions{};
    actions.emplace_back(std::make_unique<MockAction>());

    // Create Rule
    return std::make_unique<Rule>(id, std::move(actions));
}

/**
 * Modify the specified file so that fs::remove() fails with an exception.
 *
 * The file will be renamed and can be restored by calling makeFileRemovable().
 *
 * @param path path to the file
 */
inline void makeFileUnRemovable(const fs::path& path)
{
    // Rename the file to save its contents
    fs::path savePath{path.native() + ".save"};
    fs::rename(path, savePath);

    // Create a directory at the original file path
    fs::create_directory(path);

    // Create a file within the directory.  fs::remove() will throw an exception
    // if the path is a non-empty directory.
    std::ofstream childFile{path / "childFile"};
}

/**
 * Modify the specified file so that fs::remove() can successfully delete it.
 *
 * Undo the modifications from an earlier call to makeFileUnRemovable().
 *
 * @param path path to the file
 */
inline void makeFileRemovable(const fs::path& path)
{
    // makeFileUnRemovable() creates a directory at the file path.  Remove the
    // directory and all of its contents.
    fs::remove_all(path);

    // Rename the file back to the original path to restore its contents
    fs::path savePath{path.native() + ".save"};
    fs::rename(savePath, path);
}

} // namespace phosphor::power::regulators::test_utils
