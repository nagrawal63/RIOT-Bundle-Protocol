/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       Handler for commands of bundle protocol example
 *
 * @author      Nishchay Agrawal <agrawal.nishchay5@gmail.com>
 *
 * @}
 */

#include <stdio.h>
#include <inttypes.h>
#include <string.h>

#include "net/gnrc/bundle_protocol/bundle.h"
#include "net/gnrc/bundle_protocol/agent.h"
#include "timex.h"
#include "utlist.h"
#include "msg.h"
#include "xtimer.h"

int bundle_cmd(int argc, char **argv)
{
    if (argc < 2) {
        printf("usage: %s [send|receive]\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "send") == 0) {
      if (argc < 5) {
          printf("usage: %s send <addr> <port> <data> <data_len>\n",
                 argv[0]);
          return 1;
      }
      send_bundle((uint8_t *)argv[4], atoi(argv[5]),argv[2], "1", NOCRC, DUMMY_PAYLOAD_LIFETIME);
    }
    else if (strcmp(argv[1], "receive") == 0) {
      msg_t msg;
      int res = msg_try_receive(&msg);
      printf("received message with data = %s with res = %d.\n", (char *)msg.content.ptr, res);
    }
    else {
        puts("error: invalid command");
    }
    return 0;
}
