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

    /** @copydoc I2CInterface::read(uint8_t&) */
    void read(uint8_t& data) override;

    /** @copydoc I2CInterface::read(uint8_t,uint8_t&) */
    void read(uint8_t addr, uint8_t& data) override;

    /** @copydoc I2CInterface::read(uint8_t,uint16_t&) */
    void read(uint8_t addr, uint16_t& data) override;

    /** @copydoc I2CInterface::read(uint8_t,uint8_t&,uint8_t*) */
    void read(uint8_t addr, uint8_t& size, uint8_t* data) override;

    /** @copydoc I2CInterface::write(uint8_t) */
    void write(uint8_t data) override;

    /** @copydoc I2CInterface::write(uint8_t,uint8_t) */
    void write(uint8_t addr, uint8_t data) override;

    /** @copydoc I2CInterface::write(uint8_t,uint16_t) */
    void write(uint8_t addr, uint16_t data) override;

    /** @copydoc I2CInterface::write(uint8_t,uint8_t,const uint8_t*) */
    void write(uint8_t addr, uint8_t size, const uint8_t* data) override;

    /** @brief Create an I2CInterface instance
     *
     * @param[in] busId - The i2c bus ID
     * @param[in] devAddr - The device address of the i2c
     *
     * @return The unique_ptr holding the I2CInterface
     */
    static std::unique_ptr<I2CInterface> create(uint8_t busId, uint8_t devAddr);
};

} // namespace i2c
