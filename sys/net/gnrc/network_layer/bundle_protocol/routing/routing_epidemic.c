#ifndef _ROUTING_EPIDEMIC
#define _ROUTING_EPIDEMIC
#include "kernel_types.h"
#include "thread.h"
#include "utlist.h"

#include "net/gnrc/bundle_protocol/routing.h"
#include "net/gnrc/bundle_protocol/contact_manager.h"
#include "net/gnrc/bundle_protocol/routing_epidemic.h"

#define ENABLE_DEBUG    (1)
#include "debug.h"

// struct router *this_router;

static struct delivered_bundle_list *head_ptr = NULL;

void routing_epidemic_init(void) {
	DEBUG("routing_epidemic: Initializing epidemic routing.\n");
	this_router = (struct router*)malloc(sizeof(struct router));
	this_router->route_receivers = route_receivers;
	this_router->received_ack = received_ack;
	this_router->notify_bundle_deletion = notify_bundle_deletion;
	this_router->get_delivered_bundle_list = get_delivered_bundle_list;
}

//Implemented assuming endpoint_scheme is IPN
struct neighbor_t *route_receivers(uint32_t dst_num) {
	struct neighbor_t *temp;
	struct neighbor_t *head_of_neighbors = get_neighbor_list();
	
	LL_SEARCH_SCALAR(head_of_neighbors, temp, endpoint_num, dst_num);

	if(!temp) {
		return head_of_neighbors;
	}
	else {
		return temp;
	}
}

void notify_bundle_deletion(struct actual_bundle *bundle) {
	struct delivered_bundle_list *temp = NULL;
	LL_FOREACH(head_ptr, temp) {
		if (is_same_bundle(bundle, temp->bundle)) {
			LL_DELETE(head_ptr, temp);
		}
	}
	DEBUG("routing_epidemic: updated delivered ack list.\n");
	print_delivered_bundle_list();
	return ;
}

void received_ack(struct neighbor_t *src_neighbor, uint32_t creation_timestamp0, uint32_t creation_timestamp1, uint32_t src_num) {
	
	DEBUG("routing_epidemic: Inside processing received acknowledgement.\n");
	struct actual_bundle *bundle = get_bundle_from_list(creation_timestamp0, creation_timestamp1, src_num);
	if (bundle == NULL) {
		DEBUG("bp: could not find bundle in storage corresponding to which ack received.\n");
		return ;
	}

	struct delivered_bundle_list *list_item = malloc(sizeof(struct delivered_bundle_list));
	list_item->bundle = bundle;
	list_item->neighbor = src_neighbor;
	LL_APPEND(head_ptr, list_item);
	print_delivered_bundle_list();
	return;
}

void print_delivered_bundle_list (void) {
	struct delivered_bundle_list *temp;
	DEBUG("routing_epidemic: ");
	LL_FOREACH(head_ptr, temp) {
		DEBUG("(%lu, %lu)->", temp->bundle->local_creation_time, temp->neighbor->endpoint_num);
	}
	DEBUG("NULL.\n");
}

struct delivered_bundle_list *get_delivered_bundle_list(void) {
	return head_ptr;
}
// struct router* get_router(void) {
// 	return this_router;
// }

#endif