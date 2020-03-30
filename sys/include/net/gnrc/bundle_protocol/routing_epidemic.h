#ifndef _BUUNDLE_ROUTING_EPIDEMIC_H
#define _BUUNDLE_ROUTING_EPIDEMIC_H

#include <stdlib.h>
#include "net/gnrc/bundle_protocol/routing.h"

struct delivered_bundle_list{
	struct actual_bundle *bundle;
	struct neighbor_t *neighbor;
	struct delivered_bundle_list *next;
};

void routing_epidemic_init(void);
struct neighbor_t *route_receivers(uint32_t dst_num);
void notify_bundle_deletion (struct actual_bundle *bundle);
void received_ack(struct neighbor_t *src_neighbor, uint32_t creation_timestamp0, uint32_t creation_timestamp1);
void print_delivered_bundle_list (void);
// struct router* get_router(void);

#endif
