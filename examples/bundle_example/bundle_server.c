/*
 * Copyright (C) 2015-17 Freie Universität Berlin
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
 * @brief       Demonstrating the sending and receiving of UDP data
 *
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 * @author      Martine Lenders <m.lenders@fu-berlin.de>
 *
 * @}
 */

#include <stdio.h>
#include <inttypes.h>
#include <string.h>

// #include "net/gnrc.h"
// #include "net/gnrc/netif.h"
// #include "net/gnrc/netif/hdr.h"
// #include "net/gnrc/pktdump.h"
// #include "net/gnrc/convergence_layer.h"
#include "net/gnrc/bundle_protocol/bundle.h"
// #include "net/gnrc/bundle_protocol/bundle_storage.h"
// #include "net/netdev/ieee802154.h"
#include "net/gnrc/bundle_protocol/agent.h"
#include "timex.h"
#include "utlist.h"
#include "msg.h"
#include "xtimer.h"

// static gnrc_netreg_entry_t bundle_server = GNRC_NETREG_ENTRY_INIT_PID(GNRC_NETREG_DEMUX_CTX_ALL, KERNEL_PID_UNDEF);

// static void send_bundle(char *dtn_dst, char *port_str, char *data, )
// {
//   // gnrc_netif_t* netif = NULL;
//   // char* iface;
//   // uint16_t port;
//   (void) dtn_dst;
//   (void) port_str;
//   (void) data;

//   uint64_t payload_flag;
//   uint8_t *payload_data;
//   size_t data_len;

//   int iface = 9;
//   gnrc_netif_t *netif = NULL;
//   netif = gnrc_netif_get_by_pid(iface);

//   // Preparing the payload block
//   data_len = netif->l2addr_len + 1;
//   payload_data = data;
//   // payload_data[data_len] = '\0';
//   printf("Data to be added in the block is %s with size %d and addr len is %d.\n", payload_data, data_len, netif->l2addr_len);
//   for(int i=0;i<(int)data_len;i++){
//     printf("%02x",payload_data[i]);
//   }
//   printf(".\n");
//   if (calculate_canonical_flag(&payload_flag, false) < 0) {
//     printf("Error creating payload flag.\n");
//     return;
//   }

//   struct actual_bundle *bundle1;
//   if((bundle1= create_bundle()) == NULL){
//     printf("Could not create bundle.\n");
//     return;
//   }
//   print_bundle_storage();
//   fill_bundle(bundle1, 7, IPN, dtn_dst, "123", DUMMY_PAYLOAD_LIFETIME, CRC_32, "1234");
//   // printf("primary block of bundle filled.\n");
//   bundle_add_block(bundle1, BUNDLE_BLOCK_TYPE_PAYLOAD, payload_flag, payload_data, NOCRC, data_len);
  
//   /* Creating bundle age block*/
//   size_t bundle_age_len;
//   uint8_t *bundle_age_data;
//   uint64_t bundle_age_flag;

//   if (calculate_canonical_flag(&bundle_age_flag, false) < 0) {
//     printf("Error creating payload flag.\n");
//     return ;
//   }

//   uint32_t initial_bundle_age = 0;
//   bundle_age_len = 1;
//   bundle_age_data = malloc(bundle_age_len * sizeof(char));
//   sprintf((char*)bundle_age_data, "%lu", initial_bundle_age);

//   bundle_add_block(bundle1, BUNDLE_BLOCK_TYPE_BUNDLE_AGE, bundle_age_flag, bundle_age_data, NOCRC, bundle_age_len);



//   if(!gnrc_bp_dispatch(GNRC_NETTYPE_BP, GNRC_NETREG_DEMUX_CTX_ALL, bundle1, GNRC_NETAPI_MSG_TYPE_SND)) {
//     printf("Unable to find BP thread.\n");
//     delete_bundle(bundle1);
//     return ;
//   }
// }

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
      send_bundle((uint8_t *)argv[4], atoi(argv[5]),argv[2], argv[3], 9, "1", NOCRC, DUMMY_PAYLOAD_LIFETIME);
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
