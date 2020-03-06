

#include "kernel_types.h"
#include "net/gnrc.h"
#include "thread.h"
#include "utlist.h"

#include "net/gnrc/netif.h"
#include "net/gnrc/bp.h"
#include "net/gnrc/bundle_protocol/bundle.h"
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
static void _send(gnrc_pktsnip_t *pkt);
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
    
  #ifdef MODULE_GNRC_CONTACT_MANAGER
    if (bundle->primary_block.service_num  == (uint32_t)atoi(CONTACT_MANAGER_SERVICE_NUM)) {
        if (!gnrc_netapi_dispatch_receive(GNRC_NETTYPE_CONTACT_MANAGER, GNRC_NETREG_DEMUX_CTX_ALL, pkt)) {
          DEBUG("bp: no contact_manager thread found\n");
          delete_bundle(bundle);
        }
    }
  #endif

    return ;
}

static void _send(gnrc_pktsnip_t *pkt)
{
    DEBUG("bp: Send type: %d\n",pkt->type);
    return ;
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
