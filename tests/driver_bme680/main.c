/*
 * Copyright (C) 2018 Mesotic SAS
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     tests
 * @{
 *
 * @file
 * @brief       Test application for the bme680_driver package.
 *
 * @author      Dylan Laduranty <dylan.laduranty@mesotic.com>
 *
 * @}
 */

#include <stdio.h>
#include "board.h"
#include "bme680.h"
#include "bme680_params.h"
#include "bme680_defs.h"
#include "bme680_hal.h"

#ifdef MODULE_PERIPH_SPI
#include "periph/spi.h"
#endif

#ifdef MODULE_PERIPH_I2C
#include "periph/i2c.h"
#endif

#include "xtimer.h"

#define BME680_DEV dev.dev


int main(void)
{
    bme680_t dev;

    /* You may assign a chip select identifier to be handled later */
    dev.dev.dev_id = 0;

    /* amb_temp can be set to 25 prior to configuring the gas sensor
     * or by performing a few temperature readings without operating the
     * gas sensor */
    BME680_DEV.amb_temp = 25;
    BME680_DEV.delay_ms = ms_sleep;

    printf("Intialize BME680 sensor...");
    if (bme680_init(&dev, &bme680_params[0]) != BME680_OK) {
        puts("failed");
    }
    else {
        puts("OK");
    }

    while(1)
    {
        struct bme680_field_data data;
        uint8_t ret;

        xtimer_sleep(5);

        ret = bme680_get_sensor_data(&data, &(dev.dev));
        if (!ret) {
#ifndef BME680_FLOAT_POINT_COMPENSATION
        printf("[bme680]: T %02" PRIi16 ".%02" PRIu16 " degC, P %" PRIu32 " Pa," \
               " H %02" PRIu32 ".%03" PRIu32 "",
               data.temperature / 100, data.temperature % 100,
            data.pressure, data.humidity / 1000, data.humidity % 1000);
        /* Avoid using measurements from an unstable heating setup */
        if (data.status & BME680_GASM_VALID_MSK)
            printf(", G %" PRIu32 " ohms", data.gas_resistance);
#else
        printf("[bme680]: T %.2f degC, P %.2f Pa, H %2.f ", data.temperature,
            data.pressure, data.humidity);
        /* Avoid using measurements from an unstable heating setup */
        if (data.status & BME680_GASM_VALID_MSK)
            printf(", G %f ohms", data.gas_resistance);
#endif
        printf("\r\n");
        }
        else {
            puts("[bme680]: measurement failed");
        }
        /* Trigger the next measurement if you would like to read data out
           continuously */
        if (BME680_DEV.power_mode == BME680_FORCED_MODE) {
            bme680_set_sensor_mode(&dev.dev);
        }
    }
    /* Should never reach here */
    return 0;
}
