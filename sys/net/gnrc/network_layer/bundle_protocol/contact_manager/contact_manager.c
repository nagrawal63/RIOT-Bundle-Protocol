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

static void _receive(void);
static void _send(void);
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


static void _receive(void)
{

}

static void _send(void)
{

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
          _send();
          break;
      case GNRC_NETAPI_MSG_TYPE_RCV:
          DEBUG("contact_manager: GNRC_NETDEV_MSG_TYPE_RCV received\n");
          _receive();
          break;
      default:
        DEBUG("contact_manager: Successfully entered contact manager, yayyyyyy!!\n");
        break;
    }
  }
  return NULL;
}
