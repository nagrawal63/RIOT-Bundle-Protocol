/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       Default application that shows functionality of the SDP3x sensor
 *
 * @author      Jan Schlichter
 *
 * @}
 */

#include <stdio.h>
#include <string.h>

#include "thread.h"
#include "shell.h"
#include "shell_commands.h"
#include "xtimer.h"
#include "stdlib.h"
#include "sdp3x.h"
#include "sdp3x_params.h"


static sdp3x_t sdp3x_dev;
static sdp3x_params_t params = SDP3X_PARAMS;

int main(void)
{

    (void) puts("Welcome to RIOT!");

    SDP3x_init(&sdp3x_dev, &params);

    xtimer_sleep(1);

    sdp3x_measurement_t result;

    SDP3x_start_continuous(&sdp3x_dev, 2, 1, 1000000, &result);

    while(1){
        printf("temp: %f pressure: %f compensate: 0x%04x \n", result.temperature, result.differential_pressure, result.dp_scale_factor);
        xtimer_sleep(1);
    }

    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(NULL, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
