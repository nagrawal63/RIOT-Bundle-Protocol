#include "utlist.h"

#include "net/gnrc.h"
#include "net/gnrc/netif.h"
#include "net/gnrc/bundle_protocol/agent.h"
#include "net/gnrc/bundle_protocol/bundle.h"
#include "net/gnrc/bundle_protocol/bundle_storage.h"
#include "net/gnrc/convergence_layer.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

static struct registration_status *application_list = NULL;
// static gnrc_netreg_entry_t server = GNRC_NETREG_ENTRY_INIT_PID(GNRC_NETREG_DEMUX_CTX_ALL, KERNEL_PID_UNDEF);

static int calculate_size_of_num(uint32_t num);

void send_bundle(uint8_t *payload_data, size_t data_len, char *dst, char *service_num, int iface, char *report_num, uint8_t crctype, uint32_t lifetime) 
{
	// (void) data;
	(void) iface;

	uint64_t payload_flag;

	if (calculate_canonical_flag(&payload_flag, false) < 0) {
	printf("Error creating payload flag.\n");
	return;
	}

	struct actual_bundle *bundle;
	if((bundle = create_bundle()) == NULL){
	printf("Could not create bundle.\n");
	return;
	}

	fill_bundle(bundle, 7, IPN, dst, report_num, lifetime, crctype, service_num);
	bundle_add_block(bundle, BUNDLE_BLOCK_TYPE_PAYLOAD, payload_flag, payload_data, crctype, data_len);

	/* Creating bundle age block*/
	size_t bundle_age_len;
	uint8_t *bundle_age_data;
	uint64_t bundle_age_flag;

	if (calculate_canonical_flag(&bundle_age_flag, false) < 0) {
	printf("Error creating payload flag.\n");
	return ;
	}

	uint32_t initial_bundle_age = 0;
	bundle_age_len = calculate_size_of_num(initial_bundle_age);
	bundle_age_data = malloc(bundle_age_len * sizeof(char));
	sprintf((char*)bundle_age_data, "%lu", initial_bundle_age);

	bundle_add_block(bundle, BUNDLE_BLOCK_TYPE_BUNDLE_AGE, bundle_age_flag, bundle_age_data, crctype, bundle_age_len);

	if(!gnrc_bp_dispatch(GNRC_NETTYPE_BP, GNRC_NETREG_DEMUX_CTX_ALL, bundle, GNRC_NETAPI_MSG_TYPE_SND)) {
	    printf("Unable to find BP thread.\n");
	    delete_bundle(bundle);
	    return ;
	  }
}

bool register_application(uint32_t service_num)
{
    // /* check if server is already running */
    // if (server.target.pid != KERNEL_PID_UNDEF) {
    //     printf("agent: application already running with service number %" PRIu32 "\n",
    //            server.demux_ctx);
    //     return;
    // }
    // /* parse port */
    // // port = atoi(port_str);
    // if (service_num == 0) {
    //     DEBUG("agent: invalid service_num specified");
    //     return;
    // }
    // /* start server (which means registering pktdump for the chosen port) */
    // server.target.pid = gnrc_pktdump_pid;
    // server.demux_ctx = (uint32_t)port;
    // gnrc_netreg_register(GNRC_NETTYPE_UDP, &server);
    // printf("agent: started UDP server on port %" PRIu16 "\n", port);

    struct registration_status *new_application = malloc(sizeof(struct registration_status));
    new_application->service_num = service_num;
    new_application->status = REGISTRATION_ACTIVE;
    LL_APPEND(application_list, new_application);
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
		deliver_bundles_to_application(service_num);
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
  // DEBUG("bp:size = %d.\n",a );
  return a;
}