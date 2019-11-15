# BME680 driver test

## About
This is a test application for the BME680 driver. This driver depends on the Bosch Sensortech driver
(see https://github.com/BoschSensortec/BME680_driver)

## Usage
This test application will initialize BME680 device to output the following:

* Temperature
* Humidity
* Pressure
* Resistance value (depending on VOC gas)

## Interface
BME680 can be used with I2C or SPI. Selection is done by selecting the interface
in the Makefile application
