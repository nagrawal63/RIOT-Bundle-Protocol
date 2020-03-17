

#include "kernel_types.h"
#include "thread.h"
#include "utlist.h"

#include "net/gnrc/netif.h"
#include "net/gnrc/bp.h"
#include "net/gnrc.h"
#include "net/gnrc/pktbuf.h"
#include "net/gnrc/bundle_protocol/config.h"
#include "net/gnrc/bundle_protocol/bundle_storage.h"
#include "net/gnrc/bundle_protocol/routing.h"

#define ENABLE_DEBUG    (1)
#include "debug.h"

static kernel_pid_t _pid = KERNEL_PID_UNDEF;

//Hardcoded right now, find a way to change this soon
int iface = 9;


#if ENABLE_DEBUG
static char _stack[GNRC_BP_STACK_SIZE +THREAD_EXTRA_STACKSIZE_PRINTF];
#else
static char _stack[GNRC_BP_STACK_SIZE];
#endif

static void _receive(gnrc_pktsnip_t *pkt);
static void _send(struct actual_bundle *bundle);
static void _send_packet(gnrc_pktsnip_t *pkt);
static void *_event_loop(void *args);
static void print_potential_neighbor_list(struct neighbor_t* neighbors);

kernel_pid_t gnrc_bp_init(void)
{
  if(_pid > KERNEL_PID_UNDEF){
      return _pid;
  }

  _pid = thread_create(_stack, sizeof(_stack), GNRC_BP_PRIO,
                        THREAD_CREATE_STACKTEST, _event_loop, NULL, "bp");

  DEBUG("bp: thread created with pid: %d\n",_pid);
  return _pid;
}

kernel_pid_t gnrc_bp_get_pid(void)
{
    return _pid;
}

int gnrc_bp_dispatch(gnrc_nettype_t type, uint32_t demux_ctx, struct actual_bundle *bundle, uint16_t cmd)
{
  int numof = gnrc_netreg_num(type, demux_ctx);
  if (numof != 0){
    gnrc_netreg_entry_t *sendto = gnrc_netreg_lookup(type, demux_ctx);
    msg_t msg;
    /* set the outgoing message's fields */
    msg.type = cmd;
    msg.content.ptr = (void *)bundle;
    /* send message */
    int ret = msg_try_send(&msg, sendto->target.pid);
    if (ret < 1) {
        DEBUG("gnrc_bp: dropped message to %" PRIkernel_pid " (%s)\n", sendto->target.pid,
              (ret == 0) ? "receiver queue is full" : "invalid receiver");
    }
    return ret;
  }
  return ERROR;
}

void process_bundle_before_forwarding(struct actual_bundle *bundle) {
  (void) bundle;
}

static void _receive(gnrc_pktsnip_t *pkt)
{
    DEBUG("bp: Receive type: %d with length: %d and data: %s\n",pkt->type, pkt->size, (uint8_t*)pkt->data);
    if(pkt->data == NULL) {
      DEBUG("bp: No data in packet, dropping it.\n");
      gnrc_pktbuf_release(pkt);
      return ;
    }

    struct actual_bundle *bundle = create_bundle();
    if (bundle_decode(bundle, pkt->data, pkt->size) == ERROR) {
      DEBUG("bp: Packet received not for bundle protocol.\n");
      delete_bundle(bundle);
      return ;
    }
    // DEBUG("bp: Printing received packet!!!!!!!!!!!!!!!!!!!!.\n");
    // print_bundle(bundle);
    DEBUG("bp: will send packet to upper layer.\n");
  #ifdef MODULE_GNRC_CONTACT_MANAGER
    if (bundle->primary_block.service_num  == (uint32_t)atoi(CONTACT_MANAGER_SERVICE_NUM)) {
      // gnrc_pktsnip_t *tmp_pkt = gnrc_pktbuf_add(NULL, bundle, sizeof(bundle), GNRC_NETTYPE_CONTACT_MANAGER);
      if (!gnrc_bp_dispatch(GNRC_NETTYPE_CONTACT_MANAGER, GNRC_NETREG_DEMUX_CTX_ALL, bundle, GNRC_NETAPI_MSG_TYPE_RCV)) {
        DEBUG("bp: no contact_manager thread found\n");
        delete_bundle(bundle);
      }
      gnrc_pktbuf_release(pkt);
    }
  #endif
    else {
      DEBUG("bp: Not a discovery packet!!!!!!!!!!!!!!!!!!\n");
      /* This bundle is for the current node, send to application that sent it*/
      if (bundle->primary_block.dst_num == (uint32_t)atoi(get_src_num())) {
        if (!gnrc_bp_dispatch(GNRC_NETTYPE_BP, bundle->primary_block.service_num, bundle, GNRC_NETAPI_MSG_TYPE_RCV)) {
          DEBUG("bp: Couldn't send bundle to registered receivers.\n");
          delete_bundle(bundle);
        }
        else {
          send_ack();
          delete_bundle(bundle);
        }
      } /*Bundle not for this node, forward received bundle*/
      else {
        struct router *cur_router = get_router();
        nanocbor_encoder_t enc;
        gnrc_netif_t *netif = NULL;
        struct neighbor_t *temp;

        netif = gnrc_netif_get_by_pid(iface);
        DEBUG("bp: Sending bundle to hardcoded interface %d.\n", iface);

        struct neighbor_t *neighbors_to_send = cur_router->route_receivers(bundle->primary_block.dst_num);
        if (neighbors_to_send == NULL) {
          DEBUG("bp: Could not find neighbors to send bundle to.\n");
          delete_bundle(bundle);
          return ;
        }

        process_bundle_before_forwarding(bundle);

        nanocbor_encoder_init(&enc, NULL, 0);
        bundle_encode(bundle, &enc);
        size_t required_size = nanocbor_encoded_len(&enc);
        uint8_t *buf = malloc(required_size);
        nanocbor_encoder_init(&enc, buf, required_size);
        bundle_encode(bundle, &enc);
        printf("Encoded bundle while forwarding: ");
        for(int i=0;i<(int)required_size;i++){
          printf("%02x",buf[i]);
        }
        printf(" at %p\n", bundle);

        gnrc_pktsnip_t *pkt = gnrc_pktbuf_add(NULL, buf, (int)required_size, GNRC_NETTYPE_BP);
        if (pkt == NULL) {
          DEBUG("bp: unable to copy data to discovery packet buffer.\n");
          delete_bundle(bundle);
          free(buf);
          return ;
        }
        /* TODO: Think of the case when the data is sent to no node, then free the space allocated for the 
            encoded bundle*/
        LL_FOREACH(neighbors_to_send, temp) {
          if (temp->endpoint_scheme == IPN && temp->endpoint_num != (uint32_t)atoi(get_src_num())) {
            DEBUG("bp:Forwarding packet to neighbor with eid %lu.\n", temp->endpoint_num);
            if (netif != NULL) {
              gnrc_pktsnip_t *netif_hdr = gnrc_netif_hdr_build(NULL, 0, temp->l2addr, temp->l2addr_len);
              DEBUG("bp: netif hdr data is %s.\n",(char *)netif_hdr->data);
              gnrc_netif_hdr_set_netif(netif_hdr->data, netif);
              LL_PREPEND(pkt, netif_hdr);
            }
            if (netif->pid != 0) {
              DEBUG("bp: Sending discovery packet to process with pid %d.\n", netif->pid);
              gnrc_netapi_send(netif->pid, pkt);
            }
          }
        }
      }
    }

    return ;
}

static void _send(struct actual_bundle *bundle)
{
    (void) bundle;
    struct router *cur_router = get_router();
    struct neighbor_t *temp;
    DEBUG("bp: Send type: %d\n",bundle->primary_block.version);

    gnrc_netif_t *netif = NULL;
    nanocbor_encoder_t enc;

    netif = gnrc_netif_get_by_pid(iface);
    DEBUG("bp: Sending bundle to hardcoded interface %d.\n", iface);

    struct neighbor_t *neighbor_list_to_send = cur_router->route_receivers(bundle->primary_block.dst_num);
    print_potential_neighbor_list(neighbor_list_to_send);
    if (neighbor_list_to_send == NULL) {
      DEBUG("bp: Could not find neighbors to send bundle to.\n");
      // delete_bundle(bundle);
      return ;
    }

    nanocbor_encoder_init(&enc, NULL, 0);
    bundle_encode(bundle, &enc);
    size_t required_size = nanocbor_encoded_len(&enc);
    uint8_t *buf = malloc(required_size);
    nanocbor_encoder_init(&enc, buf, required_size);
    bundle_encode(bundle, &enc);
    printf("Encoded bundle: ");
    for(int i=0;i<(int)required_size;i++){
      printf("%02x",buf[i]);
    }
    printf(" at %p\n", bundle);

    gnrc_pktsnip_t *pkt = gnrc_pktbuf_add(NULL, buf, (int)required_size, GNRC_NETTYPE_BP);
    if (pkt == NULL) {
      DEBUG("bp: unable to copy data to discovery packet buffer.\n");
      delete_bundle(bundle);
      free(buf);
      return ;
    }
    LL_FOREACH(neighbor_list_to_send, temp) {
     if (netif != NULL) {
          gnrc_pktsnip_t *netif_hdr = gnrc_netif_hdr_build(NULL, 0, temp->l2addr, temp->l2addr_len);
          DEBUG("bp: netif hdr data is %s.\n",(char *)netif_hdr->data);
          gnrc_netif_hdr_set_netif(netif_hdr->data, netif);
          LL_PREPEND(pkt, netif_hdr);
      }
      if (netif->pid != 0) {
        DEBUG("bp: Sending discovery packet to process with pid %d.\n", netif->pid);
        gnrc_netapi_send(netif->pid, pkt);
      }
    }
    
    // delete_bundle(bundle);
    return ;
}

static void _send_packet(gnrc_pktsnip_t *pkt)
{
  gnrc_netif_t *netif = NULL;
  netif = gnrc_netif_hdr_get_netif(pkt->data);

  if (netif->pid != 0) {
    DEBUG("bp: Sending discovery packet to process with pid %d.\n", netif->pid);
    gnrc_netapi_send(netif->pid, pkt);
  }
}

static void *_event_loop(void *args)
{
  msg_t msg, msg_q[GNRC_BP_MSG_QUEUE_SIZE];

  gnrc_netreg_entry_t me_reg = GNRC_NETREG_ENTRY_INIT_PID(GNRC_NETREG_DEMUX_CTX_ALL, sched_active_pid);
  (void)args;

  msg_init_queue(msg_q, GNRC_BP_MSG_QUEUE_SIZE);

  gnrc_netreg_register(GNRC_NETTYPE_BP, &me_reg);
  while(1){
    DEBUG("bp: waiting for incoming message.\n");
    msg_receive(&msg);
    switch(msg.type){
      case GNRC_NETAPI_MSG_TYPE_SND:
          DEBUG("bp: GNRC_NETDEV_MSG_TYPE_SND received\n");
          if(strcmp(thread_get(msg.sender_pid)->name, "contact_manager") == 0) {
            _send_packet(msg.content.ptr);
            break;
          }
          _send(msg.content.ptr);
          break;
      case GNRC_NETAPI_MSG_TYPE_RCV:
          DEBUG("bp: GNRC_NETDEV_MSG_TYPE_RCV received\n");
          _receive(msg.content.ptr);
          break;
      default:
        DEBUG("bp: Successfully entered bp, yayyyyyy!!\n");
        break;
    }
  }
  return NULL;
}

static void print_potential_neighbor_list(struct neighbor_t* neighbors) {
  char addr_str[GNRC_NETIF_HDR_L2ADDR_PRINT_LEN];
  struct neighbor_t *temp;
  DEBUG("bp: Printing neighbor list: ");
  LL_FOREACH(neighbors, temp) {
    DEBUG("(%lu, %s )-> ", temp->endpoint_num, gnrc_netif_addr_to_str(temp->l2addr, temp->l2addr_len, addr_str));
  }
  DEBUG(".\n");
}

void send_bundles_to_new_neighbor(struct neighbor_t *neighbor) {
    
    // struct neighbor_t *temp;
    // DEBUG("bp: Send type: %d\n",bundle->primary_block.version);
    struct bundle_list *bundle_store_list, *temp_bundle;

    // netif = gnrc_netif_get_by_pid(iface);
    DEBUG("bp: Sending bundle to new neighbor on hardcoded interface %d.\n", iface);

    bundle_store_list = get_bundle_list();
    print_bundle_storage();
    LL_FOREACH(bundle_store_list, temp_bundle){
      if(temp_bundle->current_bundle.primary_block.dst_num != (uint32_t)atoi(BROADCAST_EID)) {

        gnrc_netif_t *netif = NULL;
        nanocbor_encoder_t enc;

        netif = gnrc_netif_get_by_pid(iface);

        DEBUG("bp: Sending this bundle to new neighbor.\n");
        print_bundle(&temp_bundle->current_bundle);

        nanocbor_encoder_init(&enc, NULL, 0);
        bundle_encode(&temp_bundle->current_bundle, &enc);
        size_t required_size = nanocbor_encoded_len(&enc);
        uint8_t *buf = malloc(required_size);
        nanocbor_encoder_init(&enc, buf, required_size);
        bundle_encode(&temp_bundle->current_bundle, &enc);
        printf("Encoded bundle: ");
        for(int i=0;i<(int)required_size;i++){
          printf("%02x",buf[i]);
        }
        printf(" at %p\n", &temp_bundle->current_bundle);

        gnrc_pktsnip_t *pkt = gnrc_pktbuf_add(NULL, buf, (int)required_size, GNRC_NETTYPE_BP);
        if (pkt == NULL) {
          DEBUG("bp: unable to copy data to packet buffer.\n");
          delete_bundle(&temp_bundle->current_bundle);
          free(buf);
          return ;
        }
        
       if (netif != NULL) {
            gnrc_pktsnip_t *netif_hdr = gnrc_netif_hdr_build(NULL, 0, neighbor->l2addr, neighbor->l2addr_len);
            // DEBUG("bp: netif hdr data is %s.\n",(char *)netif_hdr->data);
            gnrc_netif_hdr_set_netif(netif_hdr->data, netif);
            LL_PREPEND(pkt, netif_hdr);
        }
        if (netif->pid != 0) {
          DEBUG("bp: Sending stored packet to process with pid %d.\n", netif->pid);
          gnrc_netapi_send(netif->pid, pkt);
        }
      }
    }
    return ;
}

void send_ack(void) {

}