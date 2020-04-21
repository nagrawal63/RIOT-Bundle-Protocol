#ifndef _BUNDLE_PROTOCOL_AGENT
#define _BUNDLE_PROTOCOL_AGENT

#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#include "thread.h"
#include "net/gnrc.h"
#include "net/gnrc/netif.h"

#define REGISTRATION_ACTIVE 0x01
#define REGISTRATION_PASSIVE 0x02
#define NOT_REGISTERED 0x00

struct registration_status {
	uint32_t service_num;
	uint8_t status;
	kernel_pid_t pid;
	struct registration_status *next;
};

void bundle_protocol_init(kernel_pid_t pid);
void send_bundle(uint8_t *payload_data, size_t data_len, char *dst, char *service_num, int iface, char *report_num, uint8_t crctype, uint32_t lifetime);
bool register_application(uint32_t service_num, kernel_pid_t pid);
bool set_registration_state(uint32_t service_num, uint8_t state);
uint8_t get_registration_status(uint32_t service_num);
struct registration_status *get_registration (uint32_t service_num);

#endif