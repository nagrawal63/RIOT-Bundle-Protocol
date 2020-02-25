#include "net/gnrc/bundle_protocol/bundle_storage.h"
#include "net/gnrc/bundle_protocol/bundle.h"

static uint8_t next_block_number = 0;
struct bundle_list* free_list;
struct bundle_list* head_of_store;

struct bundle_list* bundle_storage_init(void)
{
  free_list = malloc(MAX_BUNDLES * sizeof(struct bundle_list));
  // bundle_store->next = NULL;
  free_list->num_of_bundles=0;
  // free_list->next_block_number = 0;
  // bundle_store->next_empty_index = 1;
  for(int i=0;i<MAX_BUNDLES-1;i++){
    free_list[i].next = &free_list[i+1];
  }
  free_list[MAX_BUNDLES-1].next = NULL;
  head_of_store = free_list;
  next_block_number = 0;

  return free_list;
}
//TODO: Test this implementation
struct actual_bundle* get_space_for_bundle(void)
{
  struct bundle_list *ret;
  if(free_list == NULL){
    return NULL;
  }
  ret = free_list;
  free_list = free_list->next;
  return &ret->current_bundle;
}

bool delete_bundle(struct actual_bundle* bundle)
{

  struct bundle_list* to_delete_node = find_bundle_in_list(bundle);
  if(to_delete_node == NULL){
    return false;
  }
  to_delete_node->next = free_list;
  free_list = to_delete_node;
  return true;
}

uint8_t get_next_block_number(void)
{
    return next_block_number++;
}

struct bundle_list* find_bundle_in_list ( struct actual_bundle* bundle)
{
    struct bundle_list* temp = head_of_store;
    while(temp != NULL){
      if(is_same_bundle(&temp->current_bundle, bundle)){
        break;
      }
      temp = temp->next;
    }
    return temp;
}
