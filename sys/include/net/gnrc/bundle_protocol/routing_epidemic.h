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
void received_ack(struct actual_bundle *bundle, uint32_t source);
// struct router* get_router(void);

#endif
