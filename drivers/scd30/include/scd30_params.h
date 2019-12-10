/***
 *
 * This file should be included for only once.
 * e.g.: while loading the driver.
 *
***/

#ifndef _SCD30_PARAMS_H_
#define _SCD30_PARAMS_H_

/*** Hardware ***/
#include "periph/i2c.h"

/*** System ***/
#include "saul_reg.h"

#ifdef __cplusplus
extern "C" {
#endif


static const scd30_params_t scd30_params[] = {
	{
		.i2cDev    = I2C_DEV(0),
		.i2cAddr   = 0x61
	}
};

static const saul_reg_info_t scd30_saul_info[] = {
	{
		.name = "CO2"
	},
	{
		.name = "Temperature"
	},
	{
		.name = "Humidity"
	}
};

#ifdef __cplusplus
}
#endif

#endif