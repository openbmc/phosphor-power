#pragma once

#include "i2c_interface.hpp"

namespace i2c
{

class I2CDevice : public I2CInterface
{
  private:
    I2CDevice() = delete;

    explicit I2CDevice(uint8_t busId, uint8_t devAddr)
    {
        // TODO
        (void)busId;
        (void)devAddr;
    }

  public:
    virtual ~I2CDevice() = default;

    /* @brief Read byte data from i2c
     *
     * @param[out] data - The data read from the i2c device
     *
     * @throw I2CException on error
     */
    void read(uint8_t& data) override;

    /* @brief Read byte data from i2c
     *
     * @param[in] addr - The register address of the i2c device
     * @param[out] data - The data read from the i2c device
     *
     * @throw I2CException on error
     */
    void read(uint8_t addr, uint8_t& data) override;

    /* @brief Read word data from i2c
     *
     * @param[in] addr - The register address of the i2c device
     * @param[out] data - The data read from the i2c device
     *
     * @throw I2CException on error
     */
    void read(uint8_t addr, uint16_t& data) override;

    /* @brief Read block data from i2c
     *
     * @param[in] addr - The register address of the i2c device
     * @param[out] size - The size of data read from i2c device
     * @param[out] data - The pointer to buffer to read from the i2c device,
     *                    the buffer shall be big enough to hold the data
     *                    returned by the device. SMBus allows at most 32
     *                    bytes.
     *
     * @throw I2CException on error
     */
    void read(uint8_t addr, uint8_t& size, uint8_t* data) override;

    /* @brief Write byte data to i2c
     *
     * @param[in] data - The data to write to the i2c device
     *
     * @throw I2CException on error
     */
    void write(uint8_t data) override;

    /* @brief Write byte data to i2c
     *
     * @param[in] addr - The register address of the i2c device
     * @param[in] data - The data to write to the i2c device
     *
     * @throw I2CException on error
     */
    void write(uint8_t addr, uint8_t data) override;

    /* @brief Write word data to i2c
     *
     * @param[in] addr - The register address of the i2c device
     * @param[in] data - The data to write to the i2c device
     *
     * @throw I2CException on error
     */
    void write(uint8_t addr, uint16_t data) override;

    /* @brief Write block data to i2c
     *
     * @param[in] addr - The register address of the i2c device
     * @param[in] size - The size of data to write
     * @param[in] data - The data to write to the i2c device
     *
     * @throw I2CException on error
     */
    void write(uint8_t addr, uint8_t size, const uint8_t* data) override;

    /* @brief Create an I2CInterface instance
     *
     * @param[in] busId - The i2c bus ID
     * @param[in] devAddr - The device address of the i2c
     *
     * @return The unique_ptr holding the I2CInterface
     */
    static std::unique_ptr<I2CInterface> create(uint8_t busId, uint8_t devAddr);
};

} // namespace i2c
