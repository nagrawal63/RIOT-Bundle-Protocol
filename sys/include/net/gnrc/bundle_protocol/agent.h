/**
 * @ingroup     Bundle protocol
 * @{
 *
 * @file
 * @brief       Bundle protocol agent header
 *
 * @author      Nishchay Agrawal <agrawal.nishchay5@gmail.com>
 *
 * @}
 */
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

#define BUNDLE_DELIVERY 0x01
#define BUNDLE_RECEIVE 0x02
#define BUNDLE_SEND 0x03
#define BUNDLE_FORWARD 0x04
#define BUNDLE_RETRANSMIT 0x05
#define ACK_SEND 0x06
#define ACK_RECEIVE 0x07
#define DISCOVERY_BUNDLE_RECEIVE 0x08
#define DISCOVERY_BUNDLE_SEND 0x09

struct registration_status {
	uint32_t service_num;
	uint8_t status;
	kernel_pid_t pid;
	struct registration_status *next;
};

struct statistics {
	int bundles_delivered;
	int bundles_received;
	int bundles_forwarded;
	int bundles_retransmitted;
	int bundles_sent;
	int acks_sent;
	int acks_received;
	int discovery_bundle_sent;
	int discovery_bundle_receive;
};

void bundle_protocol_init(kernel_pid_t pid);
void send_bundle(uint8_t *payload_data, size_t data_len, char *ipn_dst, char *report_num, uint8_t crctype, uint32_t lifetime);
bool register_application(uint32_t service_num, kernel_pid_t pid);
bool set_registration_state(uint32_t service_num, uint8_t state);
uint8_t get_registration_status(uint32_t service_num);
struct registration_status *get_registration (uint32_t service_num);
bool unregister_application(uint32_t service_num);

void update_statistics(int type);
void print_network_statistics(void);


#endif