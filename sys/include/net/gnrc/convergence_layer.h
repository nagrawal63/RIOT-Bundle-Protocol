
#ifndef NET_GNRC_BP_H
#define NET_GNRC_BP_H

#include <stdbool.h>

#include "kernel_types.h"

#include "net/gnrc/bundle_protocol/config.h"
#include "net/gnrc/bundle_protocol/bundle.h"
#include "net/gnrc/bundle_protocol/contact_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RETRANSMIT_TIMER_SECONDS (40000000)

/**
 * @brief   Initialization of the BP thread.
 *
 * @details If BP was already initialized, it will just return the PID of
 *          the BP thread.
 *
 * @return  The PID to the BP thread, on success.
 * @return  -EINVAL, if @ref GNRC_BP_PRIO was greater than or equal to
 *          @ref SCHED_PRIO_LEVELS
 * @return  -EOVERFLOW, if there are too many threads running already in general
 */
kernel_pid_t gnrc_bp_init(void);

int gnrc_bp_dispatch(gnrc_nettype_t type, uint32_t demux_ctx, struct actual_bundle *bundle, uint16_t cmd);

bool check_lifetime_expiry(struct actual_bundle *bundle);

void send_bundles_to_new_neighbor (struct neighbor_t *neighbor);
void send_non_bundle_ack(struct actual_bundle *bundle);
void send_ack(struct actual_bundle *bundle);

int deliver_bundles_to_application(uint32_t service_num);

#ifdef __cplusplus
}
#endif

#endif /* NET_GNRC_SIXLOWPAN_H */
