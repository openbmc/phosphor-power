#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace i2c
{

class I2CInterface
{
  public:
    virtual ~I2CInterface() = default;

    /* @brief Read data from i2c
     *
     * @param[in] addr - The register address of the i2c device
     * @param[in] size - The size of data to read
     * @param[out] data - The data read from the i2c device
     *
     * @return true if read is successful, false otherwise
     */
    virtual bool read(uint8_t addr, uint8_t size, int32_t& data) = 0;

    /* @brief Write data to i2c
     *
     * @param[in] addr - The register address of the i2c device
     * @param[in] size - The size of data to read
     * @param[in] data - The data to write to the i2c device
     *
     * @return true if write is successful, false otherwise
     */
    virtual bool write(uint8_t addr, uint8_t size, const uint8_t* data) = 0;
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
