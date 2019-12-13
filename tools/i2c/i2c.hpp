#pragma once

#include "i2c_interface.hpp"

namespace i2c
{

class I2CDevice : public I2CInterface
{
  private:
    I2CDevice() = delete;

    /** @brief Constructor
     *
     * Construct I2CDevice object from the bus id and device address
     *
     * Automatically opens the I2CDevice if initialState is OPEN.
     *
     * @param[in] busId - The i2c bus ID
     * @param[in] devAddr - The device address of the I2C device
     * @param[in] initialState - Initial state of the I2CDevice object
     */
    explicit I2CDevice(uint8_t busId, uint8_t devAddr,
                       InitialState initialState = InitialState::OPEN) :
        busId(busId),
        devAddr(devAddr), fd(INVALID_FD)
    {
        busStr = "/dev/i2c-" + std::to_string(busId);
        if (initialState == InitialState::OPEN)
        {
            open();
        }
    }

    /** @brief Invalid file descriptor */
    static constexpr int INVALID_FD = -1;

    /** @brief The I2C bus ID */
    uint8_t busId;

    /** @brief The i2c device address in the bus */
    uint8_t devAddr;

    /** @brief The file descriptor of the opened i2c device */
    int fd;

    /** @brief The i2c bus path in /dev */
    std::string busStr;

    /** @brief Check that device interface is open
     *
     * @throw I2CException if device is not open
     */
    void checkIsOpen() const
    {
        if (!isOpen())
        {
            throw I2CException("Device not open", busStr, devAddr);
        }
    }

    /** @brief Close device without throwing an exception if an error occurs */
    void closeWithoutException() noexcept
    {
        try
        {
            close();
        }
        catch (...)
        {}
    }

    /** @brief Check i2c adapter read functionality
     *
     * Check if the i2c adapter has the functionality specified by the SMBus
     * transaction type
     *
     * @param[in] type - The SMBus transaction type defined in linux/i2c.h
     *
     * @throw I2CException if the function is not supported
     */
    void checkReadFuncs(int type);

    /** @brief Check i2c adapter write functionality
     *
     * Check if the i2c adapter has the functionality specified by the SMBus
     * transaction type
     *
     * @param[in] type - The SMBus transaction type defined in linux/i2c.h
     *
     * @throw I2CException if the function is not supported
     */
    void checkWriteFuncs(int type);

  public:
    /** @copydoc I2CInterface::~I2CInterface() */
    ~I2CDevice()
    {
        if (isOpen())
        {
            // Note: destructors must not throw exceptions
            closeWithoutException();
        }
    }

    /** @copydoc I2CInterface::open() */
    void open();

    /** @copydoc I2CInterface::isOpen() */
    bool isOpen() const
    {
        return (fd != INVALID_FD);
    }

    /** @copydoc I2CInterface::close() */
    void close();

    /** @copydoc I2CInterface::read(uint8_t&) */
    void read(uint8_t& data) override;

    /** @copydoc I2CInterface::read(uint8_t,uint8_t&) */
    void read(uint8_t addr, uint8_t& data) override;

    /** @copydoc I2CInterface::read(uint8_t,uint16_t&) */
    void read(uint8_t addr, uint16_t& data) override;

    /** @copydoc I2CInterface::read(uint8_t,uint8_t&,uint8_t*,Mode) */
    void read(uint8_t addr, uint8_t& size, uint8_t* data,
              Mode mode = Mode::SMBUS) override;

    /** @copydoc I2CInterface::write(uint8_t) */
    void write(uint8_t data) override;

    /** @copydoc I2CInterface::write(uint8_t,uint8_t) */
    void write(uint8_t addr, uint8_t data) override;

    /** @copydoc I2CInterface::write(uint8_t,uint16_t) */
    void write(uint8_t addr, uint16_t data) override;

    /** @copydoc I2CInterface::write(uint8_t,uint8_t,const uint8_t*,Mode) */
    void write(uint8_t addr, uint8_t size, const uint8_t* data,
               Mode mode = Mode::SMBUS) override;

    /** @brief Create an I2CInterface instance
     *
     * Automatically opens the I2CInterface if initialState is OPEN.
     *
     * @param[in] busId - The i2c bus ID
     * @param[in] devAddr - The device address of the i2c
     * @param[in] initialState - Initial state of the I2CInterface object
     *
     * @return The unique_ptr holding the I2CInterface
     */
    static std::unique_ptr<I2CInterface>
        create(uint8_t busId, uint8_t devAddr,
               InitialState initialState = InitialState::OPEN);
};

} // namespace i2c
