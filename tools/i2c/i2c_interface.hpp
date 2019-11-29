#pragma once

#include <cstdint>
#include <cstring>
#include <exception>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace i2c
{

class I2CException : public std::exception
{
  public:
    explicit I2CException(const std::string& info, const std::string& bus,
                          uint8_t addr, int errorCode = 0) :
        bus(bus),
        addr(addr), errorCode(errorCode)
    {
        std::stringstream ss;
        ss << "I2CException: " << info << ": bus " << bus << ", addr 0x"
           << std::hex << static_cast<int>(addr);
        if (errorCode != 0)
        {
            ss << std::dec << ", errno " << errorCode << ": "
               << strerror(errorCode);
        }
        errStr = ss.str();
    }
    virtual ~I2CException() = default;

    const char* what() const noexcept override
    {
        return errStr.c_str();
    }
    std::string bus;
    uint8_t addr;
    int errorCode;
    std::string errStr;
};

class I2CInterface
{
  public:
    virtual ~I2CInterface() = default;

    /** @brief The block transaction mode */
    enum class Mode
    {
        SMBUS,
        I2C,
    };

    /** @brief Read byte data from i2c
     *
     * @param[out] data - The data read from the i2c device
     *
     * @throw I2CException on error
     */
    virtual void read(uint8_t& data) = 0;

    /** @brief Read byte data from i2c
     *
     * @param[in] addr - The register address of the i2c device
     * @param[out] data - The data read from the i2c device
     *
     * @throw I2CException on error
     */
    virtual void read(uint8_t addr, uint8_t& data) = 0;

    /** @brief Read word data from i2c
     *
     * @param[in] addr - The register address of the i2c device
     * @param[out] data - The data read from the i2c device
     *
     * @throw I2CException on error
     */
    virtual void read(uint8_t addr, uint16_t& data) = 0;

    /** @brief Read block data from i2c
     *
     * @param[in] addr - The register address of the i2c device
     * @param[in,out] size - The out parameter to indicate the size of data read
     *                       from i2c device; when mode is I2C, it is also the
     *                       input parameter to indicate how many bytes to read
     * @param[out] data - The pointer to buffer to read from the i2c device,
     *                    the buffer shall be big enough to hold the data
     *                    returned by the device. SMBus allows at most 32
     *                    bytes.
     * @param[in] mode - The block read mode, either SMBus or I2C.
     *                   Internally, it invokes functions from libi2c:
     *                    * For SMBus: i2c_smbus_read_block_data()
     *                    * For I2C: i2c_smbus_read_i2c_block_data()
     *
     * @throw I2CException on error
     */
    virtual void read(uint8_t addr, uint8_t& size, uint8_t* data,
                      Mode mode = Mode::SMBUS) = 0;

    /** @brief Write byte data to i2c
     *
     * @param[in] data - The data to write to the i2c device
     *
     * @throw I2CException on error
     */
    virtual void write(uint8_t data) = 0;

    /** @brief Write byte data to i2c
     *
     * @param[in] addr - The register address of the i2c device
     * @param[in] data - The data to write to the i2c device
     *
     * @throw I2CException on error
     */
    virtual void write(uint8_t addr, uint8_t data) = 0;

    /** @brief Write word data to i2c
     *
     * @param[in] addr - The register address of the i2c device
     * @param[in] data - The data to write to the i2c device
     *
     * @throw I2CException on error
     */
    virtual void write(uint8_t addr, uint16_t data) = 0;

    /** @brief Write block data to i2c
     *
     * @param[in] addr - The register address of the i2c device
     * @param[in] size - The size of data to write, SMBus allows at most 32
     *                   bytes
     * @param[in] data - The data to write to the i2c device
     * @param[in] mode - The block write mode, either SMBus or I2C.
     *                   Internally, it invokes functions from libi2c:
     *                    * For SMBus: i2c_smbus_write_block_data()
     *                    * For I2C: i2c_smbus_write_i2c_block_data()
     *
     * @throw I2CException on error
     */
    virtual void write(uint8_t addr, uint8_t size, const uint8_t* data,
                       Mode mode = Mode::SMBUS) = 0;
};

/** @brief Create an I2CInterface instance
 *
 * @param[in] busId - The i2c bus ID
 * @param[in] devAddr - The device address of the i2c
 *
 * @return The unique_ptr holding the I2CInterface
 */
std::unique_ptr<I2CInterface> create(uint8_t busId, uint8_t devAddr);

} // namespace i2c
