
#include "bme680.h"
#include "bme680_hal.h"

#ifdef MODULE_PERIPH_I2C
#include "periph/i2c.h"
#endif

#ifdef MODULE_PERIPH_SPI
#include "periph/spi.h"
#endif

#define ENABLE_DEBUG (0)
#include "debug.h"

#define BME680_DEV (dev->dev)

int bme680_init(bme680_t *dev, const bme680_params_t *params)
{
    int8_t ret;

    /* Select device interface and apply needed params */
    if(params->intf == BME680_I2C_INTF) {
#ifdef MODULE_PERIPH_I2C
        BME680_DEV.intf = BME680_I2C_INTF;
        BME680_DEV.read = i2c_read_hal;
        BME680_DEV.write = i2c_write_hal;
#endif
    }
    else {
#ifdef MODULE_PERIPH_SPI
        BME680_DEV.intf = BME680_SPI_INTF;
        BME680_DEV.read = spi_read_hal;
        BME680_DEV.write = spi_write_hal;
        spi_init_cs(SPI_DEV(0), SPI_NSS_PIN);
#endif
    }

    /* call internal bme680_init from Bosch Sensortech driver */
    ret = bme680_init_internal(&BME680_DEV);
    if (ret != 0) {
        DEBUG("[bme680]: Failed to get ID");
        return -1;
    }

    /*  retrieve params and set them in bme680_t */
    BME680_DEV.tph_sett.os_temp = params->temp_os;
    BME680_DEV.tph_sett.os_hum = params->hum_os;
    BME680_DEV.tph_sett.os_pres = params->pres_os;

    BME680_DEV.tph_sett.filter = params->filter;

    /* Enable gas measurement if needed */
    BME680_DEV.gas_sett.run_gas = params->gas_measure;
    /* Create a ramp heat waveform in 3 steps */
    BME680_DEV.gas_sett.heatr_temp = params->heater_temp;
    BME680_DEV.gas_sett.heatr_dur = params->heater_dur;

    /* Select the power mode */
    /* Must be set before writing the sensor configuration */
    BME680_DEV.power_mode = params->power_mode;

    /* Set the desired sensor configuration */
    ret = bme680_set_sensor_settings(params->settings, &BME680_DEV);
    if (ret != 0) {
        DEBUG("[bme680]: failed to set settings\n");
        return -2;
    }

    /* Set the power mode */
    ret = bme680_set_sensor_mode(&BME680_DEV);
    if (ret != 0) {
        DEBUG("[bme680]: cannot set sensor mode\n");
        return -3;
    }

    return ret;
}
