/**
 * @ingroup     Bundle protocol
 * @{
 *
 * @file
 * @brief       Bundle protocol agent 
 *
 * @author      Nishchay Agrawal <agrawal.nishchay5@gmail.com>
 *
 * @}
 */
#include "utlist.h"

// #include "net/gnrc.h"
#include "net/gnrc/netif.h"
#include "net/gnrc/bundle_protocol/agent.h"
#include "net/gnrc/bundle_protocol/bundle.h"
#include "net/gnrc/bundle_protocol/bundle_storage.h"
#include "net/gnrc/convergence_layer.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

static struct registration_status *application_list = NULL;

struct statistics network_stats;

static int calculate_size_of_num(uint32_t num);

void bundle_protocol_init(kernel_pid_t pid) {
	bundle_storage_init();
	gnrc_netif_t *netif = NULL;

	netif = gnrc_netif_get_by_pid(pid);
	DEBUG("agent: numof interfaces :%d.\n",gnrc_netif_numof());
	if (netif == NULL) {
		DEBUG("agent: Provided network interface is NULL, using hardcoded interface 9.\n");
		iface = 9;
	}
	else if (gnrc_netif_numof() == 1) {
		iface = netif->pid;
	}
	else {
		/*Implement handling with multiple network interfaces*/
		DEBUG("agent: Num of interfaces greater than 1.\n");
		iface = netif->pid;
	}

	network_stats.bundles_delivered = 0;
	network_stats.bundles_received = 0;
	network_stats.bundles_forwarded = 0;
	network_stats.bundles_retransmitted = 0;
	network_stats.bundles_sent = 0;
	network_stats.acks_sent = 0;
	network_stats.acks_received = 0;
	network_stats.discovery_bundle_sent = 0;
	network_stats.discovery_bundle_receive = 0;
}

void send_bundle(uint8_t *payload_data, size_t data_len, char *ipn_dst, char *report_num, uint8_t crctype, uint32_t lifetime) 
{
	// (void) data;
	// (void) iface;
	char *dst, *service_num;

	char temp[IPN_IDENTIFIER_SIZE];
	
	strncpy(temp, ipn_dst, IPN_IDENTIFIER_SIZE);
	if (strstr(temp, "ipn://") != NULL) {
		char *ptr = ipn_dst + IPN_IDENTIFIER_SIZE;
		dst = strtok(ptr, ".");
		service_num = strtok(NULL, ".");
		DEBUG("agent: dst: %s, service_num: %s.\n", dst, service_num);
	}
	else {
		DEBUG("agent: Provide ipn endpoint id.\n");
		return ;
	}

	uint64_t payload_flag;

	if (calculate_canonical_flag(&payload_flag, false) < 0) {
		DEBUG("agent: Error creating payload flag.\n");
		return;
	}

	if (strcmp(dst, get_src_num()) == 0) {
		DEBUG("agent: Bundle destination and source same.\n");
		return ;
	}


	struct actual_bundle *bundle;
	if((bundle = create_bundle()) == NULL){
	DEBUG("agent: Could not create bundle.\n");
	return;
	}

	int res = fill_bundle(bundle, 7, IPN, dst, report_num, lifetime, crctype, service_num);
	if (res < 0) {
		DEBUG("agent: Invalid bundle.\n");
		delete_bundle(bundle);
		return ;
	}
	bundle_add_block(bundle, BUNDLE_BLOCK_TYPE_PAYLOAD, payload_flag, payload_data, crctype, data_len);

	/* Creating bundle age block*/
	size_t bundle_age_len;
	uint8_t *bundle_age_data;
	uint64_t bundle_age_flag;

	if (calculate_canonical_flag(&bundle_age_flag, false) < 0) {
		DEBUG("agent: Error creating payload flag.\n");
		return ;
	}

	uint32_t initial_bundle_age = 0;
	bundle_age_len = calculate_size_of_num(initial_bundle_age);
	bundle_age_data = malloc(bundle_age_len * sizeof(char));
	sprintf((char*)bundle_age_data, "%lu", initial_bundle_age);

	bundle_add_block(bundle, BUNDLE_BLOCK_TYPE_BUNDLE_AGE, bundle_age_flag, bundle_age_data, crctype, bundle_age_len);

	if(!gnrc_bp_dispatch(GNRC_NETTYPE_BP, GNRC_NETREG_DEMUX_CTX_ALL, bundle, GNRC_NETAPI_MSG_TYPE_SND)) {
	    DEBUG("agent: Unable to find BP thread.\n");
	    delete_bundle(bundle);
	    return ;
	}
}

bool register_application(uint32_t service_num, kernel_pid_t pid)
{
	struct registration_status *temp;
	LL_SEARCH_SCALAR(application_list, temp, service_num, service_num);
	if (temp != NULL) {
		DEBUG("agent: Application already running.\n");
		return false;
	}
	struct registration_status *new_application = malloc(sizeof(struct registration_status));
	new_application->service_num = service_num;
	new_application->status = REGISTRATION_ACTIVE;
	new_application->pid = pid;
	LL_APPEND(application_list, new_application);
	deliver_bundles_to_application(new_application);
	return true;
}

bool set_registration_state(uint32_t service_num, uint8_t state)
{
	struct registration_status *temp;
	LL_SEARCH_SCALAR(application_list, temp, service_num, service_num);
	if (temp == NULL) {
		DEBUG("agent: Couldn't find application running.\n");
		return false;
	}
	temp->status = state;
	if (state == REGISTRATION_ACTIVE) {
		deliver_bundles_to_application(temp);
	}
	return true;
}

uint8_t get_registration_status(uint32_t service_num)
{
	struct registration_status *temp;
	LL_SEARCH_SCALAR(application_list, temp, service_num, service_num);
	if (temp == NULL) {
		DEBUG("agent: Couldn't find application running.\n");
		return 0;
	}
	return temp->status;
}

static int calculate_size_of_num(uint32_t num) {
	if(num == 0) {
		return 0;
	}
	int a = ((ceil(log10(num))+1)*sizeof(char)); 
	return a;
}

struct registration_status *get_registration (uint32_t service_num)
{
	struct registration_status *temp;
	LL_SEARCH_SCALAR(application_list, temp, service_num, service_num);
	if (temp == NULL) {
		DEBUG("agent: Couldn't find application running.\n");
		return 0;
	}
	return temp;
}

bool unregister_application(uint32_t service_num) 
{
	struct registration_status *temp;
	LL_SEARCH_SCALAR(application_list, temp, service_num, service_num);
	if (temp == NULL) {
		DEBUG("agent: Couldn't find application running.\n");
		return false;
	}
	else {
		LL_DELETE(application_list, temp);
		return true;
	}
}

void update_statistics(int type)
{
	switch(type) {
		case BUNDLE_DELIVERY:
			network_stats.bundles_delivered++;
			break;
		case BUNDLE_RECEIVE:
			network_stats.bundles_received++;
			break;
		case BUNDLE_SEND:
			network_stats.bundles_sent++;
			break;
		case BUNDLE_FORWARD:
			network_stats.bundles_forwarded++;
			break;
		case ACK_SEND:
			network_stats.acks_sent++;
			break;
		case ACK_RECEIVE:
			network_stats.acks_received++;
			break;
		case DISCOVERY_BUNDLE_RECEIVE:
			network_stats.discovery_bundle_receive++;
			break;
		case DISCOVERY_BUNDLE_SEND:
			network_stats.discovery_bundle_sent++;
			break;
		case BUNDLE_RETRANSMIT:
			network_stats.bundles_retransmitted++;
			break;
	}
}

void print_network_statistics(void)
{
	// printf("Network stats:\n");
	// printf("#*#*#Total bundles sent : %d.\n", network_stats.bundles_sent);
	// printf("#*#*#Total bundles recieved : %d.\n", network_stats.bundles_received);
	// printf("#*#*#Total bundles forwarded : %d.\n", network_stats.bundles_forwarded);
	// printf("#*#*#Total bundles delivered : %d.\n", network_stats.bundles_delivered);
	// printf("#*#*#Total bundles transmitted: %d.\n", network_stats.bundles_retransmitted);
	// printf("#*#*#Total acks sent : %d.\n", network_stats.acks_sent);
	// printf("#*#*#Total acks received : %d.\n", network_stats.acks_received);
	// printf("#*#*#Total discovery bundles sent : %d.\n", network_stats.discovery_bundle_sent);
	// printf("#*#*#Total discovery bundles received : %d.\n", network_stats.discovery_bundle_receive);
	// printf("#*#*#Bundles in storage: %d.\n", get_current_active_bundles());
	// printf("#*#*#Current system time: %lu.\n", xtimer_now().ticks32);

	printf("#*#*,%lu, %d, %d, %d, %d, %d, %d, %d, %d,%d,%d,\n",xtimer_now().ticks32, get_current_active_bundles(), network_stats.bundles_sent, 
						network_stats.bundles_received, network_stats.bundles_forwarded
					, network_stats.bundles_retransmitted, network_stats.bundles_delivered, network_stats.acks_sent, network_stats.acks_received
					, network_stats.discovery_bundle_sent, network_stats.discovery_bundle_receive);
}
