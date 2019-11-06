#pragma once

#include "i2c_interface.hpp"

namespace i2c
{

class I2CDevice : public I2CInterface
{
  private:
    I2CDevice() = delete;

    /* @brief Constructor
     *
     * Construct I2CDevice object from the bus id and device address
     *
     * @param[in] busId - The i2c bus ID
     * @param[in] devAddr - The device address of the i2c
     */
    explicit I2CDevice(uint8_t busId, uint8_t devAddr) :
        busId(busId), devAddr(devAddr)
    {
        busStr = "/dev/i2c-" + std::to_string(busId);
        open();
    }

    /** @brief Open the i2c device */
    void open();

    /** @brief Close the i2c device */
    void close();

    /** @brief The i2c device id */
    uint8_t busId;

    /** @brief The i2c device address in the bus */
    uint8_t devAddr;

    /** @brief The file descriptor of the opened i2c device */
    int fd;

    /** @brief The i2c bus path in /dev */
    std::string busStr;

  public:
    ~I2CDevice()
    {
        close();
    }

    /* @brief Read data from i2c
     *
     * @param[in] addr - The register address of the i2c device
     * @param[in] size - The size of data to read
     * @param[out] data - The data read from the i2c device
     */
    void read(uint8_t addr, uint8_t size, std::vector<uint8_t>& data) override;

    /* @brief Write data to i2c
     *
     * @param[in] addr - The register address of the i2c device
     * @param[in] size - The size of data to read
     * @param[in] data - The data to write to the i2c device
     *
     * @return true if write is successful, false otherwise
     */
    bool write(uint8_t addr, uint8_t size, const uint8_t* data) override;

    /* @brief Create an I2CInterface instance
     *
     * @param[in] busId - The i2c bus ID
     * @param[in] devAddr - The device address of the i2c
     *
     * @return The unique_ptr holding the I2CInterface
     */
    static std::unique_ptr<I2CInterface> Create(uint8_t busId, uint8_t devAddr);
};

} // namespace i2c
