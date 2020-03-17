#include "random.h"

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
    DEBUG("bundle_storage: Free list is NULL.\n");
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

  DEBUG("bundle_storage: Inside delete bundle.\n");
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
  return true;
}

uint8_t get_next_block_number(void)
{
    return next_block_number++;
}

struct bundle_list* get_previous_bundle_in_list(struct actual_bundle* bundle)
{
    DEBUG("bundle_storage: Inside get previous bundle.\n");
    print_bundle_storage();
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

void print_bundle_storage(void)
{
  DEBUG("bundle_storage: Printing bundle storage list.\n");
  struct bundle_list* temp = head_of_store;
  while(temp!=NULL){
    DEBUG("%ld->", temp->unique_id);
    temp = temp->next;
  }
  DEBUG("NULL.\n");
}


struct bundle_list *get_bundle_list(void) {
  return head_of_store;
}