
/*** System ***/
#include "phydat.h"
#include "saul.h"

/*** Self ***/
#include "scd30.h"
#include "scd30_internal.h"

/*** Debug ***/
#define ENABLE_DEBUG (1)
#include "debug.h"

/********* Variables *********/
static float scd30_measurements[3];
/********* Functions *********/



/********* Application *************************************************************/

/***
 * 400 ppm corresponds to 0x43c80000
***/
static int _co2_read(const void *dev, phydat_t *res)
{
	uint8_t state = 0;
//	memset(res, 0, sizeof(phydat_t));
	state = scd30_read_single((scd30_t *)dev, scd30_measurements);
	if(state == 0) {
		return -ECANCELED;
	}
	res->val[0] = (int16_t)(scd30_measurements[0]);
	res->unit = UNIT_PPM;
	res->scale = 0;
	return 1;
}

static int _temp_read(const void *dev, phydat_t *res)
{
	uint8_t state = 0;
	//	memset(res, 0, sizeof(phydat_t));
	state = scd30_read_single((scd30_t *)dev, scd30_measurements);
	if(state == 0) {
		return -ECANCELED;
	}
	res->val[0] = (int16_t)(scd30_measurements[1] * 100);
	res->unit = UNIT_TEMP_C;
	res->scale = -2;
	return 1;
}

static int _hum_read(const void *dev, phydat_t *res)
{
	uint8_t state = 0;
	//	memset(res, 0, sizeof(phydat_t));
	state = scd30_read_single((scd30_t *)dev, scd30_measurements);
	if(state == 0) {
		return -ECANCELED;
	}
	res->val[0] = (int16_t)(scd30_measurements[2] * 100);
	res->unit = UNIT_UNDEF;
	res->scale = -2;
	return 1;
}

const saul_driver_t scd30_co2_saul_driver = {
	.read = _co2_read,
	.write = saul_notsup,
	.type = SAUL_SENSE_CO2,
};

const saul_driver_t scd30_temp_saul_driver = {
	.read = _temp_read,
	.write = saul_notsup,
	.type = SAUL_SENSE_TEMP,
};

const saul_driver_t scd30_hum_saul_driver = {
	.read = _hum_read,
	.write = saul_notsup,
	.type = SAUL_SENSE_HUM,
};
