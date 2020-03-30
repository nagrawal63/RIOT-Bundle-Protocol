#include "random.h"
#include "utlist.h"

#include "net/gnrc/bundle_protocol/bundle_storage.h"
#include "net/gnrc/bundle_protocol/bundle.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

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
    free_list[i].unique_id = 0;
  }
  free_list[MAX_BUNDLES-1].next = NULL;
  free_list[MAX_BUNDLES-1].unique_id = 0;
  head_of_store = free_list;
  next_block_number = 0;

  random_init(RANDOM_SEED_DEFAULT);

  return free_list;
}
//TODO: Test this implementation
struct actual_bundle* get_space_for_bundle(void)
{
  struct bundle_list *ret = NULL;
  if(free_list == NULL){
    DEBUG("bundle_storage: Bundle storage is full, deleting oldest bundle.\n");
    print_bundle_storage();
    struct bundle_list *oldest_bundle = find_oldest_bundle_to_purge();
    if(delete_bundle(&oldest_bundle->current_bundle)) {
      DEBUG("bundle_storage: deleted oldest bundle .\n");
      get_router()->notify_bundle_deletion(&oldest_bundle->current_bundle);
      return get_space_for_bundle();
    }
    return NULL;
  }
  ret = free_list;
  ret->unique_id = 	random_uint32();
  free_list = free_list->next;

  if(head_of_store != ret) {
    DEBUG("bundle_storage: head_of_store != ret.\n");
    ret->next = head_of_store;
    head_of_store = ret;
  } else {
    // returning the bundle at head_of_store
    ret->next = NULL;
  }
  return &ret->current_bundle;
}

// Implement the case when the node is not found since NULL is returned when both the
// first node is to be deleted and when the node is not found
bool delete_bundle(struct actual_bundle* bundle)
{

  DEBUG("bundle_storage: Deleting bundle created at %lu.\n", bundle->local_creation_time);
  struct bundle_list* previous_of_to_delete_node = get_previous_bundle_in_list(bundle);
  struct bundle_list* to_delete_node = NULL;

  if(previous_of_to_delete_node != NULL) {
    to_delete_node = previous_of_to_delete_node->next;
    previous_of_to_delete_node->next = to_delete_node->next;
  } else {
    //When trying to delete the head_of_store
    to_delete_node = head_of_store;
    head_of_store = to_delete_node -> next;
  }

  to_delete_node->next = free_list;
  free_list = to_delete_node;

  if(head_of_store == NULL){
    head_of_store = free_list;
  }
  DEBUG("bundle_storage: Printing bundle storage after deleting.\n");
  // memset(&to_delete_node->unique_id, 0, sizeof(uint32_t));
  print_bundle_storage();
  return true;
}

uint8_t get_next_block_number(void)
{
    return next_block_number++;
}

struct bundle_list* get_previous_bundle_in_list(struct actual_bundle* bundle)
{
    // DEBUG("bundle_storage: Inside get previous bundle.\n");
    // print_bundle_storage();
    struct bundle_list* temp = head_of_store;
    if(is_same_bundle(&(temp->current_bundle), bundle)){
      DEBUG("bundle_storage: To delete first bundle.\n");
      return NULL;
    }
    while(temp->next != NULL){
      if (is_same_bundle(&temp->next->current_bundle, bundle)){
        break;
      }
      temp = temp->next;
    }
    return temp;
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

struct actual_bundle *get_bundle_from_list(uint32_t creation_timestamp0, uint32_t creation_timestamp1) 
{
  struct bundle_list *temp = NULL;
  bool found = false;
  LL_FOREACH(head_of_store, temp) {
    if (temp->current_bundle.primary_block.creation_timestamp[0] == creation_timestamp0 
        && temp->current_bundle.primary_block.creation_timestamp[1] == creation_timestamp1) {
      found = true;
      break;
    }
  }
  if (found) {
    return &temp->current_bundle;
  }
  else {
    return NULL;
  }
}

void print_bundle_storage(void)
{
  DEBUG("bundle_storage: Printing bundle storage list.\n");
  struct bundle_list* temp = head_of_store;
  while(temp!=NULL){
    DEBUG("(%ld, %lu)->", temp->unique_id, temp->current_bundle.local_creation_time);
    temp = temp->next;
  }
  DEBUG("NULL.\n");
}


struct bundle_list *get_bundle_list(void) {
  return head_of_store;
}

struct bundle_list *find_oldest_bundle_to_purge(void) {
  struct bundle_list *temp, *oldest_bundle = NULL;
  uint32_t oldest_time = UINT32_MAX;
  LL_FOREACH(head_of_store, temp) {
    if (temp->current_bundle.local_creation_time < oldest_time) {
      oldest_time = temp->current_bundle.local_creation_time;
      oldest_bundle = temp;
    }
  }
  DEBUG("bundle_storage: Will delete bundle with creation time:%lu.\n", oldest_bundle->current_bundle.local_creation_time);
  return oldest_bundle;
}