/*
 * Copyright (C) 2019 Mesotic SAS
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     drivers_bme680
 *
 * @{
 * @file
 * @brief       Default configuration for BME680 device driver
 *
 * @author      Dylan Laduranty <dylan.laduranty@mesotic.com>
 */

#ifndef BME680_PARAMS_H
#define BME680_PARAMS_H

#include "board.h"
#include "bme680.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name    Set default configuration parameters for the BME680
 * @{
 */

#ifdef MODULE_PERIPH_I2C
#ifndef BMX280_PARAM_I2C_DEV
#define BMX280_PARAM_I2C_DEV         I2C_DEV(0)
#endif
#ifndef BMX280_PARAM_I2C_ADDR
#define BMX280_PARAM_I2C_ADDR        (0x77)
#endif
#endif /* MODULE_PERIPH_I2C */

#ifdef MODULE_PERIPH_SPI

#endif

/* Defaults Parameters if none provided */
#define BME680_PARAMS                        \
    {                                          \
        .intf        = BME680_I2C_INTF,         \
        .temp_os     = BME680_OS_8X,           \
        .hum_os      = BME680_OS_2X,           \
        .pres_os     = BME680_OS_4X,           \
        .filter      = BME680_FILTER_SIZE_3,   \
        .gas_measure = BME680_ENABLE_GAS_MEAS, \
        .heater_dur  = 320,                    \
        .heater_temp = 150,                    \
        .power_mode  = BME680_FORCED_MODE,     \
        .settings    = BME680_OST_SEL | BME680_OSP_SEL | \
                       BME680_OSH_SEL | BME680_FILTER_SEL | \
                       BME680_GAS_SENSOR_SEL,               \
    }
/**@}*/

/**
 * @brief   Configure params for BME680
 */
static const bme680_params_t bme680_params[] =
{
    BME680_PARAMS,
};

/**
 * @brief   The number of configured sensors
 */
#define BME680_NUMOF    ARRAY_SIZE(bme680_params)

#ifdef __cplusplus
}
#endif

#endif /* BME680_PARAMS_H */
/** @} */
