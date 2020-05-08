
By Blake Felt - blake.w.felt@gmail.com

ESP32 ADS1115
=============

A component for TI ADS1115 on ESP-IDF. For an example, see https://github.com/Molorius/ESP32-Examples.

To add to a project, type:
`git submodule add https://github.com/Molorius/esp32-ads1115.git components/ads1115`
into the base directory of your project.

The datasheet can be found [here](http://www.ti.com/lit/ds/symlink/ads1115.pdf).

This has not been tested with more than one ADS1115 connected.
Any suggestions or fixes are gladly appreciated.

Table of Contents
=================

* [Enumerations](#enumerations)
  * [ads11115_mux_t](#enum-ads1115_mux_t)
  * [ads1115_fsr_t](#enum-ads1115_fsr_t)
  * [ads1115_sps_t](#enum-ads1115_sps_t)
  * [ads1115_mode_t](#enum-ads1115_mode_t)
* [Functions](#functions)
  * [ads1115_config](#ads1115_t-ads1115_configi2c_port_t-i2c_portuint8_t-address)
  * [ads1115_set_rdy_pin](#void-ads1115_set_rdy_pinads1115_t-adsgpio_num_t-gpio)
  * [ads1115_set_mux](#void-ads1115_set_muxads1115_t-adsads1115_mux_t-mux)
  * [ads1115_set_pga](#void-ads1115_set_pgaads1115_t-adsads1115_fsr_t-fsr)
  * [ads1115_set_mode](#void-ads1115_set_modeads1115_t-adsads1115_mode_t-mode)
  * [ads1115_set_sps](#void-ads1115_set_spsads1115_t-adsads1115_sps_t-sps)
  * [ads1115_set_max_ticks](#void-ads1115_set_max_ticksads1115_t-adsticktype_t-max_ticks)
  * [ads1115_get_raw](#int16_t-ads1115_get_rawads1115_t-ads)
  * [ads1115_get_voltage](#double-ads1115_get_voltageads1115_t-ads)

Enumerations
============

enum ads1115_mux_t
------------------

The different multiplexer options on the ADS1115.

*Values*
  * `ADS1115_MUX_0_1`: Connect pins 0 and 1.
  * `ADS1115_MUX_0_3`: Connect pins 0 and 3.
  * `ADS1115_MUX_1_3`: Connect pins 1 and 3.
  * `ADS1115_MUX_2_3`: Connect pins 2 and 3.
  * `ADS1115_MUX_0_GND`: Connect pin 0 to ground.
  * `ADS1115_MUX_1_GND`: Connect pin 1 to ground.
  * `ADS1115_MUX_2_GND`: Connect pin 2 to ground.
  * `ADS1115_MUX_3_GND`: Connect pin 3 to ground.

enum ads1115_fsr_t
------------------

The different full-scale resolution options.

*Values*
  * `ADS1115_FSR_6_144`: 6.144 volts
  * `ADS1115_FSR_4_096`: 4.096 volts
  * `ADS1115_FSR_2_048`: 2.048 volts
  * `ADS1115_FSR_1_024`: 1.024 volts
  * `ADS1115_FSR_0_512`: 0.512 volts
  * `ADS1115_FSR_0_256`: 0.256 volts

enum ads1115_sps_t
------------------

The different samples per second, or data rate, options.

*Values*
  * `ADS1115_SPS_8`: 8 samples per second
  * `ADS1115_SPS_16`: 16 samples per second
  * `ADS1115_SPS_32`: 32 samples per second
  * `ADS1115_SPS_64`: 64 samples per second
  * `ADS1115_SPS_128`: 128 samples per second
  * `ADS1115_SPS_250`: 250 samples per second
  * `ADS1115_SPS_475`: 475 samples per second
  * `ADS1115_SPS_860`: 860 samples per second

enum ads1115_mode_t
-------------------

Continuous or single-shot mode.

*Values*
  * `ADS1115_MODE_CONTINUOUS`: continuous mode.
  * `ADS1115_MODE_SINGLE`: single-shot mode.

Functions
=========

ads1115_t ads1115_config(i2c_port_t i2c_port,uint8_t address)
-------------------------------------------------------------

Setup of the device.

*Parameters*
  * `i2c_port`: the i2c bus number.
  * `address`: the device's i2c address.

*Returns*
  * The configuration file, which is passed to all subsequent functions.

*Notes*
  * This does not setup the i2c bus, this must be done before passing to this function.

void ads1115_set_rdy_pin(ads1115_t* ads,gpio_num_t gpio)
--------------------------------------------------------

Sets up an optional data-ready pin to verify when conversions are complete.
Connect to ALRT/RDY pin on ADS1115.

*Parameters*
  * `ads`: The configuration file.
  * `gpio`: The esp32 gpio. Do not setup beforehand.

void ads1115_set_mux(ads1115_t* ads,ads1115_mux_t mux)
------------------------------------------------------

Sets the pins to be multiplexed.

*Parameters*
  * `ads`: the configuration file.
  * `mux`: the desired multiplex option (see enumeration).

void ads1115_set_pga(ads1115_t* ads,ads1115_fsr_t fsr)
------------------------------------------------------

Sets the full-scale range, or the programmable-gain amplifier.

*Parameters*
  * `ads`: the configuration file.
  * `fsr`: the desired full-scale range option (see enumeration).

void ads1115_set_mode(ads1115_t* ads,ads1115_mode_t mode)
---------------------------------------------------------

Sets the read mode.

*Paremeters*
  * `ads`: the configuration file.
  * `mode`: the desired mode (see enumeration).

*Notes*
  * To end continuous mode, set it to single-shot mode and make one voltage read.

void ads1115_set_sps(ads1115_t* ads,ads1115_sps_t sps)
------------------------------------------------------

Sets the sampling speed.

*Parameters*
  * `ads`: the configuration file.
  * `sps`: the desired samples per second (see enumeration).

void ads1115_set_max_ticks(ads1115_t* ads,TickType_t max_ticks)
---------------------------------------------------------------

Sets the maximum wait ticks for the i2c reads and writes. See the [i2c documentation](http://esp-idf.readthedocs.io/en/latest/api-reference/peripherals/i2c.html#_CPPv220i2c_master_cmd_begin10i2c_port_t16i2c_cmd_handle_t10TickType_t).

*Parameters*
  * `ads`: the configuration file.
  * `max_ticks`: maximum wait ticks.

int16_t ads1115_get_raw(ads1115_t* ads)
---------------------------------------

Reads the voltage based on the current configuration.

*Parameters*
  * `ads`: the configuration file.

*Returns*
  * The 16 bit raw voltage value directly from the ADS1115.

double ads1115_get_voltage(ads1115_t* ads)
------------------------------------------

Reads the voltage based on the current configuration.

*Parameters*
  * `ads`: the configuration file.

*Returns*
  * The voltage, based on the current full-scale range. This is just a conversion from the raw value.
