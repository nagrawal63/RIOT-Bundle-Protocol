/*
 * Copyright (C) 2015 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       Example application for demonstrating the RIOT network stack
 *
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 *
 * @}
 */

#include <stdio.h>

#include "shell.h"
#include "msg.h"
#include "thread.h"
#include "xtimer.h"
#include "net/gnrc/bundle_protocol/agent.h"
#include "net/gnrc/bundle_protocol/bundle.h"
#include "bundle_server.c"

#define SLEEP 10000000

#define MAIN_QUEUE_SIZE     (8)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

extern int bundle_cmd(int argc, char **argv);

static const shell_command_t shell_commands[] = {
    { "bundle", "send data over bundle_server and listen on bundle_server ports", bundle_cmd },
    { NULL, NULL, NULL }
};

int main(void)
{
    /* we need a message queue for the thread running the shell in order to
     * receive potentially fast incoming networking packets */
    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);
    puts("Bundle network stack example application");

    register_application(1234, thread_getpid());

    /* start shell */
    puts("All up, running the shell now");
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    // uint8_t data[4] = "test";
    // char dst[2] = "41";
    // char service[4] = "1234";
    // char report[2] = "22";


    // while (1) {
    //     puts("Trying to send exmaple bundle.");
    //     printf("Sending data: %s to: %s.\n", data, dst);
    //     send_bundle(data, 4, dst, service, 9, report, NOCRC, DUMMY_PAYLOAD_LIFETIME);
    //     xtimer_sleep(SLEEP);
    // }


    /* should be never reached */
    return 0;
}
