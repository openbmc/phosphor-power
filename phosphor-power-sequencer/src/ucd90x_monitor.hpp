#pragma once

#include "pmbus.hpp"
#include "power_sequencer_monitor.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>

#include <cstdint>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace phosphor::power::sequencer
{

struct Pin
{
    std::string name;
    unsigned int line;
    std::string presence;
};

struct Rail
{
    std::string name;
    std::string presence;
};

/**
 * @class UCD90xMonitor
 * Define a base class for monitoring the UCD90* family of power sequencer
 * devices.
 */
class UCD90xMonitor : public PowerSequencerMonitor
{
  public:
    UCD90xMonitor() = delete;
    UCD90xMonitor(const UCD90xMonitor&) = delete;
    UCD90xMonitor& operator=(const UCD90xMonitor&) = delete;
    UCD90xMonitor(UCD90xMonitor&&) = delete;
    UCD90xMonitor& operator=(UCD90xMonitor&&) = delete;
    virtual ~UCD90xMonitor() = default;

    /**
     * Create a base object for UCD90* monitoring.
     * @param bus D-Bus bus object
     * @param i2cBus The bus number of the power sequencer device
     * @param i2cAddress The I2C address of the power sequencer device
     * @param deviceName The name of the device
     * @param numberPages The number of pages the PMBus device supports
     */
    UCD90xMonitor(sdbusplus::bus_t& bus, std::uint8_t i2cBus,
                  std::uint16_t i2cAddress, const std::string& deviceName,
                  size_t numberPages);

    /**
     * Callback function to handle interfacesAdded D-Bus signals
     * @param msg Expanded sdbusplus message data
     */
    void interfacesAddedHandler(sdbusplus::message_t& msg);

    /** @copydoc PowerSequencerMonitor::onFailure() */
    void onFailure(bool timeout, const std::string& powerSupplyError) override;

  protected:
    /**
     * Formats the GPIO values read from the device.
     * @param values List of GPIO values
     * @param numberLines Number of GPIO lines
     * @param additionalData AdditionalData property of the error log entry
     */
    virtual void formatGpioValues(
        const std::vector<int>& values, unsigned int numberLines,
        std::map<std::string, std::string>& additionalData) const;

  private:
    /**
     * Device name
     */
    std::string deviceName;

    /**
     * The match to Entity Manager interfaces added.
     */
    sdbusplus::bus::match_t match;

    /**
     * The number of pages the PMBus device supports
     */
    size_t numberPages;

    /**
     * List of pins
     */
    std::vector<Pin> pins;

    /**
     * The read/write interface to this hardware
     */
    pmbus::PMBus pmbusInterface;

    /**
     * List of rails
     */
    std::vector<Rail> rails;

    /**
     * Finds the list of compatible system types using D-Bus methods.
     * This list is used to find the correct JSON configuration file for the
     * current system.
     */
    void findCompatibleSystemTypes();

    /**
     * Finds the JSON configuration file.
     * Looks for a configuration file based on the list of compatible system
     * types.
     * Throws an exception if an operating system error occurs while checking
     * for the existance of a file.
     * @param compatibleSystemTypes List of compatible system types
     */
    void findConfigFile(const std::vector<std::string>& compatibleSystemTypes);

    /**
     * Returns whether the hardware with the specified inventory path is
     * present.
     * If an error occurs while obtaining the presence value, presence is
     * assumed to be false. An empty string path indicates no presence check is
     * needed.
     * @param inventoryPath D-Bus inventory path of the hardware
     * @return true if hardware is present, false otherwise
     */
    bool isPresent(const std::string& inventoryPath);

    /**
     * Analyzes the device pins for errors when the device is known to be in an
     * error state.
     * @param message Message property of the error log entry
     * @param additionalData AdditionalData property of the error log entry
     */
    void onFailureCheckPins(std::string& message,
                            std::map<std::string, std::string>& additionalData);

    /**
     * Analyzes the device rails for errors when the device is known to be in an
     * error state.
     * @param message Message property of the error log entry
     * @param additionalData AdditionalData property of the error log entry
     * @param powerSupplyError The power supply error to log. A default
     * std:string, i.e. empty string (""), is passed when there is no power
     * supply error to log.
     */
    void onFailureCheckRails(std::string& message,
                             std::map<std::string, std::string>& additionalData,
                             const std::string& powerSupplyError);

    /**
     * Parse the JSON configuration file.
     * @param pathName the path name
     */
    void parseConfigFile(const std::filesystem::path& pathName);

    /**
     * Reads the mfr_status register
     * @return the register contents
     */
    uint64_t readMFRStatus();

    /**
     * Reads the status_word register
     * @return the register contents
     */
    uint16_t readStatusWord();
};

} // namespace phosphor::power::sequencer
