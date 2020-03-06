#include "thread.h"
#include "kernel_types.h"
#include "xtimer.h"

#include "net/gnrc/bundle_protocol/contact_scheduler_periodic.h"
#include "net/gnrc/bundle_protocol/bundle.h"
#include "net/gnrc/bundle_protocol/bundle_storage.h"
#include "net/gnrc/pkt.h"
#include "net/gnrc/netif/internal.h"
#include "net/gnrc.h"

#define ENABLE_DEBUG  (1)
#include "debug.h"

#define DISCOVERY_SEND_DATA 1

#if ENABLE_DEBUG
static char _stack[GNRC_CONTACT_MANAGER_STACK_SIZE + THREAD_EXTRA_STACKSIZE_PRINTF];
#else
static char _stack[GNRC_CONTACT_MANAGER_STACK_SIZE];
#endif

static kernel_pid_t _pid = KERNEL_PID_UNDEF;
static char *BROADCAST_DEST_ID = "0000";

static void *contact_scheduler(void * args);
static uint8_t *_encode_discovery_bundle(struct actual_bundle *bundle, size_t *required_size);

kernel_pid_t gnrc_contact_scheduler_periodic_init(void)
{
  if(_pid > KERNEL_PID_UNDEF){
    return _pid;
  }

  _pid = thread_create(_stack, sizeof(_stack), GNRC_CONTACT_MANAGER_PRIO, THREAD_CREATE_STACKTEST, contact_scheduler, NULL, "contact_scheduler");
  // to be part of bundle_agent

  DEBUG("contact_scheduler: Thread created with pid: %d.\n", _pid);
  return _pid;
}

static uint8_t *_encode_discovery_bundle(struct actual_bundle *bundle, size_t *required_size)
{
  nanocbor_encoder_t enc;
  nanocbor_encoder_init(&enc, NULL, 0);
  bundle_encode(bundle, &enc);
  *required_size = nanocbor_encoded_len(&enc);
  uint8_t *buf = malloc(*required_size);
  nanocbor_encoder_init(&enc, buf, *required_size);
  bundle_encode(bundle, &enc);
  return buf;
}

int send(char *addr_str, int data, int iface)
{
  (void)addr_str;
  (void)data;
  gnrc_pktsnip_t *discovery_packet;
  gnrc_netif_t *netif = NULL;
  size_t size = 0;

  struct actual_bundle *bundle = create_bundle();
  if (bundle == NULL) {
    DEBUG("contact_scheduler: Could not obtain space for bundle.\n");
    return ERROR;
  }
  fill_bundle(bundle, 7, IPN, BROADCAST_EID, NULL, 1, NOCRC, CONTACT_MANAGER_SERVICE_NUM);
  print_bundle(bundle);
  uint8_t *buf_data = _encode_discovery_bundle(bundle, &size);
  if(buf_data == NULL) {
    DEBUG("contact_scheduler: Unable to encode bundle.\n");
    delete_bundle(bundle);
    return ERROR;
  }
  printf("Encoded bundle: ");
  for(int i=0;i<(int)size;i++){
    printf("%02x",buf_data[i]);
  }
  printf(" at %p\n", bundle);
  discovery_packet = gnrc_pktbuf_add(NULL, buf_data, (int)size, GNRC_NETTYPE_CONTACT_MANAGER);
  if (discovery_packet == NULL) {
    DEBUG("contact_scheduler: unable to copy data to discovery packet buffer.\n");
    delete_bundle(bundle);
    return ERROR;
  }

  netif = gnrc_netif_get_by_pid(iface);

  if (netif != NULL) {
      gnrc_pktsnip_t *netif_hdr = gnrc_netif_hdr_build(NULL, 0, NULL, 0);
      printf("contact_scheduler: netif hdr data is %s.\n",(char *)netif_hdr->data);
      gnrc_netif_hdr_set_netif(netif_hdr->data, netif);
      LL_PREPEND(discovery_packet, netif_hdr);
  }
  DEBUG("contact_scheduler: Dispatching packet send.\n");
  if(!gnrc_netapi_dispatch_send(GNRC_NETTYPE_CONTACT_MANAGER, GNRC_NETREG_DEMUX_CTX_ALL, discovery_packet)) {
    DEBUG("contact_scheduler: Unable to find BP thread.\n");
    gnrc_pktbuf_release(discovery_packet);
    return ERROR;
  }
  // print_bundle_storage();
  delete_bundle(bundle);
  return 0;
}

void *contact_scheduler (void *args)
{
  (void)args;
  int iface = 9;
  while(1){
    //message send command to discover new nodes
    xtimer_sleep(CONTACT_PERIOD_SECONDS);
    DEBUG("Trying to send discovery packet.\n");
    DEBUG("Scheduling discovery packet over hard coded interface : %d.\n", iface);
    if(send(BROADCAST_DEST_ID, DISCOVERY_SEND_DATA, iface) < 0) {
      DEBUG("contact_scheduler: Couldn't send discovery packet.\n");
    }
  }
  return NULL;
}
