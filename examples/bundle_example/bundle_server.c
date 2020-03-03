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

#include "net/gnrc.h"
#include "net/gnrc/netif.h"
#include "net/gnrc/netif/hdr.h"
#include "net/gnrc/pktdump.h"
#include "net/gnrc/bundle_protocol/bundle.h"
#include "net/gnrc/bundle_protocol/bundle_storage.h"
#include "timex.h"
#include "utlist.h"
#include "xtimer.h"

static gnrc_netreg_entry_t bundle_server = GNRC_NETREG_ENTRY_INIT_PID(GNRC_NETREG_DEMUX_CTX_ALL, KERNEL_PID_UNDEF);

static void send_bundle(char *dtn_dst, char *port_str, char *data)
{
  // gnrc_netif_t* netif = NULL;
  // char* iface;
  // uint16_t port;
  (void) dtn_dst;
  (void) port_str;
  (void) data;

  // struct actual_bundle *bundle1, *bundle2, *bundle3 ;
  struct actual_bundle *bundle1;
  if((bundle1= create_bundle()) == NULL){
    printf("Could not create bundle.\n");
    return;
  }
  print_bundle_storage();
  fill_bundle(bundle1, 7, DTN, dtn_dst, NULL, 1, NOCRC);
  // printf("bundle: Set version to %d.\n",bundle1->primary_block.version);
  print_bundle(bundle1);
  // if((bundle2 = create_bundle()) == NULL){
  //   printf("Could not create bundle.\n");
  //   return;
  // }
  // fill_bundle(bundle2, 7, DTN, dtn_dst, NULL, 1, NOCRC);
  // print_bundle_storage();
  // if((bundle3= create_bundle()) == NULL){
  //   printf("Could not create bundle.\n");
  //   return;
  // }
  // fill_bundle(bundle3, 7, DTN, dtn_dst, NULL, 1, NOCRC);
  // print_bundle_storage();
  // delete_bundle(bundle1);
  // print_bundle_storage();

  //Bundle getting encoded
  nanocbor_encoder_t enc;
  nanocbor_encoder_init(&enc, NULL, 0);
  bundle_encode(bundle1, &enc);
  size_t required_size = nanocbor_encoded_len(&enc);
  uint8_t *buf = malloc(required_size);
  nanocbor_encoder_init(&enc, buf, required_size);
  bundle_encode(bundle1, &enc);
  printf("Encoded bundle: ");
  for(int i=0;i<(int)required_size;i++){
    printf("%02x",buf[i]);
  }
  printf(" at %p\n", bundle1);
  gnrc_pktsnip_t *payload = gnrc_pktbuf_add(NULL, buf, required_size, GNRC_NETTYPE_BP);
  int iface = 9;
  gnrc_netif_t *netif = NULL;
  netif = gnrc_netif_get_by_pid(iface);
  if (netif != NULL) {
          gnrc_pktsnip_t *netif_hdr = gnrc_netif_hdr_build(NULL, 0, NULL, 0);
          printf("netif hdr data is %s.\n",(char *)netif_hdr->data);
          gnrc_netif_hdr_set_netif(netif_hdr->data, netif);
          LL_PREPEND(payload, netif_hdr);
  }
  //Send bundle
  // netdev_t *dev = netif->dev;
  // uint8_t mhr[IEEE802154_MAX_HDR_LEN] = {0};
  gnrc_netapi_send(iface, payload);
  // iolist_t iolist = {
  //     .iol_next = (iolist_t *)buf,
  //     .iol_base = mhr,
  //     .iol_len = required_size
  // };
  // int res = dev->driver->send(dev, &iolist);
  //Packet too large because it designed to travel over like a list
  // printf("Result of sending finally is: %d.\n",res);
}

//
// static void send(char *addr_str, char *port_str, char *data, unsigned int num,
//                  unsigned int delay)
// {
//     gnrc_netif_t *netif = NULL;
//     char *iface;
//     uint16_t port;
//     ipv6_addr_t addr;
//
//     // printf("Address before parsing iface: %s.\n",addr_str);
//     iface = ipv6_addr_split_iface(addr_str);
//     if ((!iface) && (gnrc_netif_numof() == 1)) {
//       printf("Interface is null.\n");
//         netif = gnrc_netif_iter(NULL);
//     }
//     else if (iface) {
//       printf("Interface is not null with value %d.\n", atoi(iface));
//         netif = gnrc_netif_get_by_pid(atoi(iface));
//     } else {
//       printf("Falling back to 8.\n");
//         netif = gnrc_netif_get_by_pid(8);
//     }
//     printf("iface is %d.\n",*iface);
//     printf("netif is %s.\n", !netif?"not there":"there");
//     // printf("Address after parsing iface: %s.\n",addr_str);
//     /* parse destination address */
//     if (ipv6_addr_from_str(&addr, addr_str) == NULL) {
//         puts("Error: unable to parse destination address");
//         return;
//     }
//     /* parse port */
//     port = atoi(port_str);
//     if (port == 0) {
//         puts("Error: unable to parse destination port");
//         return;
//     }
//
//     for (unsigned int i = 0; i < num; i++) {
//         gnrc_pktsnip_t *payload, *udp, *ip;
//         unsigned payload_size;
//         /* allocate payload */
//         payload = gnrc_pktbuf_add(NULL, data, strlen(data), GNRC_NETTYPE_UNDEF);
//         if (payload == NULL) {
//             puts("Error: unable to copy data to packet buffer");
//             return;
//         }
//         /* store size for output */
//         payload_size = (unsigned)payload->size;
//         /* allocate UDP header, set source port := destination port */
//         udp = gnrc_udp_hdr_build(payload, port, port);
//         if (udp == NULL) {
//             puts("Error: unable to allocate UDP header");
//             gnrc_pktbuf_release(payload);
//             return;
//         }
//         /* allocate IPv6 header */
//         ip = gnrc_ipv6_hdr_build(udp, NULL, &addr);
//         if (ip == NULL) {
//             puts("Error: unable to allocate IPv6 header");
//             gnrc_pktbuf_release(udp);
//             return;
//         }
//         /* add netif header, if interface was given */
//         if (netif != NULL) {
//             gnrc_pktsnip_t *netif_hdr = gnrc_netif_hdr_build(NULL, 0, NULL, 0);
//
//             gnrc_netif_hdr_set_netif(netif_hdr->data, netif);
//             LL_PREPEND(ip, netif_hdr);
//         }
//         /* send packet */
//         if (!gnrc_netapi_dispatch_send(GNRC_NETTYPE_BP, GNRC_NETREG_DEMUX_CTX_ALL, ip)) {
//             puts("Error: unable to locate UDP thread");
//             gnrc_pktbuf_release(ip);
//             return;
//         }
//         /* access to `payload` was implicitly given up with the send operation above
//          * => use temporary variable for output */
//         printf("Success: sent %u byte(s) to [%s]:%u\n", payload_size, addr_str,
//                port);
//         xtimer_usleep(delay);
//     }
// }

static void start_bundle_server(char *port_str)
{
    uint16_t port;

    /* check if bundle_server is already running */
    if (bundle_server.target.pid != KERNEL_PID_UNDEF) {
        printf("Error: bundle_server already running on port %" PRIu32 "\n",
               bundle_server.demux_ctx);
        return;
    }
    /* parse port */
    port = atoi(port_str);
    if (port == 0) {
        puts("Error: invalid port specified");
        return;
    }
    /* start bundle_server (which means registering pktdump for the chosen port) */
    bundle_server.target.pid = gnrc_pktdump_pid;
    bundle_server.demux_ctx = (uint32_t)port;
    gnrc_netreg_register(GNRC_NETTYPE_BP, &bundle_server);
    printf("Success: started bundle_server on port %" PRIu16 "\n", port);
}

static void stop_bundle_server(void)
{
    /* check if bundle_server is running at all */
    if (bundle_server.target.pid == KERNEL_PID_UNDEF) {
        printf("Error: bundle_server was not running\n");
        return;
    }
    /* stop bundle_server */
    gnrc_netreg_unregister(GNRC_NETTYPE_BP, &bundle_server);
    bundle_server.target.pid = KERNEL_PID_UNDEF;
    puts("Success: stopped bundle_server");
}

int bundle_cmd(int argc, char **argv)
{
    if (argc < 2) {
        printf("usage: %s [send|bundle_server]\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "send") == 0) {
        // uint32_t num = 1;
        // uint32_t delay = 1000000;
        if (argc < 5) {
            printf("usage: %s send <addr> <port> <data> [<num> [<delay in us>]]\n",
                   argv[0]);
            return 1;
        }
        // if (argc > 5) {
        //     num = atoi(argv[5]);
        // }
        // if (argc > 6) {
        //     delay = atoi(argv[6]);
        // }
        send_bundle(argv[2], argv[3], argv[4]);
    }
    else if (strcmp(argv[1], "bundle_server") == 0) {
        if (argc < 3) {
            printf("usage: %s bundle_server [start|stop]\n", argv[0]);
            return 1;
        }
        if (strcmp(argv[2], "start") == 0) {
            if (argc < 4) {
                printf("usage %s bundle_server start <port>\n", argv[0]);
                return 1;
            }
            start_bundle_server(argv[3]);
        }
        else if (strcmp(argv[2], "stop") == 0) {
            stop_bundle_server();
        }
        else {
            puts("error: invalid command");
        }
    }
    else {
        puts("error: invalid command");
    }
    return 0;
}
