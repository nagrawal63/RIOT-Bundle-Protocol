#include "thread.h"
#include "kernel_types.h"
#include "xtimer.h"

#include "net/gnrc/bundle_protocol/contact_scheduler_periodic.h"

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

kernel_pid_t gnrc_contact_scheduler_periodic_init(void)
{
  if(_pid > KERNEL_PID_UNDEF){
    return _pid;
  }

  _pid = thread_create(_stack, sizeof(_stack), GNRC_CONTACT_MANAGER_PRIO, THREAD_CREATE_STACKTEST, contact_scheduler, NULL, "contact_scheduler");

  DEBUG("contact_scheduler: Thread created with pid: %d.\n", _pid);
  return _pid;
}

void send(char *addr_str, int data)
{
  (void)addr_str;
  (void)data;


}

void *contact_scheduler (void *args)
{
  (void)args;
  while(1){
    //message send command to discover new nodes
    DEBUG("Trying to send discovery packet.\n");
    send(BROADCAST_DEST_ID, DISCOVERY_SEND_DATA);
    xtimer_sleep(CONTACT_PERIOD_SECONDS);
  }
  return NULL;
}
