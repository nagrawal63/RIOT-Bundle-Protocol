/*
 * SDP3x.h
 *
 *  Created on: 17.09.2019
 *      Author: Dirk Ehmen
 */

#ifndef SDP3X_H_
#define SDP3X_H_

#include "saul.h"
#include "periph/i2c.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief   SDP3x models
 */
typedef enum {
    SDP3x_31 = 1,
    SDP3x_32 = 2
} sdp3x_model_t;

/**
 * @brief   Device initialization parameters
 */
typedef struct {
    i2c_t i2c_dev;                              /**< I2C device which is used */
    uint8_t i2c_addr;                           /**< I2C address */
    sdp3x_model_t model;                        /**< Model Number */
} sdp3x_params_t;

/**
 * @brief   Device descriptor for the BMP180 sensor
 */
typedef struct {
    sdp3x_params_t      params;                /**< Device initialization parameters */
    uint32_t irq_pin;                           /**< IRQ-Pin */
} sdp3x_t;

typedef struct {
    double differential_pressure;
    double temperature;
    int16_t dp_scale_factor;
} sdp3x_measurement_t;

/**
 * @brief   Status and error return codes
 */
enum {
    SDP3x_OK = 0,             /**< all went as expected */
    SDP3x_CRCERROR = -1,      /**< CRC-Check failed */
    SDP3x_NODATA = -2,        /**< No Data available */
    SDP3x_IOERROR = -3,       /**< I/O error */
    SDP3x_WRONGSIZE = -4      /**< Wrong size of array for method */
};

/** Initialize SPD3x
 *
 */
int SDP3x_init(sdp3x_t *dev, const sdp3x_params_t *params);

/** Start Continuous Measuring
 *
 *      @param addr             Address byte 1 = 0x21, 2 = 0x22, 3 = 0x23, else = Last address
 *      @param tempComp Choose method for temperature compensation 1 = Mass flow, 2 = Differential pressure
 *      @param avg              Choose method for averaging values 0 = No averaging, 1 = Averaging till read
 *      @param interval         Interval in microseconds in which values are read from the sensor
 *      @param result           Values are stored in this struct
 *
 *      @return 1 if device started
 */
int8_t SDP3x_start_continuous(const sdp3x_t *dev, uint8_t tempComp, uint8_t avg, uint32_t interval, sdp3x_measurement_t *result);

/** Read current temperature value
 *
 *      @param dev sdp3x device
 *      @param tempComp         Choose method for temperature compensation 1 = Mass flow, 2 = Differential pressure
 *      @param clkStretching    Activates clock stretching for measurement 0 = Deactivated, 1 = Activated
 *
 *      @return current temperature as double in degree celsius
 */
double SDP3x_read_single_temperature(const sdp3x_t *dev, uint8_t tempComp, uint8_t clkStretching);

/** Read current differential_pressure value
 *
 *      @param dev dp3x device
 *      @param tempComp         Choose method for temperature compensation 1 = Mass flow, 2 = Differential pressure
 *      @param clkStretching    Activates clock stretching for measurement 0 = Deactivated, 1 = Activated
 *
 *      @return current differential pressure as double in pascal
 */
double SDP3x_read_single_differential_pressure(const sdp3x_t *dev, uint8_t tempComp, uint8_t clkStretching);

/** read temperature and differential pressure
 *
 *      @param dev               dp3x device
 *      @param tempComp          Choose method for temperature compensation 1 = Mass flow, 2 = Differential pressure
 *      @param clkStretching     Activates clock stretching for measurement 0 = Deactivated, 1 = Activated
 *      @param result            Values are stored in this struct
 *
 *      @return                  1 on success
 */
int8_t SDP3x_read_single_measurement(const sdp3x_t *dev, uint8_t tempComp, uint8_t clkStretching, sdp3x_measurement_t *result);

/** Stop Continuous Measuring
 *
 *      It takes 500 us till next command.
 *
 *      @param dev device
 *      @return 0 if measurement stopped
 */
int8_t SDP3x_stop_continuous(const sdp3x_t *dev);

/** Resets all I2C devices
 *
 *      After the reset command it takes maximum 20ms to reset
 *      @param dev device
 *
 *      @return 0 if reset is started
 */
int8_t SDP3x_soft_reset(const sdp3x_t *dev);

/** Activates sleep mode
 *
 *      Only activates Sleep Mode if the device is in Idle Mode
 *
 *      @param dev device
 *
 *      @return 0 if sleep is activated
 */
int8_t SDP3x_enter_sleep(const sdp3x_t *dev);

/** Exit sleep mode
 *
 *      @param dev device
 *
 *      @return 0 if sleep is deactivated
 */
int8_t SDP3x_exit_sleep(const sdp3x_t *dev);

#ifdef __cplusplus
}
#endif

#endif /* SDP3X_H_ */
