#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace phosphor
{
namespace pmbus
{

namespace fs = std::filesystem;

// The file name Linux uses to capture the READ_VIN from pmbus.
constexpr auto READ_VIN = "in1_input";

// The file name Linux uses to capture the MFR_POUT_MAX from pmbus.
constexpr auto MFR_POUT_MAX = "max_power_out";
// The max_power_out value expected to be read for 1400W IBM CFFPS type.
constexpr auto IBM_CFFPS_1400W = 30725;

namespace in_input
{
// VIN thresholds in Volts
constexpr auto VIN_VOLTAGE_MIN = 20;
constexpr auto VIN_VOLTAGE_110_THRESHOLD = 160;

// VIN actual values in Volts
// VIN_VOLTAGE_0:   VIN < VIN_VOLTAGE_MIN
// VIN_VOLTAGE_110: VIN_VOLTAGE_MIN < VIN < VIN_VOLTAGE_110_THRESHOLD
// VIN_VOLTAGE_220: VIN_VOLTAGE_110_THRESHOLD < VIN
constexpr auto VIN_VOLTAGE_0 = 0;
constexpr auto VIN_VOLTAGE_110 = 110;
constexpr auto VIN_VOLTAGE_220 = 220;
} // namespace in_input

// The file name Linux uses to capture the STATUS_WORD from pmbus.
constexpr auto STATUS_WORD = "status0";

// The file name Linux uses to capture the STATUS_INPUT from pmbus.
constexpr auto STATUS_INPUT = "status0_input";

// Voltage out status.
// Overvoltage fault or warning, Undervoltage fault or warning, maximum or
// minimum warning, ....
// Uses Page substitution
constexpr auto STATUS_VOUT = "statusP_vout";

namespace status_vout
{
// Mask of bits that are only warnings
constexpr auto WARNING_MASK = 0x6A;
} // namespace status_vout

// Current output status bits.
constexpr auto STATUS_IOUT = "status0_iout";

// Manufacturing specific status bits
constexpr auto STATUS_MFR = "status0_mfr";

// Reports on the status of any fans installed in position 1 and 2.
constexpr auto STATUS_FANS_1_2 = "status0_fan12";

// Reports on temperature faults or warnings. Overtemperature fault,
// overtemperature warning, undertemperature warning, undertemperature fault.
constexpr auto STATUS_TEMPERATURE = "status0_temp";

// Reports on the communication, memory, logic fault(s).
constexpr auto STATUS_CML = "status0_cml";

namespace status_word
{
constexpr auto VOUT_FAULT = 0x8000;

// The IBM CFF power supply driver does map this bit to power1_alarm in the
// hwmon space, but since the other bits that need to be checked do not have
// a similar mapping, the code will just read STATUS_WORD and use bit masking
// to see if the INPUT FAULT OR WARNING bit is on.
constexpr auto INPUT_FAULT_WARN = 0x2000;

// The bit mask representing the MFRSPECIFIC fault, bit 4 of STATUS_WORD high
// byte. A manufacturer specific fault or warning has occurred.
constexpr auto MFR_SPECIFIC_FAULT = 0x1000;

// The bit mask representing the POWER_GOOD Negated bit of the STATUS_WORD.
constexpr auto POWER_GOOD_NEGATED = 0x0800;

// The bit mask representing the FAN FAULT or WARNING bit of the STATUS_WORD.
// Bit 2 of the high byte of STATUS_WORD.
constexpr auto FAN_FAULT = 0x0400;

// The bit mask representing the UNITI_IS_OFF bit of the STATUS_WORD.
constexpr auto UNIT_IS_OFF = 0x0040;

// Bit 5 of the STATUS_BYTE, or lower byte of STATUS_WORD is used to indicate
// an output overvoltage fault.
constexpr auto VOUT_OV_FAULT = 0x0020;

// The bit mask representing that an output overcurrent fault has occurred.
constexpr auto IOUT_OC_FAULT = 0x0010;

// The IBM CFF power supply driver does map this bit to in1_alarm, however,
// since a number of the other bits are not mapped that way for STATUS_WORD,
// this code will just read the entire STATUS_WORD and use bit masking to find
// out if that fault is on.
constexpr auto VIN_UV_FAULT = 0x0008;

// The bit mask representing the TEMPERATURE FAULT or WARNING bit of the
// STATUS_WORD. Bit 2 of the low byte (STATUS_BYTE).
constexpr auto TEMPERATURE_FAULT_WARN = 0x0004;

// The bit mask representing the CML (Communication, Memory, and/or Logic) fault
// bit of the STATUS_WORD. Bit 1 of the low byte (STATUS_BYTE).
constexpr auto CML_FAULT = 0x0002;
} // namespace status_word

namespace status_vout
{
// The IBM CFF power supply driver maps MFR's OV_FAULT and VAUX_FAULT to this
// bit.
constexpr auto OV_FAULT = 0x80;

// The IBM CFF power supply driver maps MFR's UV_FAULT to this bit.
constexpr auto UV_FAULT = 0x10;
} // namespace status_vout

namespace status_temperature
{
// Overtemperature Fault
constexpr auto OT_FAULT = 0x80;
} // namespace status_temperature

constexpr auto ON_OFF_CONFIG = "on_off_config";

// From PMBus Specification Part II Revsion 1.2:
// The ON_OFF_CONFIG command configures the combination of CONTROL pin input
// and serial bus commands needed to turn the unit on and off. This includes how
// the unit responds when power is applied.
// Bits [7:5] - 000 - Reserved
// Bit 4 - 1 - Unit does not power up until commanded by the CONTROL pin and
// OPERATION command (as programmed in bits [3:0]).
// Bit 3 - 0 - Unit ignores the on/off portion of the OPERATION command from
// serial bus.
// Bit 2 - 1 - Unit requires the CONTROL pin to be asserted to start the unit.
// Bit 1 - 0 - Polarity of the CONTROL pin. Active low (Pull pin low to start
// the unit).
// Bit 0 - 1 - Turn off the output and stop transferring energy to the output as
// fast as possible.
constexpr auto ON_OFF_CONFIG_CONTROL_PIN_ONLY = 0x15;

/**
 * Where the access should be done
 */
enum class Type
{
    Base,            // base device directory
    Hwmon,           // hwmon directory
    Debug,           // pmbus debug directory
    DeviceDebug,     // device debug directory
    HwmonDeviceDebug // hwmon device debug directory
};

/**
 * @class PMBusBase
 *
 * This is a base class for PMBus to assist with unit testing via mocking.
 */
class PMBusBase
{
  public:
    virtual ~PMBusBase() = default;

    virtual uint64_t read(const std::string& name, Type type,
                          bool errTrace = true) = 0;
    virtual std::string readString(const std::string& name, Type type) = 0;
    virtual std::vector<uint8_t> readBinary(const std::string& name, Type type,
                                            size_t length) = 0;
    virtual void writeBinary(const std::string& name, std::vector<uint8_t> data,
                             Type type) = 0;
    virtual void findHwmonDir() = 0;
    virtual const fs::path& path() const = 0;
    virtual std::string insertPageNum(const std::string& templateName,
                                      size_t page) = 0;
    virtual fs::path getPath(Type type) = 0;
};

/**
 * Wrapper function for PMBus
 *
 * @param[in] bus - I2C bus
 * @param[in] address - I2C address (as a 2-byte string, e.g. 0069)
 *
 * @return PMBusBase pointer
 */
std::unique_ptr<PMBusBase> createPMBus(std::uint8_t bus,
                                       const std::string& address);

/**
 * @class PMBus
 *
 * This class is an interface to communicating with PMBus devices
 * by reading and writing sysfs files.
 *
 * Based on the Type parameter, the accesses can either be done
 * in the base device directory (the one passed into the constructor),
 * or in the hwmon directory for the device.
 */
class PMBus : public PMBusBase
{
  public:
    PMBus() = delete;
    virtual ~PMBus() = default;
    PMBus(const PMBus&) = default;
    PMBus& operator=(const PMBus&) = default;
    PMBus(PMBus&&) = default;
    PMBus& operator=(PMBus&&) = default;

    /**
     * Constructor
     *
     * @param[in] path - path to the sysfs directory
     */
    PMBus(const std::string& path) : basePath(path)
    {
        findHwmonDir();
    }

    /**
     * Constructor
     *
     * This version is required when DeviceDebug
     * access will be used.
     *
     * @param[in] path - path to the sysfs directory
     * @param[in] driverName - the device driver name
     * @param[in] instance - chip instance number
     */
    PMBus(const std::string& path, const std::string& driverName,
          size_t instance) :
        basePath(path), driverName(driverName), instance(instance)
    {
        findHwmonDir();
    }

    /**
     * Wrapper function for PMBus
     *
     * @param[in] bus - I2C bus
     * @param[in] address - I2C address (as a 2-byte string, e.g. 0069)
     *
     * @return PMBusBase pointer
     */
    static std::unique_ptr<PMBusBase>
        createPMBus(std::uint8_t bus, const std::string& address);

    /**
     * Reads a file in sysfs that represents a single bit,
     * therefore doing a PMBus read.
     *
     * @param[in] name - path concatenated to
     *                   basePath to read
     * @param[in] type - Path type
     *
     * @return bool - false if result was 0, else true
     */
    bool readBit(const std::string& name, Type type);

    /**
     * Reads a file in sysfs that represents a single bit,
     * where the page number passed in is substituted
     * into the name in place of the 'P' character in it.
     *
     * @param[in] name - path concatenated to
     *                   basePath to read
     * @param[in] page - page number
     * @param[in] type - Path type
     *
     * @return bool - false if result was 0, else true
     */
    bool readBitInPage(const std::string& name, size_t page, Type type);
    /**
     * Checks if the file for the given name and type exists.
     *
     * @param[in] name   - path concatenated to basePath to read
     * @param[in] type   - Path type
     *
     * @return bool - True if file exists, false if it does not.
     */
    bool exists(const std::string& name, Type type);

    /**
     * Read byte(s) from file in sysfs.
     *
     * @param[in] name   - path concatenated to basePath to read
     * @param[in] type   - Path type
     * @param[in] errTrace - true to enable tracing error (defaults to true)
     *
     * @return uint64_t - Up to 8 bytes of data read from file.
     */
    uint64_t read(const std::string& name, Type type,
                  bool errTrace = true) override;

    /**
     * Read a string from file in sysfs.
     *
     * @param[in] name   - path concatenated to basePath to read
     * @param[in] type   - Path type
     *
     * @return string - The data read from the file.
     */
    std::string readString(const std::string& name, Type type) override;

    /**
     * Read data from a binary file in sysfs.
     *
     * @param[in] name   - path concatenated to basePath to read
     * @param[in] type   - Path type
     * @param[in] length - length of data to read, in bytes
     *
     * @return vector<uint8_t> - The data read from the file.
     */
    std::vector<uint8_t> readBinary(const std::string& name, Type type,
                                    size_t length) override;

    /**
     * Writes an integer value to the file, therefore doing
     * a PMBus write.
     *
     * @param[in] name - path concatenated to
     *                   basePath to write
     * @param[in] value - the value to write
     * @param[in] type - Path type
     */
    void write(const std::string& name, int value, Type type);

    /**
     * Writes binary data to a file in sysfs.
     *
     * @param[in] name - path concatenated to basePath to write
     * @param[in] data - The data to write to the file
     * @param[in] type - Path type
     */
    void writeBinary(const std::string& name, std::vector<uint8_t> data,
                     Type type) override;

    /**
     * Returns the sysfs base path of this device
     */
    const fs::path& path() const override
    {
        return basePath;
    }

    /**
     * Replaces the 'P' in the string passed in with
     * the page number passed in.
     *
     * For example:
     *   insertPageNum("inP_enable", 42)
     *   returns "in42_enable"
     *
     * @param[in] templateName - the name string, with a 'P' in it
     * @param[in] page - the page number to insert where the P was
     *
     * @return string - the new string with the page number in it
     */
    std::string insertPageNum(const std::string& templateName,
                              size_t page) override;

    /**
     * Finds the path relative to basePath to the hwmon directory
     * for the device and stores it in hwmonRelPath.
     */
    void findHwmonDir() override;

    /**
     * Returns the path to use for the passed in type.
     *
     * @param[in] type - Path type
     *
     * @return fs::path - the full path
     */
    fs::path getPath(Type type) override;

  private:
    /**
     * Returns the device name
     *
     * This is found in the 'name' file in basePath.
     *
     * @return string - the device name
     */
    std::string getDeviceName();

    /**
     * The sysfs device path
     */
    fs::path basePath;

    /**
     * The directory name under the basePath hwmon directory
     */
    fs::path hwmonDir;

    /**
     * The device driver name.  Used for finding the device
     * debug directory.  Not required if that directory
     * isn't used.
     */
    std::string driverName;

    /**
     * The device instance number.
     *
     * Used in conjunction with the driver name for finding
     * the debug directory.  Not required if that directory
     * isn't used.
     */
    size_t instance = 0;

    /**
     * The pmbus debug path with status files
     */
    const fs::path debugPath = "/sys/kernel/debug/";
};

} // namespace pmbus
} // namespace phosphor
