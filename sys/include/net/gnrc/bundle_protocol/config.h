
#ifndef NET_GNRC_BP_CONFIG_H
#define NET_GNRC_BP_CONFIG_H

#include "timex.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Default stack size to use for the BP thread.
 */
#ifndef GNRC_BP_STACK_SIZE
#define GNRC_BP_STACK_SIZE           (THREAD_STACKSIZE_DEFAULT)
#endif

/**
 * @brief   Default priority for the BP thread.
 */
#ifndef GNRC_BP_PRIO
#define GNRC_BP_PRIO                 (THREAD_PRIORITY_MAIN - 5)
#endif

/**
 * @brief   Default message queue size to use for the BP thread.
 */
#ifndef GNRC_BP_MSG_QUEUE_SIZE
#define GNRC_BP_MSG_QUEUE_SIZE       (8U)
#endif

#ifdef __cplusplus
}
#endif

#endif /* NET_GNRC_BP_CONFIG_H */
/** @} */
