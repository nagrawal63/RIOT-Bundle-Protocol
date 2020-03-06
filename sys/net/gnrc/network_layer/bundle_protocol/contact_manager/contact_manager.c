#include "thread.h"
#include "kernel_types.h"
#include "utlist.h"

#include "net/gnrc/netif.h"
#include "net/gnrc/bundle_protocol/contact_manager.h"
#include "net/gnrc.h"
#include "net/gnrc/netif.h"

#define ENABLE_DEBUG  (1)
#include "debug.h"

static kernel_pid_t _pid = KERNEL_PID_UNDEF;

#if ENABLE_DEBUG
static char _stack[GNRC_CONTACT_MANAGER_STACK_SIZE + THREAD_EXTRA_STACKSIZE_PRINTF];
#else
static char _stack[GNRC_CONTACT_MANAGER_STACK_SIZE];
#endif

static gnrc_pktsnip_t *_create_netif_hdr(uint8_t *dst_l2addr, unsigned dst_l2addr_len, gnrc_pktsnip_t *pkt, uint8_t flags);
static void _receive(gnrc_pktsnip_t *pkt);
static void _send(gnrc_pktsnip_t *pkt);
static void *_event_loop(void* args);

kernel_pid_t gnrc_contact_manager_init(void)
{
  if(_pid > KERNEL_PID_UNDEF){
    return _pid;
  }

  _pid = thread_create(_stack, sizeof(_stack), GNRC_CONTACT_MANAGER_PRIO, THREAD_CREATE_STACKTEST, _event_loop, NULL, "contact_manager");

  DEBUG("contact_manager: Thread created with pid: %d.\n", _pid);
  return _pid;
}

static gnrc_pktsnip_t *_create_netif_hdr(uint8_t *dst_l2addr, unsigned dst_l2addr_len, gnrc_pktsnip_t *pkt, uint8_t flags)
{
  gnrc_pktsnip_t *netif_hdr = gnrc_netif_hdr_build(NULL, 0, dst_l2addr, dst_l2addr_len);
  gnrc_netif_hdr_t *hdr;

  if (netif_hdr == NULL) {
      DEBUG("contact_manager: error on interface header allocation, dropping packet\n");
      gnrc_pktbuf_release(pkt);
      return NULL;
  }
  hdr = netif_hdr->data;
  /* previous netif header might have been allocated by some higher layer
   * to provide some flags (provided to us via netif_flags). */
  hdr->flags = flags;

  /* add netif_hdr to front of the pkt list */
  LL_PREPEND(pkt, netif_hdr);

  return pkt;
}

static void _receive(gnrc_pktsnip_t *pkt)
{
  gnrc_netif_t *netif = NULL;
  gnrc_pktsnip_t *netif_hdr;
  assert(pkt != NULL);

  netif_hdr = gnrc_pktsnip_search_type(pkt, GNRC_NETTYPE_NETIF);

  if (netif_hdr != NULL) {
    netif = gnrc_netif_hdr_get_netif(netif_hdr->data);
    DEBUG("contact_manager: netif is %s.\n", !netif?"not null": "null");
  }

  if (pkt->data == NULL) {
    DEBUG("contact_manager: discovery_packet received is not discovery type, dropping it.\n");
    gnrc_pktbuf_release(pkt);
    return;
  }

  DEBUG("contact_manager: size of discovry packet received is %d and data in it is %s.\n", pkt->size, (char*) pkt->data);
}

static void _send(gnrc_pktsnip_t *pkt)
{
  gnrc_netif_t *netif = NULL;
  gnrc_pktsnip_t *tmp_pkt;
  uint8_t netif_hdr_flags = 0U;
  int iface = 0;

  if(pkt->type == GNRC_NETTYPE_NETIF) {
    const gnrc_netif_hdr_t *netif_hdr = pkt->data;

    netif = gnrc_netif_hdr_get_netif(pkt->data);
    /* discard broadcast and multicast flags because those could be
     * potentially wrong (dst is later checked to assure that multicast is
     * set if dst is a multicast address) */
    netif_hdr_flags = netif_hdr->flags &
                      ~(GNRC_NETIF_HDR_FLAGS_BROADCAST |
                        GNRC_NETIF_HDR_FLAGS_MULTICAST);

    tmp_pkt = gnrc_pktbuf_start_write(pkt);
    if (tmp_pkt == NULL) {
        DEBUG("contact_manager: unable to get write access to netif header, dropping packet\n");
        gnrc_pktbuf_release(pkt);
        return;
    }
    pkt = gnrc_pktbuf_remove_snip(tmp_pkt, tmp_pkt);
  }
  // TODO: Add broadcast destination l2adrr and addr length here instead of NULL and 0.
  if ((pkt = _create_netif_hdr(NULL, 0, pkt, netif_hdr_flags | GNRC_NETIF_HDR_FLAGS_BROADCAST)) == NULL) {
    return ;
  }
  iface = netif->pid;

  if(iface != 0) {
    DEBUG("contact_manager: Sending discovery packet.\n");
    DEBUG("contact_manager: type of packet before sending it: %d.\n", pkt->next->type);
    gnrc_netapi_send(iface, pkt);
  }
}

static void *_event_loop(void *args)
{
  msg_t msg, msg_q[GNRC_CONTACT_MANAGER_MSG_QUEUE_SIZE];

  gnrc_netreg_entry_t me_reg = GNRC_NETREG_ENTRY_INIT_PID(GNRC_NETREG_DEMUX_CTX_ALL, sched_active_pid);
  (void)args;

  msg_init_queue(msg_q, GNRC_CONTACT_MANAGER_MSG_QUEUE_SIZE);

  gnrc_netreg_register(GNRC_NETTYPE_CONTACT_MANAGER, &me_reg);
  while(1){
    DEBUG("contact_manager: waiting for incoming message.\n");
    msg_receive(&msg);
    switch(msg.type){
      case GNRC_NETAPI_MSG_TYPE_SND:
          DEBUG("contact_manager: GNRC_NETDEV_MSG_TYPE_SND received\n");
          _send(msg.content.ptr);
          break;
      case GNRC_NETAPI_MSG_TYPE_RCV:
          DEBUG("contact_manager: GNRC_NETDEV_MSG_TYPE_RCV received\n");
          _receive(msg.content.ptr);
          break;
      default:
        DEBUG("contact_manager: Successfully entered contact manager, yayyyyyy!!\n");
        break;
    }
  }
  return NULL;
}
