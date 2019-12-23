/*
 * Copyright (C) 2019 Jan Schlichter
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     drivers_sdp3x
 * @{
 *
 * @file
 * @brief       SAUL adaption for Sensirion SDP3x devices
 *
 * @author      Jan Schlichter
 *
 * @}
 */

#include "saul.h"

#include "sdp3x.h"

static int read_temperature(const void *dev, phydat_t *res)
{
    double temp = SDP3x_read_single_temperature((const sdp3x_t *)dev, 2, 0);
    res->val[0] = (int16_t)(temp * 100); // multiply with 100 to not lose accuracy when converting to int
    res->unit = UNIT_TEMP_C;
    res->scale = -2;
    return 1;
}


static int read_differential_pressure(const void *dev, phydat_t *res)
{
    double pres = SDP3x_read_single_differential_pressure((const sdp3x_t *)dev, 2, 0);
    res->val[0] = (int16_t)(pres * 100); // multiply with 100 to not lose accuracy when converting to int
    res->unit = UNIT_PA;
    res->scale = -2;
    return 1;
}

const saul_driver_t sdp3x_temperature_saul_driver = {
    .read = read_temperature,
    .write = saul_notsup,
    .type = SAUL_SENSE_TEMP,
};


const saul_driver_t sdp3x_differential_pressure_saul_driver = {
    .read = read_differential_pressure,
    .write = saul_notsup,
    .type = SAUL_SENSE_PRESS,
};
