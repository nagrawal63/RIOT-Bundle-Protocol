
/*** Base ***/
#include <string.h>

/*** Hardware ***/
#include "periph/i2c.h"

/*** Module ***/
#include "xtimer.h"

/*** Self ***/
#include "scd30.h"
#include "scd30_internal.h"

/*** Debug ***/
#define ENABLE_DEBUG (0)
//#define ENABLE_DEBUG (1)
#include "debug.h"

/****** Macro ******/
#define SCD30_I2C           (dev->params.i2cDev)
#define SCD30_I2C_ADDRESS   (dev->params.i2cAddr)
#define SCD30_ERROR         (0xFFFF)

/********* Variables *********/
static uint8_t buffI2C[20];
static uint16_t retI2C;
static unsigned int scd30_raw[3];
static float scd30_measurements[3];
/* CRC */
extern uint8_t tabCRC[256];

/********* Functions *********/
static uint8_t scd30_crc_check(uint8_t *buff, uint8_t len);
static void scd30_crc_calc(uint8_t *buff, uint8_t len);



/********* Application *************************************************************/

uint16_t scd30_read(scd30_t *dev, uint16_t cmd)
{
	i2c_acquire(SCD30_I2C);
	buffI2C[0] = (uint8_t)(cmd >> 8);
	buffI2C[1] = (uint8_t)cmd;
	i2c_write_bytes(SCD30_I2C, SCD30_I2C_ADDRESS, buffI2C, 2, 0);
	i2c_read_bytes(SCD30_I2C, SCD30_I2C_ADDRESS, buffI2C, 3, 0);
	i2c_release(SCD30_I2C);

	if(scd30_crc_check(buffI2C, 3) != 0) {
		return SCD30_ERROR;
	}

	retI2C = (buffI2C[0]<<8) | buffI2C[1];
	return retI2C;
}

/***
 * CO2          0x0028 0x0029
 * Temperature  0x002A 0x002B
 * Humidity     0x002C 0x002D
***/
uint16_t scd30_read_data(scd30_t *dev, uint16_t cmd)
{
	uint8_t *buff;

	i2c_acquire(SCD30_I2C);
	buffI2C[0] = (uint8_t)(cmd >> 8);
	buffI2C[1] = (uint8_t)cmd;
	i2c_write_bytes(SCD30_I2C, SCD30_I2C_ADDRESS, buffI2C, 2, 0);
	i2c_read_bytes(SCD30_I2C, SCD30_I2C_ADDRESS, buffI2C, 18, 0);
	i2c_release(SCD30_I2C);

	if(scd30_crc_check(buffI2C, 3) != 0) {
		return SCD30_ERROR;
	}
	if(scd30_crc_check(buffI2C+3, 3) != 0) {
		return SCD30_ERROR;
	}
	if(scd30_crc_check(buffI2C+6, 3) != 0) {
		return SCD30_ERROR;
	}
	if(scd30_crc_check(buffI2C+9, 3) != 0) {
		return SCD30_ERROR;
	}
	if(scd30_crc_check(buffI2C+12, 3) != 0) {
		return SCD30_ERROR;
	}
	if(scd30_crc_check(buffI2C+15, 3) != 0) {
		return SCD30_ERROR;
	}

	for(int i=0; i<3; i++) {
		buff = buffI2C + i*6;
		scd30_raw[i] = (unsigned int)((((unsigned int)buff[0]) << 24) |
						 (((unsigned int)buff[1]) << 16) |
						 (((unsigned int)buff[3]) << 8) |
						 ((unsigned int)buff[4]));
	}

	/*** Type Punning ***/
	//scd30_raw[0] = 0x43db8c2e; // 439.09f
	memcpy(&scd30_measurements, &scd30_raw, 3*sizeof(float));
	DEBUG("[scd30][data] co2: %f, temp: %f, hum: %f. \n",
		scd30_measurements[0], scd30_measurements[1], scd30_measurements[2] );
	return 0;
}

void scd30_cmd(scd30_t *dev, uint16_t cmd)
{
	i2c_acquire(SCD30_I2C);
	buffI2C[0] = (uint8_t)(cmd >> 8);
	buffI2C[1] = (uint8_t)cmd;
	i2c_write_bytes(SCD30_I2C, SCD30_I2C_ADDRESS, buffI2C, 2, 0);
	i2c_release(SCD30_I2C);
}

void scd30_cmd_ext(scd30_t *dev, uint16_t cmd, uint16_t ext)
{
	i2c_acquire(SCD30_I2C);
	buffI2C[0] = (uint8_t)(cmd >> 8);
	buffI2C[1] = (uint8_t)cmd;
	buffI2C[2] = (uint8_t)(ext >> 8);
	buffI2C[3] = (uint8_t)ext;
	scd30_crc_calc(buffI2C+2, 3);
	i2c_write_bytes(SCD30_I2C, SCD30_I2C_ADDRESS, buffI2C, 5, 0);
	i2c_release(SCD30_I2C);
}

/***
 * same as scd30_read()
 * maybe merge in the future.
***/
uint16_t scd30_get_param(scd30_t *dev, uint16_t param)
{
	i2c_acquire(SCD30_I2C);
	buffI2C[0] = (uint8_t)(param >> 8);
	buffI2C[1] = (uint8_t)param;
	i2c_write_bytes(SCD30_I2C, SCD30_I2C_ADDRESS, buffI2C, 2, 0);
	i2c_read_bytes(SCD30_I2C, SCD30_I2C_ADDRESS, buffI2C, 3, 0);
	i2c_release(SCD30_I2C);

	if(scd30_crc_check(buffI2C, 3) != 0) {
		return SCD30_ERROR;
	}

	retI2C = (buffI2C[0]<<8) | buffI2C[1];
	return retI2C;
}

/***
 * same as scd30_cmd_ext()
 * maybe merge in the future.
***/
void scd30_set_param(scd30_t *dev, uint16_t param, uint16_t val)
{
	i2c_acquire(SCD30_I2C);
	buffI2C[0] = (uint8_t)(param >> 8);
	buffI2C[1] = (uint8_t)param;
	buffI2C[2] = (uint8_t)(val >> 8);
	buffI2C[3] = (uint8_t)val;
	scd30_crc_calc(buffI2C, 5);
	i2c_write_bytes(SCD30_I2C, SCD30_I2C_ADDRESS, buffI2C, 5, 0);
	i2c_release(SCD30_I2C);
}

/***
 * CRC check
***/
static uint8_t scd30_crc_check(uint8_t *buff, uint8_t len)
{
	uint8_t crc = 0xff;

	for(int i=0; i<(len-1); i++) {
		crc = tabCRC[crc^buff[i]];
	}

	if(crc == buff[len-1]) {
		//DEBUG("[scd30] crc: ok\n");
		return 0;
	} else {
		//DEBUG("[scd30] crc: failed\n");
		return 0xff;
	}
}

/***
 * CRC calculate
***/
static void scd30_crc_calc(uint8_t *buff, uint8_t len)
{
	uint8_t crc = 0xff;

	for(int i=0; i<(len-1); i++) {
		crc = tabCRC[crc^buff[i]];
	}

	buff[len-1] = crc;
}

/********* Functions *************************************************************/

uint8_t scd30_read_single(scd30_t *dev, float *res)
{
	uint16_t state = 0;
	uint8_t upLimit = 5; // max: 5 times.

	while(upLimit--) {
		state = scd30_read(dev, SCD30_STATUS);
		if(state == 1) {
			DEBUG("[scd30][status] ready\n");
			scd30_read_data(dev, SCD30_DATA);
			res[0] = scd30_measurements[0];
			res[1] = scd30_measurements[1];
			res[2] = scd30_measurements[2];
			break;
		} else {
			DEBUG("[scd30][status] NOT ready\n");
		}
		xtimer_sleep(2); // Interval: 2s.
	}

	return state;
}

uint8_t scd30_read_cycle(scd30_t *dev)
{
	uint16_t state = 0;

	scd30_cmd_ext(dev, SCD30_START, SCD30_DEF_PRESSURE);
	xtimer_sleep(5);  // wait for interval

	state = scd30_read(dev, SCD30_STATUS);
	if(state == 1) {
		scd30_read_data(dev, SCD30_DATA);
	}

	scd30_cmd(dev, SCD30_STOP);
	return state;
}

int scd30_init(scd30_t *dev, const scd30_params_t *params)
{
	DEBUG("[scd30] init\n");
	dev->params = *params;

	/*** Firmware Version ***/
	scd30_read(dev, SCD30_VERSION);
	DEBUG("[scd30] --- Version 0x%02x%02x, CRC 0x%02x\n", buffI2C[0], buffI2C[1], buffI2C[2]);

	/*** Start ***/
	//scd30_cmd_ext(SCD30_START, 0);

	DEBUG("[scd30] init: done.\n");

	return 0;
}



/****** CRC ******/
uint8_t tabCRC[256] = {0x00,0x31,0x62,0x53,0xC4,0xF5,0xA6,0x97,0xB9,0x88,0xDB,0xEA,0x7D,0x4C,0x1F,0x2E,0x43,0x72,0x21,0x10,0x87,0xB6,0xE5,0xD4,0xFA,0xCB,0x98,0xA9,0x3E,0x0F,0x5C,0x6D,0x86,0xB7,0xE4,0xD5,0x42,0x73,0x20,0x11,0x3F,0x0E,0x5D,0x6C,0xFB,0xCA,0x99,0xA8,0xC5,0xF4,0xA7,0x96,0x01,0x30,0x63,0x52,0x7C,0x4D,0x1E,0x2F,0xB8,0x89,0xDA,0xEB,0x3D,0x0C,0x5F,0x6E,0xF9,0xC8,0x9B,0xAA,0x84,0xB5,0xE6,0xD7,0x40,0x71,0x22,0x13,0x7E,0x4F,0x1C,0x2D,0xBA,0x8B,0xD8,0xE9,0xC7,0xF6,0xA5,0x94,0x03,0x32,0x61,0x50,0xBB,0x8A,0xD9,0xE8,0x7F,0x4E,0x1D,0x2C,0x02,0x33,0x60,0x51,0xC6,0xF7,0xA4,0x95,0xF8,0xC9,0x9A,0xAB,0x3C,0x0D,0x5E,0x6F,0x41,0x70,0x23,0x12,0x85,0xB4,0xE7,0xD6,0x7A,0x4B,0x18,0x29,0xBE,0x8F,0xDC,0xED,0xC3,0xF2,0xA1,0x90,0x07,0x36,0x65,0x54,0x39,0x08,0x5B,0x6A,0xFD,0xCC,0x9F,0xAE,0x80,0xB1,0xE2,0xD3,0x44,0x75,0x26,0x17,0xFC,0xCD,0x9E,0xAF,0x38,0x09,0x5A,0x6B,0x45,0x74,0x27,0x16,0x81,0xB0,0xE3,0xD2,0xBF,0x8E,0xDD,0xEC,0x7B,0x4A,0x19,0x28,0x06,0x37,0x64,0x55,0xC2,0xF3,0xA0,0x91,0x47,0x76,0x25,0x14,0x83,0xB2,0xE1,0xD0,0xFE,0xCF,0x9C,0xAD,0x3A,0x0B,0x58,0x69,0x04,0x35,0x66,0x57,0xC0,0xF1,0xA2,0x93,0xBD,0x8C,0xDF,0xEE,0x79,0x48,0x1B,0x2A,0xC1,0xF0,0xA3,0x92,0x05,0x34,0x67,0x56,0x78,0x49,0x1A,0x2B,0xBC,0x8D,0xDE,0xEF,0x82,0xB3,0xE0,0xD1,0x46,0x77,0x24,0x15,0x3B,0x0A,0x59,0x68,0xFF,0xCE,0x9D,0xAC};

