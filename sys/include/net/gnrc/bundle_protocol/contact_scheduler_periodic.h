#ifndef _CONTACT_SCHEDULER_PERIODIC_H
#define _CONTACT_SCHEDULER_PERIODIC_H

#include <stdbool.h>

#include "net/gnrc/bundle_protocol/contact_manager_config.h"

#define CONTACT_PERIOD_SECONDS 20

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Initialization of the CONTACT_SCHEDULER_PERIODIC thread.
 *
 * @details If CONTACT_SCHEDULER_PERIODIC was already initialized, it will just return the PID of
 *          the CONTACT_SCHEDULER_PERIODIC thread.
 *
 * @return  The PID to the CONTACT_SCHEDULER_PERIODIC thread, on success.
 * @return  -EINVAL, if @ref GNRC_CONTACT_SCHEDULER_PERIODIC_PRIO was greater than or equal to
 *          @ref SCHED_PRIO_LEVELS
 * @return  -EOVERFLOW, if there are too many threads running already in general
 */
kernel_pid_t gnrc_contact_scheduler_periodic_init(void);
int send(char *addr_str, int data, int iface);

#ifdef __cplusplus
}
#endif

#endif
