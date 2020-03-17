
#ifndef NET_CONTACT_MANAGER_BP_CONFIG_H
#define NET_CONTACT_MANAGER_BP_CONFIG_H

#include "timex.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Default stack size to use for the _CONTACT_MANAGER_BP thread.
 */
#ifndef GNRC_CONTACT_MANAGER_STACK_SIZE
#define GNRC_CONTACT_MANAGER_STACK_SIZE           (THREAD_STACKSIZE_DEFAULT)
#endif

/**
 * @brief   Default priority for the _CONTACT_MANAGER_BP thread.
 */
#ifndef GNRC_CONTACT_MANAGER_PRIO
#define GNRC_CONTACT_MANAGER_PRIO                 (THREAD_PRIORITY_MAIN - 3)
#endif

/**
 * @brief   Default message queue size to use for the _CONTACT_MANAGER_BP thread.
 */
#ifndef GNRC_CONTACT_MANAGER_MSG_QUEUE_SIZE
#define GNRC_CONTACT_MANAGER_MSG_QUEUE_SIZE       (8U)
#endif

#ifdef __cplusplus
}
#endif

#endif /* NET_CONTACT_MANAGER_BP_CONFIG_H */
/** @} */
