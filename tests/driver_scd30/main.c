
/*** Hardware ***/
#include <board.h>
#include "periph/gpio.h"
#include "periph/uart.h"

/*** Base ***/
#include <stdio.h>
//#include <unistd.h>

/*** System ***/
#include "thread.h"
#include "net/gnrc.h"
#include "phydat.h"

/*** Module ***/
#include "xtimer.h"
#include "shell.h"
#include "saul_reg.h"

/*** Driver ***/
#include "scd30.h"
#include "scd30_internal.h"

/****** Macro ******/


/*** Variables ************************************************/
static float res[3];


/*** Functions ************************************************/
int show(int cnt, char **arg);


/*** Special Variables ***/
const shell_command_t sh_cmd[] = {
	{"show", "Los geht's.", show},
	{ NULL, NULL, NULL }
};


/*** Application **********************************************/
int main(void)
{
	/*** main process ***/
	printf("\nSCD30 Unit Test:\n");
	printf("Input: show (Enter)\n");

	/*** shell ***/
	char line_buf[SHELL_DEFAULT_BUFSIZE];
	shell_run(sh_cmd, line_buf, SHELL_DEFAULT_BUFSIZE);

	return 0;
}

int show(int cnt, char **arg)
{
	(void)cnt;
	(void)arg;
	uint16_t value = 0;

	/*** Start ***/
	//scd30_cmd_ext(dev, SCD30_START, SCD30_DEF_PRESSURE);

	printf("\n[test] Measurements:\n");

	uint8_t state = 0;
	for(int i=0; i<scd30_num; i++) {
		value = scd30_get_param(&scd30_devs[0], SCD30_INTERVAL);
		printf("[test][dev-%d] Interval: %u s\n", i, value);
		value = scd30_get_param(&scd30_devs[0], SCD30_T_OFFSET);
		printf("[test][dev-%d] Temperature Offset: %u.%02u C\n", i, value/100, value%100);
		value = scd30_get_param(&scd30_devs[0], SCD30_A_OFFSET);
		printf("[test][dev-%d] Altitude Compensation: %u m\n", i, value);
		value = scd30_get_param(&scd30_devs[0], SCD30_ASC);
		printf("[test][dev-%d] ASC: %u\n", i, value);
		value = scd30_get_param(&scd30_devs[0], SCD30_FRC);
		printf("[test][dev-%d] FRC: %u ppm\n", i, value);
		state = scd30_read_single(&scd30_devs[i], res);
		if(state != 0) {
			printf("[test][dev-%d] co2: %f, temp: %f, hum: %f. \n",
				   i, res[0], res[1], res[2] );
		}
	}

	/*** Reset ***/
	//scd30_cmd(&scd30_devs[0], SCD30_RESET);

	/*** Configuration ***/
	//scd30_set_param(&scd30_devs[0], SCD30_FRC, 0x01c2);
	//uint16_t stat = scd30_get_param(&scd30_devs[0], SCD30_FRC);

#ifdef SCD30_EXT_SAUL
	saul_reg_t *dev = saul_reg;
	phydat_t res;

	if (dev == NULL) {
		printf("end (none)\n");
		return 0;
	}

	int dim;
	while (dev) {
		dim = saul_reg_read(dev, &res);
		printf("\nDev: %s\tType: %s\tDim: %d\n", dev->name,
			   saul_class_to_str(dev->driver->type), dim);
		phydat_dump(&res, dim);
		dev = dev->next;
	}

//	printf("--- saul_reg_find_name\n");
//	dev = saul_reg_find_name("CO2");
//	printf("\nDev: %s\tType: %s\n", dev->name,
//		   saul_class_to_str(dev->driver->type));
//	phydat_dump(&res, dim);
#endif

	printf("[test] end\n");
	return 0;
}
