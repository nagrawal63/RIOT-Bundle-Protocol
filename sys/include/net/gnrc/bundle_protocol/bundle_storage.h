#ifndef _BUNDLE_STORAGE_BP_H
#define _BUNDLE_STORAGE_BP_H

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include "net/gnrc/bundle_protocol/bundle.h"
#include "net/gnrc/bundle_protocol/routing.h"

//Implemented bundle storage as a linkedlist of bundles
struct bundle_list{
  struct actual_bundle current_bundle;
  struct bundle_list* next;
  uint8_t num_of_bundles;
  uint32_t unique_id;
};


#define MAX_BUNDLES 5


struct bundle_list* bundle_storage_init(void);
struct actual_bundle* get_space_for_bundle(void);
bool delete_bundle(struct actual_bundle* bundle);
uint8_t get_next_block_number(void);
struct bundle_list* get_previous_bundle_in_list(struct actual_bundle* bundle);
struct bundle_list* find_bundle_in_list(struct actual_bundle* bundle);
struct actual_bundle *get_bundle_from_list(uint32_t creation_timestamp0, uint32_t creation_timestamp1);
void print_bundle_storage(void);
struct bundle_list *get_bundle_list(void);
struct bundle_list *find_oldest_bundle_to_purge(void);
uint8_t get_current_active_bundles(void);

#endif
