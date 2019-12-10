
#ifndef _SCD30_H_
#define _SCD30_H_

/*** Hardware ***/
#include "periph/i2c.h"

#ifdef __cplusplus
extern "C" {
#endif


/********* Definition *********/
typedef struct {
	i2c_t i2cDev;
	uint8_t i2cAddr;
} scd30_params_t;

typedef struct {
	int16_t mode;
} scd30_calibration_t;

typedef struct {
	scd30_params_t       params;
	scd30_calibration_t  calib;
} scd30_t;


/********* Variables *********/
extern uint8_t scd30_num;
extern scd30_t scd30_devs[];


/********* Functions *********/
extern int scd30_init(scd30_t *dev, const scd30_params_t *params);
/* cmd */
extern void scd30_cmd_ext(scd30_t *dev, uint16_t cmd, uint16_t ext);
extern void scd30_cmd(scd30_t *dev, uint16_t cmd);
/* config */
extern void scd30_set_param(scd30_t *dev, uint16_t param, uint16_t val);
extern uint16_t scd30_get_param(scd30_t *dev, uint16_t param);
/* read data */
extern uint8_t scd30_read_single(scd30_t *dev, float *res);
extern uint8_t scd30_read_cycle(scd30_t *dev);


#ifdef __cplusplus
}
#endif

#endif