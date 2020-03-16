#ifndef _BUUNDLE_ROUTING_EPIDEMIC_H
#define _BUUNDLE_ROUTING_EPIDEMIC_H

#include <stdlib.h>
#include "net/gnrc/bundle_protocol/routing.h"

void routing_epidemic_init(void);
struct neighbor_t *route_receivers(uint32_t dst_num);
struct router* get_router(void);

#endif
