#pragma once

#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

namespace i2c
{

class I2CException : public std::system_error
{
  public:
    explicit I2CException(const std::string& bus, uint8_t addr, int errorCode) :
        bus(bus), addr(addr), errorCode(errorCode)
    {
        std::stringstream ss;
        ss << "I2CException: bus " << bus << ", addr 0x" << std::hex
           << static_cast<int>(addr) << std::dec << ", errno " << errorCode
           << ": " << strerror(errorCode);
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

    /* @brief Read byte data from i2c
     *
     * @param[in] addr - The register address of the i2c device
     * @param[out] data - The data read from the i2c device
     *
     * @throw I2CException on error
     */
    virtual void read(uint8_t addr, uint8_t& data) = 0;

    /* @brief Read word data from i2c
     *
     * @param[in] addr - The register address of the i2c device
     * @param[out] data - The data read from the i2c device
     *
     * @throw I2CException on error
     */
    virtual void read(uint8_t addr, uint16_t& data) = 0;

    /* @brief Read block data from i2c
     *
     * @param[in] addr - The register address of the i2c device
     * @param[in] size - The size of data to read
     * @param[out] data - The pointer to buffer to read from the i2c device,
     *                    it shall gurantee enough size.
     *
     * @throw I2CException on error
     */
    virtual void read(uint8_t addr, uint8_t size, uint8_t* data) = 0;

    /* @brief Write byte data to i2c
     *
     * @param[in] addr - The register address of the i2c device
     * @param[in] data - The data to write to the i2c device
     *
     * @throw I2CException on error
     */
    virtual void write(uint8_t addr, uint8_t data) = 0;

    /* @brief Write word data to i2c
     *
     * @param[in] addr - The register address of the i2c device
     * @param[in] data - The data to write to the i2c device
     *
     * @throw I2CException on error
     */
    virtual void write(uint8_t addr, uint16_t data) = 0;

    /* @brief Write block data to i2c
     *
     * @param[in] addr - The register address of the i2c device
     * @param[in] size - The size of data to write
     * @param[in] data - The data to write to the i2c device
     *
     * @throw I2CException on error
     */
    virtual void write(uint8_t addr, uint8_t size, const uint8_t* data) = 0;
};

/* @brief Create an I2CInterface instance
 *
 * @param[in] busId - The i2c bus ID
 * @param[in] devAddr - The device address of the i2c
 *
 * @return The unique_ptr holding the I2CInterface
 */
std::unique_ptr<I2CInterface> create(uint8_t busId, uint8_t devAddr);

} // namespace i2c
