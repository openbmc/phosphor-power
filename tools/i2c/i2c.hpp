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
     * @param[in] size - The size of data to read, only 1 or 2 are supported
     * @param[in] pec - Whether or not enable packet error checking
     *
     * @return - The data read from the i2c device
     */
    bool read(uint8_t addr, uint8_t size, int32_t& data,
              bool pec = false) override;

    /* @brief Write data to i2c
     *
     * @param[in] addr - The register address of the i2c device
     * @param[in] size - The size of data to read
     * @param[in] data - The data to write to the i2c device
     * @param[in] pec - Whether or not enable packet error checking
     *
     * @return true if write is successful, false otherwise
     */
    bool write(uint8_t addr, uint8_t size, const uint8_t* data,
               bool pec = false) override;

    /* @brief Create an I2CInterface instance
     *
     * @param[in] busId - The i2c bus ID
     * @param[in] devAddr - The device address of the i2c
     *
     * @return The unique_ptr holding the I2CInterface
     */
    static std::unique_ptr<I2CInterface> Create(uint8_t busId, uint8_t devAddr);

    /* @brief Check i2c adapter functionality
     *
     * Check if the i2c adapter has the functionality specified by the SMBus
     * transaction type
     *
     * @param[in] type - The SMBus transaction type defined in linux/i2c.h
     * @param[in] pec - Indication if SMBus PEC (packet error checking) is used
     *
     * @return true if it supports the functionality, false otherwise
     */
    bool checkFuncs(int size, bool pec = false);

    /* @brief Enable or disable PEC (packet error checking)
     *
     * @param[in] - Indicate whether to enable or disable
     *
     * @return true if the operation succeeds, false otherwise
     */
    bool setPEC(bool enable);
};

} // namespace i2c
