#ifndef _ROUTING_BP_H
#define _ROUTING_BP_H

#include <stdint.h>

#include "net/gnrc/bundle_protocol/contact_manager.h"
#include "net/gnrc/bundle_protocol/bundle.h"
#include "net/gnrc/bundle_protocol/bundle_storage.h"

struct router{
//	int result;
	struct neighbor_t* (*route_receivers) (uint32_t dst_num);
	void (*received_ack) (struct neighbor_t *src_neighbor, uint32_t creation_timestamp0, uint32_t creation_timestamp1);
	void (*notify_bundle_deletion) (struct actual_bundle *bundle);
	struct delivered_bundle_list* (*get_delivered_bundle_list) (void);
};

struct delivered_bundle_list{
	struct actual_bundle *bundle;
	struct neighbor_t *neighbor;
	struct delivered_bundle_list *next;
};


extern struct router *this_router;

static inline struct router *get_router(void) {
	return this_router;
}

#endif
