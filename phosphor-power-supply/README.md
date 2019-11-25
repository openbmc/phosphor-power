OpenBMC power supply monitoring application.

Design document: https://github.com/openbmc/docs/blob/master/designs/psu-monitoring.md

# JSON Configuration File

The JSON configuration file should contain:

## pollInterval
Required property that specifies the delay between updating the power supply 
status, period in milliseconds.

