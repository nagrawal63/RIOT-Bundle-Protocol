/**
 * @ingroup     Bundle protocol
 * @{
 *
 * @file
 * @brief       Periodic scheduler for discovery management in bundle protocol
 *
 * @author      Nishchay Agrawal <agrawal.nishchay5@gmail.com>
 *
 * @}
 */
#include "thread.h"
#include "kernel_types.h"
#include "xtimer.h"

#include "net/gnrc/bundle_protocol/contact_scheduler_periodic.h"
#include "net/gnrc/bundle_protocol/bundle.h"
#include "net/gnrc/bundle_protocol/bundle_storage.h"
#include "net/gnrc/bundle_protocol/agent.h"
#include "net/gnrc/pkt.h"
#include "net/gnrc/netif/internal.h"
#include "net/gnrc.h"
#include "net/gnrc/netif/hdr.h"

#define ENABLE_DEBUG  (1)
#include "debug.h"
#include "od.h"

#define DISCOVERY_SEND_DATA 1

#if ENABLE_DEBUG
static char _stack[GNRC_CONTACT_MANAGER_STACK_SIZE + THREAD_EXTRA_STACKSIZE_PRINTF];
#else
static char _stack[GNRC_CONTACT_MANAGER_STACK_SIZE];
#endif

static kernel_pid_t _pid = KERNEL_PID_UNDEF;
// int iface = 0;

static void *contact_scheduler(void * args);
static uint8_t *_encode_discovery_bundle(struct actual_bundle *bundle, size_t *required_size);

kernel_pid_t gnrc_contact_scheduler_periodic_init(void)
{
  if(_pid > KERNEL_PID_UNDEF){
    return _pid;
  }

  _pid = thread_create(_stack, sizeof(_stack), GNRC_CONTACT_SCHEDULER_PRIO, THREAD_CREATE_STACKTEST, contact_scheduler, NULL, "contact_scheduler");

  DEBUG("contact_scheduler: Thread created with pid: %d.\n", _pid);
  return _pid;
}
//
static uint8_t *_encode_discovery_bundle(struct actual_bundle *bundle, size_t *required_size)
{
  nanocbor_encoder_t enc;
  nanocbor_encoder_init(&enc, NULL, 0);
  // DEBUG("contact_scheduler: Will try to encode bundle for the first time.\n");
  bundle_encode(bundle, &enc);
  *required_size = nanocbor_encoded_len(&enc);
  // DEBUG("contact_scheduler: space required to encode this discovery bundle: %u.\n", (uint8_t)*required_size);
  uint8_t *buf = malloc(*required_size);
  nanocbor_encoder_init(&enc, buf, *required_size);
  bundle_encode(bundle, &enc);
  return buf;
}

int send(int data)
{
  DEBUG("contact_scheduler_periodic: Will send discovery packet to interface : %d.\n", iface);
  (void)data;
  gnrc_pktsnip_t *discovery_packet;
  gnrc_netif_t *netif = NULL;
  size_t size = 0, data_len;
  uint8_t *payload_data, *buf_data;
  uint64_t payload_flag;

  netif = gnrc_netif_get_by_pid(iface);

  data_len = netif->l2addr_len;
  payload_data = (uint8_t*)malloc(data_len);
  memcpy(payload_data, netif->l2addr, data_len);
  if (calculate_canonical_flag(&payload_flag, false) < 0) {
    DEBUG("contact_scheduler: Error making discovery payload flag.\n");
    return ERROR;
  }

  struct actual_bundle *bundle = create_bundle();
  set_retention_constraint(bundle, DISPATCH_PENDING_RETENTION_CONSTRAINT);
  if (bundle == NULL) {
    DEBUG("contact_scheduler: Could not obtain space for bundle.\n");
    return ERROR;
  }
  fill_bundle(bundle, 7, IPN, BROADCAST_EID, NULL, 1, NOCRC, CONTACT_MANAGER_SERVICE_NUM);
  bundle_add_block(bundle, BUNDLE_BLOCK_TYPE_PAYLOAD, payload_flag, payload_data, NOCRC, data_len);
  // print_bundle(bundle);

  buf_data = _encode_discovery_bundle(bundle, &size);
  if(buf_data == NULL) {
    DEBUG("contact_scheduler: Unable to encode bundle.\n");
    set_retention_constraint(bundle, NO_RETENTION_CONSTRAINT);
    delete_bundle(bundle);
    return ERROR;
  }
  DEBUG("contact_scheduler: Encoded discovery bundle: ");
  for(int i=0;i<(int)size;i++){
    printf("%02x",buf_data[i]);
  }
  printf(" at %p\n", bundle);
  discovery_packet = gnrc_pktbuf_add(NULL, buf_data, (int)size, GNRC_NETTYPE_CONTACT_MANAGER);
  if (discovery_packet == NULL) {
    DEBUG("contact_scheduler: unable to copy data to discovery packet buffer.\n");
    set_retention_constraint(bundle, NO_RETENTION_CONSTRAINT);
    delete_bundle(bundle);
    return ERROR;
  }

  if (netif != NULL) {
      gnrc_pktsnip_t *netif_hdr = gnrc_netif_hdr_build(NULL, 0, NULL, 0);
      // DEBUG("contact_scheduler: netif hdr data is %s.\n",(char *)netif_hdr->data);
      gnrc_netif_hdr_set_netif(netif_hdr->data, netif);
      LL_PREPEND(discovery_packet, netif_hdr);
  }
  DEBUG("contact_scheduler_periodic: Sending discovery packet.\n");
  if(!gnrc_netapi_dispatch_send(GNRC_NETTYPE_CONTACT_MANAGER, GNRC_NETREG_DEMUX_CTX_ALL, discovery_packet)) {
    DEBUG("contact_scheduler: Unable to find BP thread.\n");
    gnrc_pktbuf_release(discovery_packet);
    return ERROR;
  }
  // print_bundle_storage();
  free(payload_data);
  set_retention_constraint(bundle, NO_RETENTION_CONSTRAINT);
  delete_bundle(bundle);
  return 0;
}

void *contact_scheduler (void *args)
{
  (void)args;
  while(1){
    //message send command to discover new nodes
    xtimer_sleep(CONTACT_PERIOD_SECONDS);
    if(send(DISCOVERY_SEND_DATA) < 0) {
      DEBUG("contact_scheduler: Couldn't send discovery packet.\n");
    }
  }
  return NULL;
}
