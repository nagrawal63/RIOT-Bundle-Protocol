#ifndef _CONTACT_MANAGER_BP_H
#define _CONTACT_MANAGER_BP_H

#include <stdbool.h>

#include "net/gnrc/bundle_protocol/contact_manager_config.h"
#include "net/gnrc/ipv6/nib/conf.h"

#ifdef __cplusplus
extern "C" {
#endif

//Don't implement linked list stuff for this. Rather directly use
//inbuilt LL stuff of RIOT
struct neighbor_t{
  char* eid; // Neighbor's bp addr
  uint8_t l2addr [GNRC_IPV6_NIB_L2ADDR_MAX_LEN];
  uint8_t 	l2addr_len;
  struct neighbor_t *next;
};

/**
 * @brief   Initialization of the CONTACT_MANAGER thread.
 *
 * @details If CONTACT_MANAGER was already initialized, it will just return the PID of
 *          the CONTACT_MANAGER thread.
 *
 * @return  The PID to the CONTACT_MANAGER thread, on success.
 * @return  -EINVAL, if @ref GNRC_CONTACT_MANAGER_PRIO was greater than or equal to
 *          @ref SCHED_PRIO_LEVELS
 * @return  -EOVERFLOW, if there are too many threads running already in general
 */
kernel_pid_t gnrc_contact_manager_init(void);

#ifdef __cplusplus
}
#endif

#endif
