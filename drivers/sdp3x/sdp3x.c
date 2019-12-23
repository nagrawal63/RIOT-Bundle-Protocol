/*
 * SDP3x.c
 *
 *  Created on: 17.09.2019
 *      Author: Dirk Ehmen, Jan Schlichter
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "periph/i2c.h"
#include "pycrc.c"
#include "xtimer.h"
#include "debug.h"
#include "sdp3x_params.h"
#include "sdp3x.h"
#include "thread.h"

#define ENABLE_DEBUG    (0)

#define DEV_I2C      (dev->params.i2c_dev)
#define DEV_ADDR     (dev->params.i2c_addr)
#define DEV_MODEL    (dev->params.model)


static int8_t _checkCRC(uint16_t value, uint8_t test);
static int8_t _SDP3x_read_data(const sdp3x_t *dev, int16_t* data);
static double _SDP3x_convert_to_pascal(const sdp3x_t *dev, int16_t value);
static double _SDP3x_convert_to_inches_h2o(const sdp3x_t *dev, int16_t value) __attribute__((unused));
static double _SDP3x_convert_to_celsius(int16_t value);
int8_t _SDP3x_start_triggered(const sdp3x_t *dev, uint8_t tempComp, uint8_t clkStretching);
double _SDP3x_read_temp(const sdp3x_t *dev);
double _SDP3x_read_pressure(const sdp3x_t *dev);

typedef struct {
    const sdp3x_t *dev;
    sdp3x_measurement_t *result;
} sdp3x_continuous_args_t;

xtimer_t continuous_timer;
sdp3x_continuous_args_t continuous_args;
uint32_t continuous_interval = 0;

/** Initialize SPD3x
 *
 */
int SDP3x_init(sdp3x_t *dev, const sdp3x_params_t *params) {
    DEBUG("[SDP3x] init: Initializing device\n");

    dev->params = *params;

    i2c_init(DEV_I2C);
    SDP3x_soft_reset(dev);

    //try to read product number to check if sensor is connected and working
    int ret = 0;
    uint8_t cmd1[2] = {0x36, 0x7C};
    uint8_t cmd2[2] = {0xE1, 0x02};
    i2c_acquire(DEV_I2C);
    ret = i2c_write_bytes(DEV_I2C, DEV_ADDR, cmd1, 2, I2C_NOSTOP);
    if(ret != 0) {
        i2c_release(DEV_I2C);
        return ret;
    }
    ret = i2c_write_bytes(DEV_I2C, DEV_ADDR, cmd2, 2, I2C_NOSTOP);
    if(ret != 0) {
        i2c_release(DEV_I2C);
        return ret;
    }
    i2c_release(I2C_DEV(0));
    DEBUG("[SDP3x] init: Init done\n");
    return 1;
}

/** Read current temperature value
 *
 *      @param dev sdp3x device
 *      @param tempComp         Choose method for temperature compensation 1 = Mass flow, 2 = Differential pressure
 *      @param clkStretching    Activates clock stretching for measurement 0 = Deactivated, 1 = Activated
 *
 *      @return current temperature as double in degree celsius
 */
double SDP3x_read_single_temperature(const sdp3x_t *dev, uint8_t tempComp, uint8_t clkStretching){
    _SDP3x_start_triggered(dev, tempComp, clkStretching);
    xtimer_usleep(45000);                   //wait for measurement to be ready
    double temp = _SDP3x_read_temp(dev);
    return temp;
}

/** Read current differential_pressure value
 *
 *      @param dev dp3x device
 *      @param tempComp         Choose method for temperature compensation 1 = Mass flow, 2 = Differential pressure
 *      @param clkStretching    Activates clock stretching for measurement 0 = Deactivated, 1 = Activated
 *
 *      @return current differential pressure as double in pascal
 */
double SDP3x_read_single_differential_pressure(const sdp3x_t *dev, uint8_t tempComp, uint8_t clkStretching){
    _SDP3x_start_triggered(dev, tempComp, clkStretching);
    xtimer_usleep(45000);                   //wait for measurement to be ready
    double temp = _SDP3x_read_pressure(dev);
    return temp;
}

/** read temperature and differential pressure
 *
 *      @param dev               dp3x device
 *      @param tempComp          Choose method for temperature compensation 1 = Mass flow, 2 = Differential pressure
 *      @param clkStretching     Activates clock stretching for measurement 0 = Deactivated, 1 = Activated
 *      @param result            Values are stored in this struct
 *
 *      @return                  1 on success
 */
int8_t SDP3x_read_single_measurement(const sdp3x_t *dev, uint8_t tempComp, uint8_t clkStretching, sdp3x_measurement_t *result){
    _SDP3x_start_triggered(dev, tempComp, clkStretching);
    xtimer_usleep(45000);                   //wait for measurement to be ready
    // read in sensor values here
    int16_t data[3];
    uint8_t ret = _SDP3x_read_data(dev, data);
    if(ret != 0) {
        DEBUG("[SDP3x] Error reading date\n");
    } else {
        result->differential_pressure = _SDP3x_convert_to_pascal(dev, data[0]);
        result->temperature = _SDP3x_convert_to_celsius(data[1]);
        result->dp_scale_factor = data[2];
    }
    return 1;
}

/**
 * intern function that reads one measurement value in a different thread
 * @param arg data struct for result
 */
void _read_continuous_value(void *arg) {
    sdp3x_continuous_args_t *args = (sdp3x_continuous_args_t *) arg;
    sdp3x_measurement_t *result = args->result;
    const sdp3x_t *dev = args->dev;

    // read in sensor values here
    int16_t data[3];
    uint8_t ret = _SDP3x_read_data(dev, data);
    if(ret != 0) {
        DEBUG("[SDP3x] Error reading date\n");
    } else {
        result->differential_pressure = _SDP3x_convert_to_pascal(dev, data[0]);
        result->temperature = _SDP3x_convert_to_celsius(data[1]);
        result->dp_scale_factor = data[2];
    }

    if(continuous_interval != 0){
        xtimer_set(&continuous_timer, continuous_interval);
    }
}

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
int8_t SDP3x_start_continuous(const sdp3x_t *dev, uint8_t tempComp, uint8_t avg, uint32_t interval, sdp3x_measurement_t *result) {
    int ret  = 0;
    uint8_t cmd[2] = {0x36,0};
    if(tempComp == 1) {
        if (avg == 0) {
            cmd[1] = 0x08;
        } else {
            cmd[1] = 0x03;
        }
    } else {
        if (avg == 0) {
            cmd[1] = 0x1E;
        } else {
            cmd[1] = 0x15;
        }
    }

    i2c_acquire(DEV_I2C);
    ret = i2c_write_bytes(DEV_I2C, DEV_ADDR, cmd, 2, 0);
    i2c_release(DEV_I2C);

    if(ret == 0){

        continuous_interval = interval;

        continuous_args.dev = dev;
        continuous_args.result = result;

        continuous_timer.target = continuous_timer.long_target = 0;
        continuous_timer.callback = (void *)_read_continuous_value;
        continuous_timer.arg = &continuous_args;

        xtimer_set(&continuous_timer, continuous_interval);



        //thread_create(sdp3x_stack, sizeof(sdp3x_stack), THREAD_PRIORITY_MAIN +1, 0, read_continuous_value, &args, "sdp3x_data_reader");
    } else {
        DEBUG("[SDP3x] Error starting continuous measurement\n");
    }

    return ret;
}

/**
 *      intern function to start triggered measurement
 *      @param dev device
 *      @param tempComp Choose method for temperature compensation 1 = Mass flow, 2 = Differential pressure
 *      @param clkStretching     Activates clock stretching for measurement 0 = Deactivated, 1 = Activated
 *      @return i2c error code
 */
int8_t _SDP3x_start_triggered(const sdp3x_t *dev, uint8_t tempComp, uint8_t clkStretching) {
    int ret = 0;
    uint8_t cmd[2];
    if(clkStretching == 0) {
        cmd[0] = 0x36;
        if(tempComp == 1) {
            cmd[1] = 0x24;
        } else {
            cmd[1] = 0x2F;
        }
    } else {
        cmd[0] = 0x37;
        if(tempComp == 1) {
            cmd[1] = 0x26;
        } else {
            cmd[1] = 0x2D;
        }
    }

    i2c_acquire(DEV_I2C);
    ret = i2c_write_bytes(DEV_I2C, DEV_ADDR, cmd, 2, 0);
    i2c_release(DEV_I2C);
    return ret;
}

/** Stop Continuous Measuring
 *
 *      It takes 500 us till next command.
 *
 *      @param dev device
 *      @return 0 if measurement stopped
 */
int8_t SDP3x_stop_continuous(const sdp3x_t *dev) {
    int ret = 0;
    uint8_t cmd[2] = {0x3F, 0xF9};
    DEBUG("[SDP3x] stop_continuous: Stopping continuous measurement on device %#X\n", DEV_ADDR);
    i2c_acquire(DEV_I2C);
    ret = i2c_write_bytes(DEV_I2C, DEV_ADDR, cmd, 2, 0);
    i2c_release(DEV_I2C);

    continuous_interval = 0;

    return ret;
}

/**
 *      intern function to read temperature
 *      @param dev device
 *      @return celsius value
 */
double _SDP3x_read_temp(const sdp3x_t *dev) {
    int16_t data[3];
    uint8_t ret = _SDP3x_read_data(dev, data);
    if(ret != 0) {
        DEBUG("[SDP3x] Error reading date\n");
        return ret;
    }
    DEBUG("[SDP3x] red temperature bare value %i\n", data[1]);
    return _SDP3x_convert_to_celsius(data[1]);
}

/**
 *      intern function to read differential pressure
 *      @param dev device
 *      @return pascal value
 */
double _SDP3x_read_pressure(const sdp3x_t *dev) {
    int16_t data[3];
    uint8_t ret = _SDP3x_read_data(dev, data);
    if(ret != 0) {
        DEBUG("[SDP3x] Error reading date\n");
        return ret;
    }
    return _SDP3x_convert_to_pascal(dev, data[0]);
}

/** Resets all I2C devices
 *
 *      After the reset command it takes maximum 20ms to reset
 *      @param dev device
 *
 *      @return 0 if reset is started
 */
int8_t SDP3x_soft_reset(const sdp3x_t *dev) {
    int ret = 0;
    DEBUG("[SDP3x] soft_reset: Sending soft reset to all devices\n");
    i2c_acquire(DEV_I2C);
    ret = i2c_write_byte(DEV_I2C, 0x00, 0x06, 0);  //General Call Reset
    i2c_release(DEV_I2C);
    xtimer_usleep(20000);   //Wait 20ms for the reset to be processed
    DEBUG("[SDP3x] soft_reset: reset done\n");
    return ret;
}

/** Activates sleep mode
 *
 *      Only activates Sleep Mode if the device is in Idle Mode
 *
 *      @param dev device
 *
 *      @return 0 if sleep is activated
 */
int8_t SDP3x_enter_sleep(const sdp3x_t *dev) {
    int ret = 0;
    uint8_t cmd[2] = {0x36, 0x77};
    DEBUG("[SDP3x] enter_sleep: Entering sleep mode on device %#X\n", DEV_ADDR);
    i2c_acquire(DEV_I2C);
    ret = i2c_write_bytes(DEV_I2C, DEV_ADDR, cmd, 2, 0);
    i2c_release(DEV_I2C);

    return ret;
}

/** Exit sleep mode
 *
 *      @param dev device
 *
 *      @return 0 if sleep is deactivated
 */
int8_t SDP3x_exit_sleep(const sdp3x_t *dev) {
    int ret = 0;
    uint8_t ptr[1] = {0};
    DEBUG("[SDP3x] exit_sleep: Exiting sleep mode on device %#X\n", DEV_ADDR);
    i2c_acquire(DEV_I2C);
    ret = i2c_write_bytes(DEV_I2C, DEV_ADDR, ptr, 0, 0);
    xtimer_usleep(2000);
    ret = i2c_write_bytes(DEV_I2C, DEV_ADDR, ptr, 0, 0);
    i2c_release(DEV_I2C);

    return ret;
}

/** Read measurements
 *
 *      Data consists of:
 *      2 byte Differential Pressure,
 *      1 byte CRC
 *      2 byte Temperature,
 *      1 byte CRC
 *      2 byte Scale Factor differential pressure
 *      1 byte CRC
 *
 *      @param addr                     Address byte 1 = 0x21, 2 = 0x22, 3 = 0x23, else = Last address
 *      @param data                     Data will be stored here
 *
 *      @return 0 if data could be read, 1 if CRC-Error
 */
static int8_t _SDP3x_read_data(const sdp3x_t *dev, int16_t* data) {
    int ret = 0;
    uint8_t *readData = calloc(9, sizeof(uint8_t));
    i2c_acquire(DEV_I2C);
    ret = i2c_read_bytes(DEV_I2C, DEV_ADDR, readData, 9, 0);
    i2c_release(DEV_I2C);

    if(ret != 0){
        DEBUG("[SDP3x] read_data: ret %i\n", ret);
        free(readData);
        return ret;
    }
    data[2] = (readData[6] << 8) + readData[7];
    data[1] = (readData[3] << 8) + readData[4];
    data[0] = (readData[0] << 8) + readData[1];

    uint8_t correct = 1;

    correct &= _checkCRC(data[2], readData[8]);
    correct &= _checkCRC(data[1], readData[5]);
    correct &= _checkCRC(data[0], readData[2]);

    if(correct == 0) {
        DEBUG("[SDP3x] read_data: CRC-Failure\n");
        free(readData);
        return SDP3x_CRCERROR;
    }

    free(readData);
    return 0;
}

/**
 *      intern method to convert sensor value to pascal
 *      @param dev device
 *      @param value raw sensor value
 *      @return pascal value
 */
static double _SDP3x_convert_to_pascal(const sdp3x_t *dev, int16_t value){
    double div = 1.0;
    if (DEV_MODEL == 1) {
        div = 60.0;
    } else {
        div = 240.0;
    }
    return (((double) value) / div);
}

/**
 *      intern method to convert sensor value to inches h2o
 *      @param dev device
 *      @param value raw sensor value
 *      @return inches h2o value
 */
static double _SDP3x_convert_to_inches_h2o(const sdp3x_t *dev, int16_t value){
    double div = 1.0;
    if (DEV_MODEL == 1) {
        div = 14945.0;
    } else {
        div = 59780.0;
    }
    return (((double) value) / div);
}

/**
 *      intern method to convert sensor value to celsius
 *      @param dev device
 *      @param value raw sensor value
 *      @return celsius value
 */
static double _SDP3x_convert_to_celsius(int16_t value){
    double div = 200;
    return ((double) value / div);
}

/**
 *      check if crc is valid
 *      @param value value
 *      @param test value
 *      @return 1 on success
 */
static int8_t _checkCRC(uint16_t value, uint8_t test) {
        crc_t crc;
        uint8_t data[2] = {value>>8, value&0x00FF};

        crc = crc_init();
        crc = crc_update(crc, data, 2);
        crc = crc_finalize(crc);

        return (crc == test ? 1 : 0);
}


