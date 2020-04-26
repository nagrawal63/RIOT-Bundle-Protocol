#include "random.h"
#include "utlist.h"

#include "net/gnrc/bundle_protocol/bundle_storage.h"
#include "net/gnrc/bundle_protocol/bundle.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

static uint8_t next_block_number = 0;
struct bundle_list* free_list;
struct bundle_list* head_of_store;
static uint8_t active_bundles = 0;

struct processed_bundle_list *head_processed_list_ptr;
static uint8_t num_processed_list = 0;

static void delete_oldest(void);

struct bundle_list* bundle_storage_init(void)
{
  free_list = malloc(MAX_BUNDLES * sizeof(struct bundle_list));
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

struct actual_bundle* get_space_for_bundle(void)
{
  struct bundle_list *temp = head_of_store;
  int i = 0;
  while(temp != NULL && i < active_bundles) {
    if (is_expired_bundle(&temp->current_bundle)) {
      set_retention_constraint(NO_RETENTION_CONSTRAINT);
      delete_bundle(&temp->current_bundle);
    }
    temp = temp->next;
    i++;
  }
  struct bundle_list *ret = NULL;
  if(free_list == NULL){
    DEBUG("bundle_storage: Bundle storage is full, deleting oldest bundle.\n");
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
  active_bundles++;
  set_retention_constraint(&ret->current_bundle, NO_RETENTION_CONSTRAINT);
  return &ret->current_bundle;
}

// Implement the case when the node is not found since NULL is returned when both the
// first node is to be deleted and when the node is not found
bool delete_bundle(struct actual_bundle* bundle)
{
  if (bundle == NULL) {
    DEBUG("bundle_storage: Bundle to be deleted is NULL.\n");
    return false;
  }
  DEBUG("bundle_storage: Deleting bundle created at %lu.\n", bundle->local_creation_time);
  if (get_retention_constraint(bundle) != NO_RETENTION_CONSTRAINT) {
    DEBUG("bundle_storage: Cannot delete bundle since bundle's retention constraint is %u.\n", get_retention_constraint(bundle));
    return false;
  }
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
  get_router()->notify_bundle_deletion(bundle);
  active_bundles--;
  DEBUG("bundle_storage: Printing bundle storage after deleting.\n");
  print_bundle_storage();
  return true;
}

uint8_t get_next_block_number(void)
{
    return next_block_number++;
}

struct bundle_list* get_previous_bundle_in_list(struct actual_bundle* bundle)
{
    struct bundle_list* temp = head_of_store;
    if(is_same_bundle(&(temp->current_bundle), bundle)){
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

struct actual_bundle *get_bundle_from_list(uint32_t creation_timestamp0, uint32_t creation_timestamp1, uint32_t src_num) 
{
  struct bundle_list *temp = NULL;
  bool found = false;
  LL_FOREACH(head_of_store, temp) {
    if (temp->current_bundle.primary_block.creation_timestamp[0] == creation_timestamp0 
        && temp->current_bundle.primary_block.creation_timestamp[1] == creation_timestamp1
        && temp->current_bundle.primary_block.src_num == src_num) {
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


struct bundle_list *get_bundle_list(void) 
{
  return head_of_store;
}

struct bundle_list *find_oldest_bundle_to_purge(void) 
{
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

uint8_t get_current_active_bundles(void) 
{
  return active_bundles;
}

bool is_redundant_bundle(struct actual_bundle *bundle) 
{
  struct bundle_list *temp;
  int i = 0;
  LL_FOREACH(head_of_store, temp) {
    if (is_same_bundle(bundle, &temp->current_bundle)) {
      i++;
    }
  }
  if (i > 1) {
    return true;
  }
  return false;
}

int add_bundle_to_processed_bundle_list(struct actual_bundle *bundle)
{
  if (num_processed_list == MAX_PROCESSED_BUNDLES) {
    delete_oldest();
  }
  struct processed_bundle_list *new_processed = malloc(sizeof(struct processed_bundle_list));
  new_processed->src_num = bundle->primary_block.src_num;
  new_processed->creation_timestamp[0] = bundle->primary_block.creation_timestamp[0];
  new_processed->creation_timestamp[1] = bundle->primary_block.creation_timestamp[1];
  new_processed->fragment_offset = bundle->primary_block.fragment_offset;
  new_processed->total_application_data_length = bundle->primary_block.total_application_data_length;
  LL_APPEND(head_processed_list_ptr, new_processed);
  num_processed_list++;
  return OK;
}

bool verify_bundle_processed(struct actual_bundle *bundle)
{
  struct processed_bundle_list *temp;
  LL_FOREACH(head_processed_list_ptr, temp) {
    if(bundle->primary_block.src_num == temp->src_num) {
      if (bundle->primary_block.creation_timestamp[0] == temp->creation_timestamp[0]
          && bundle->primary_block.creation_timestamp[1] == temp->creation_timestamp[1]) {
        if (bundle->primary_block.fragment_offset == temp->fragment_offset 
          && bundle->primary_block.total_application_data_length == temp->total_application_data_length) {
          return true;
        }
      }
    }
  }
  return false;
}

/* Can simply delete the head of the list since it contains the oldest added element,
  therefore, no need to keep track of time and thus saving space*/
static void delete_oldest(void) 
{
  LL_DELETE(head_processed_list_ptr, head_processed_list_ptr);
  num_processed_list--;
}