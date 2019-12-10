
#ifdef MODULE_SCD30

/*** System ***/

/*** Driver ***/
#include "scd30.h"
#include "scd30_params.h"

/********* Macros *********/
#define SCD30_NUM   ARRAY_SIZE(scd30_params)

/********* Variables *********/
scd30_t scd30_devs[SCD30_NUM];
uint8_t scd30_num = SCD30_NUM;

/********* Functions *********/


/********* Extension *********/
#ifdef SCD30_EXT_SAUL
#include "saul_reg.h"
static saul_reg_t saul_entries[SCD30_NUM][3];
extern const saul_driver_t scd30_co2_saul_driver;
extern const saul_driver_t scd30_temp_saul_driver;
extern const saul_driver_t scd30_hum_saul_driver;
#endif



void auto_init_scd30(void)
{
	for (uint8_t i=0; i<SCD30_NUM; i++) {

		scd30_init(&scd30_devs[i], &scd30_params[i]);

#ifdef SCD30_EXT_SAUL
		saul_entries[i][0].dev = &scd30_devs[i];
		saul_entries[i][0].name = scd30_saul_info[0].name;
		saul_entries[i][0].driver = &scd30_co2_saul_driver;
		saul_reg_add(&saul_entries[i][0]);

		saul_entries[i][1].dev = &scd30_devs[i];
		saul_entries[i][1].name = scd30_saul_info[1].name;
		saul_entries[i][1].driver = &scd30_temp_saul_driver;
		saul_reg_add(&saul_entries[i][1]);

		saul_entries[i][2].dev = &scd30_devs[i];
		saul_entries[i][2].name = scd30_saul_info[2].name;
		saul_entries[i][2].driver = &scd30_hum_saul_driver;
		saul_reg_add(&saul_entries[i][2]);
#endif
	}
}

#else
typedef int dont_be_pedantic;
#endif
