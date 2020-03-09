

#include "kernel_types.h"
#include "thread.h"
#include "utlist.h"

#include "net/gnrc/netif.h"
#include "net/gnrc/bp.h"
#include "net/gnrc.h"
#include "net/gnrc/pktbuf.h"
#include "net/gnrc/bundle_protocol/config.h"
#include "net/gnrc/bundle_protocol/bundle_storage.h"

#define ENABLE_DEBUG    (1)
#include "debug.h"

static kernel_pid_t _pid = KERNEL_PID_UNDEF;

#if ENABLE_DEBUG
static char _stack[GNRC_BP_STACK_SIZE +THREAD_EXTRA_STACKSIZE_PRINTF];
#else
static char _stack[GNRC_BP_STACK_SIZE];
#endif

static void _receive(gnrc_pktsnip_t *pkt);
static void _send(struct actual_bundle *bundle);
static void _send_discovery_packet(gnrc_pktsnip_t *pkt);
static void *_event_loop(void *args);

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

static void _receive(gnrc_pktsnip_t *pkt)
{
    DEBUG("bp: Receive type: %d with length: %d and data: %s\n",pkt->type, pkt->size, (uint8_t*)pkt->data);
    if(pkt->data == NULL) {
      DEBUG("bp: No data in packet, dropping it.\n");
      gnrc_pktbuf_release(pkt);
      return ;
    }

    struct actual_bundle *bundle = create_bundle();
    bundle_decode(bundle, pkt->data, pkt->size);
    DEBUG("bp: Printing received packet!!!!!!!!!!!!!!!!!!!!.\n");
    print_bundle(bundle);
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

    return ;
}

static void _send(struct actual_bundle *bundle)
{
    (void) bundle;
    DEBUG("bp: Send type: %d\n",bundle->primary_block.version);

    int iface = 9;
    gnrc_netif_t *netif = NULL;
    nanocbor_encoder_t enc;

    netif = gnrc_netif_get_by_pid(iface);
    DEBUG("bp: Sending bundle to hardcoded interface %d.\n", iface);

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
      printf("unable to copy data to discovery packet buffer.\n");
      delete_bundle(bundle);
      free(buf);
      return ;
    }

    if (netif != NULL) {
        gnrc_pktsnip_t *netif_hdr = gnrc_netif_hdr_build(NULL, 0, NULL, 0);
        printf("netif hdr data is %s.\n",(char *)netif_hdr->data);
        gnrc_netif_hdr_set_netif(netif_hdr->data, netif);
        LL_PREPEND(pkt, netif_hdr);
    }
    delete_bundle(bundle);
    return ;
}

static void _send_discovery_packet(gnrc_pktsnip_t *pkt)
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
            _send_discovery_packet(msg.content.ptr);
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
