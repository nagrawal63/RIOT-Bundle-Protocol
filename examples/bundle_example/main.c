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
#include "bundle_server.c"

#define MAIN_QUEUE_SIZE     (8)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];
char shell_thread_stack[THREAD_STACKSIZE_MAIN];

void *_shell_thread(void *arg)
{
    (void) arg;

    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(NULL, line_buf, SHELL_DEFAULT_BUFSIZE);
    return NULL;
}


int main(void)
{
    kernel_pid_t _second_thread = thread_create(shell_thread_stack, sizeof(shell_thread_stack),
                                  THREAD_PRIORITY_MAIN-1, THREAD_CREATE_STACKTEST,
                                  _shell_thread, NULL, "shell_thread");
    (void) _second_thread;
    /* we need a message queue for the thread running the shell in order to
     * receive potentially fast incoming networking packets */
    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);
    printf("RIOT bundle network stack example application\n");


    // port to listen on
    char *p = "5000";

    // starting server
    start_server(p);

    // ipv6 addr of the destination
    // Address of node 4
    char *addr6 = "fe80::4417:5f03:3b73:686d";
    // Address of node 1
    // char *addr6 = "fe80::2014:3f03:3173:6862%8";
    // // payload
    char *data = "test";

    // send a paket every 5sec
    while(1){
        // send data to the source on the same port we listen
        // printf("Sending packet!\n");
        send_bundle(addr6, p, data);
        // wait 5sec
        xtimer_sleep(5);
    }

    /* should be never reached */
    return 0;
}
