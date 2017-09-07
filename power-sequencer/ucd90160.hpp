#pragma once

#include <algorithm>
#include <experimental/filesystem>
#include <map>
#include <vector>
#include "device.hpp"
#include "gpio.hpp"
#include "pmbus.hpp"
#include "types.hpp"

namespace witherspoon
{
namespace power
{

/**
 * @class UCD90160
 *
 * This class implements fault analysis for the UCD90160
 * power sequencer device.
 *
 */
class UCD90160 : public Device
{
    public:

        UCD90160() = delete;
        ~UCD90160() = default;
        UCD90160(const UCD90160&) = delete;
        UCD90160& operator=(const UCD90160&) = delete;
        UCD90160(UCD90160&&) = default;
        UCD90160& operator=(UCD90160&&) = default;

        /**
         * Constructor
         *
         * @param[in] instance - the device instance number
         */
        UCD90160(size_t instance);

        /**
         * Analyzes the device for errors when the device is
         * known to be in an error state.  A log will be created.
         */
        void onFailure() override;

        /**
         * Checks the device for errors and only creates a log
         * if one is found.
         */
        void analyze() override;

        /**
         * Clears faults in the device
         */
        void clearFaults() override
        {
        }

    private:

        /**
         * Finds the GPIO device path for this device
         */
        void findGPIODevice();

        /**
         * Checks for VOUT faults on the device.
         *
         * This device can monitor voltages of its dependent
         * devices, and VOUT faults are voltage faults
         * on these devices.
         *
         * @return bool - true if an error log was created
         */
        bool checkVOUTFaults();

        /**
         * Checks for PGOOD faults on the device.
         *
         * This device can monitor the PGOOD signals of its dependent
         * devices, and this check will look for faults of
         * those PGOODs.
         *
         * @param[in] polling - If this is running while polling for errors,
         *                      as opposing to analyzing a fail condition.
         *
         * @return bool - true if an error log was created
         */
         bool checkPGOODFaults(bool polling);

        /**
         * Creates an error log when the device has an error
         * but it isn't a PGOOD or voltage failure.
         */
        void createPowerFaultLog();

        /**
         * Reads the status_word register
         *
         * @return uint16_t - the register contents
         */
        uint16_t readStatusWord();

        /**
         * Reads the mfr_status register
         *
         * @return uint32_t - the register contents
         */
        uint32_t readMFRStatus();

        /**
         * Says if we've already logged a Vout fault
         *
         * The policy is only 1 of the same error will
         * be logged for the duration of a class instance.
         *
         * @param[in] page - the page to check
         *
         * @return bool - if we've already logged a fault against
         *                this page
         */
        inline bool isVoutFaultLogged(uint32_t page) const
        {
            return std::find(voutErrors.begin(),
                             voutErrors.end(),
                             page) != voutErrors.end();
        }

        /**
         * Saves that a Vout fault has been logged
         *
         * @param[in] page - the page the error was logged against
         */
        inline void setVoutFaultLogged(uint32_t page)
        {
            voutErrors.push_back(page);
        }

        /**
         * Says if we've already logged a PGOOD fault
         *
         * The policy is only 1 of the same errors will
         * be logged for the duration of a class instance.
         *
         * @param[in] input - the input to check
         *
         * @return bool - if we've already logged a fault against
         *                this input
         */
        inline bool isPGOODFaultLogged(uint32_t input) const
        {
            return std::find(pgoodErrors.begin(),
                             pgoodErrors.end(),
                             input) != pgoodErrors.end();
        }

        /**
         * Saves that a PGOOD fault has been logged
         *
         * @param[in] input - the input the error was logged against
         */
        inline void setPGOODFaultLogged(uint32_t input)
        {
            pgoodErrors.push_back(input);
        }

        /**
         * List of pages that Vout errors have
         * already been logged against
         */
        std::vector<uint32_t> voutErrors;

        /**
         * List of inputs that PGOOD errors have
         * already been logged against
         */
        std::vector<uint32_t> pgoodErrors;

        /**
         * The read/write interface to this hardware
         */
        pmbus::PMBus interface;

        /**
         * A map of GPI pin IDs to the GPIO object
         * used to access them
         */
        std::map<size_t, std::unique_ptr<gpio::GPIO>> gpios;

        /**
         * Keeps track of device access errors to avoid repeatedly
         * logging errors for bad hardware
         */
        bool accessError = false;

        /**
         * The path to the GPIO device used to read
         * the GPI (PGOOD) status
         */
        std::experimental::filesystem::path gpioDevice;

        /**
         * Map of device instance to the instance specific data
         */
        static const ucd90160::DeviceMap deviceMap;
};

}
}
