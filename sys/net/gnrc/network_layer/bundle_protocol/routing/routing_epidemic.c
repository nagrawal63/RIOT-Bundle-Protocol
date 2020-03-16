#include "kernel_types.h"
#include "thread.h"
#include "utlist.h"

// #include "net/gnrc/bp.h"
#include "net/gnrc/bundle_protocol/routing.h"
#include "net/gnrc/bundle_protocol/contact_manager.h"
#include "net/gnrc/bundle_protocol/routing_epidemic.h"

#define ENABLE_DEBUG    (1)
#include "debug.h"

struct router *this_router;

void routing_epidemic_init(void) {
	DEBUG("routing_epidemic: Initializing epidemic routing.\n");
	this_router = (struct router*)malloc(sizeof(struct router));
	this_router->route_receivers = route_receivers;
}

//Implemented assuming endpoint_scheme is IPN
struct neighbor_t *route_receivers(uint32_t dst_num) {
	struct neighbor_t *temp;
	struct neighbor_t *head_of_neighbors = get_neighbor_list();
	// LL_SEARCH(head_of_neighbors, temp, , comparator);
	LL_SEARCH_SCALAR(head_of_neighbors, temp, endpoint_num, dst_num);

	if(!temp) {
		return head_of_neighbors;
	}
	else {
		return temp;
	}
}

struct router* get_router(void) {
	return this_router;
}