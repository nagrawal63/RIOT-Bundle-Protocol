
#include "net/gnrc/bundle_protocol/bundle.h"
#include "net/gnrc/bundle_protocol/bundle_storage.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

// Assuming the endpoint_scheme is the same for the src, dest and report nodes
static bool is_fragment_bundle(struct actual_bundle* bundle);
static void decode_primary_block_element(nanocbor_value_t *decoder, struct actual_bundle* bundle, uint8_t element);
static void decode_canonical_block_element(nanocbor_value_t* decoder, struct bundle_canonical_block_t* block, uint8_t element);
// static void insert_block_in_bundle(struct actual_bundle* bundle, struct bundle_canonical_block_t* block);

void insert_block_in_bundle(struct actual_bundle* bundle, struct bundle_canonical_block_t* block)
{
  struct bundle_canonical_block_t* temp = bundle->other_blocks;
  if(temp == NULL){
    bundle->other_blocks= block;
    return ;
  }
  while(temp->next!=NULL){
      temp = temp->next;
  }
  temp->next = block;
  block->next = NULL;
  return ;
}

bool is_same_bundle(struct actual_bundle* current_bundle, struct actual_bundle* compare_to_bundle)
{

  if (strcmp((char*)current_bundle->primary_block.src_eid,(char*)compare_to_bundle->primary_block.src_eid) != 0){
    return false;
  }
  if(!(current_bundle->primary_block.creation_timestamp[0] == compare_to_bundle->primary_block.creation_timestamp[0] && current_bundle->primary_block.creation_timestamp[1] == compare_to_bundle->primary_block.creation_timestamp[1])){
    return false;
  }
  if(!(current_bundle->primary_block.fragment_offset == compare_to_bundle->primary_block.fragment_offset)){
    return false;
  }
  if(!(current_bundle->primary_block.total_application_data_length == compare_to_bundle->primary_block.total_application_data_length)){
    return false;
  }
  return true;
}

static bool is_fragment_bundle(struct actual_bundle* bundle)
{
  return ((bundle->primary_block.flags & FRAGMENT_IDENTIFICATION_MASK) == 1);
}

int bundle_encode(struct actual_bundle* bundle, nanocbor_encoder_t *enc)
{
    bool isFragment= is_fragment_bundle(bundle);

    //actual encoding of bundle
    nanocbor_fmt_array_indefinite(enc);

    //parsing and encoding primary block
    if(isFragment){ // Bundle is fragment
      DEBUG("bundle: Encoding fragment bundle.\n");
      nanocbor_fmt_array(enc, 11);
      nanocbor_fmt_uint(enc, bundle->primary_block.version);
      nanocbor_fmt_uint(enc, bundle->primary_block.flags);
      nanocbor_fmt_uint(enc, bundle->primary_block.crc_type);

      //encoding destination endpoint
      nanocbor_fmt_array(enc,2);
      nanocbor_fmt_uint(enc,bundle->primary_block.endpoint_scheme);
      if(bundle->primary_block.endpoint_scheme == DTN){
        if(bundle->primary_block.dest_eid == NULL){
          nanocbor_fmt_uint(enc,0);
        }
        else{
          nanocbor_put_tstr(enc,(char*)bundle->primary_block.dest_eid);
        }
      }
      // TODO: Talk about the thing written in doubt in notes and then implement this
      else if (bundle->primary_block.endpoint_scheme == IPN){

      }

      // encoding source endpoint
      nanocbor_fmt_array(enc,2);
      nanocbor_fmt_uint(enc,bundle->primary_block.endpoint_scheme);
      if(bundle->primary_block.endpoint_scheme == DTN){
        if(bundle->primary_block.src_eid == NULL){
          nanocbor_fmt_uint(enc,0);
        }
        else{
          nanocbor_put_tstr(enc,(char*)bundle->primary_block.src_eid);
        }
      }
      // TODO: Talk about the thing written in doubt in notes and then implement this
      else if (bundle->primary_block.endpoint_scheme == IPN){

      }

      // encoding report to endpoint
      nanocbor_fmt_array(enc,2);
      nanocbor_fmt_uint(enc,bundle->primary_block.endpoint_scheme);
      if(bundle->primary_block.endpoint_scheme == DTN){
        if(bundle->primary_block.report_eid == NULL){
          nanocbor_fmt_uint(enc,0);
        }
        else{
          nanocbor_put_tstr(enc,(char*)bundle->primary_block.report_eid);
        }
      }
      // TODO: Talk about the thing written in doubt in notes and then implement this
      else if (bundle->primary_block.endpoint_scheme == IPN){

      }

      //encoding the creation timestamp
      nanocbor_fmt_array(enc,2);
      nanocbor_fmt_uint(enc,bundle->primary_block.creation_timestamp[0]);
      nanocbor_fmt_uint(enc,bundle->primary_block.creation_timestamp[1]);

      nanocbor_fmt_uint(enc, bundle->primary_block.lifetime);

      nanocbor_fmt_uint(enc, bundle->primary_block.fragment_offset);
      nanocbor_fmt_uint(enc, bundle->primary_block.total_application_data_length);

      nanocbor_fmt_uint(enc,bundle->primary_block.crc);
    }
    else{ // Bundle is not fragment
      DEBUG("bundle: Encoding non fragment bundle.\n");
      nanocbor_fmt_array(enc, 9);
      nanocbor_fmt_uint(enc, bundle->primary_block.version);
      nanocbor_fmt_uint(enc, bundle->primary_block.flags);
      nanocbor_fmt_uint(enc, bundle->primary_block.crc_type);
      //
      // //encoding destination endpoint
      nanocbor_fmt_array(enc,2);
      nanocbor_fmt_uint(enc,bundle->primary_block.endpoint_scheme);
      if(bundle->primary_block.endpoint_scheme == DTN){
        DEBUG("bundle: endpoint scheme is DTN.\n");
        if(bundle->primary_block.dest_eid == NULL){
          nanocbor_fmt_uint(enc,0);
        }
        else{
          nanocbor_put_tstr(enc,(char*)bundle->primary_block.dest_eid);
        }
      }
      // TODO: Talk about the thing written in doubt in notes and then implement this
      else if (bundle->primary_block.endpoint_scheme == IPN){
        DEBUG("bundle: endpoint scheme is IPN.\n");
      }

      // encoding source endpoint
      nanocbor_fmt_array(enc,2);
      nanocbor_fmt_uint(enc,bundle->primary_block.endpoint_scheme);
      if(bundle->primary_block.endpoint_scheme == DTN){
        if(bundle->primary_block.src_eid == NULL){
          nanocbor_fmt_uint(enc,0);
        }
        else{
          nanocbor_put_tstr(enc,(char*)bundle->primary_block.src_eid);
        }
      }
      // TODO: Talk about the thing written in doubt in notes and then implement this
      else if (bundle->primary_block.endpoint_scheme == IPN){

      }

      // encoding report to endpoint
      nanocbor_fmt_array(enc,2);
      nanocbor_fmt_uint(enc,bundle->primary_block.endpoint_scheme);
      if(bundle->primary_block.endpoint_scheme == DTN){
        if(bundle->primary_block.report_eid == NULL){
          nanocbor_fmt_uint(enc,0);
        }
        else{
          nanocbor_put_tstr(enc,(char*)bundle->primary_block.report_eid);
        }
      }
      // TODO: Talk about the thing written in doubt in notes and then implement this
      else if (bundle->primary_block.endpoint_scheme == IPN){

      }

      //encoding the creation timestamp
      nanocbor_fmt_array(enc,2);
      nanocbor_fmt_uint(enc,bundle->primary_block.creation_timestamp[0]);
      nanocbor_fmt_uint(enc,bundle->primary_block.creation_timestamp[1]);

      nanocbor_fmt_uint(enc, bundle->primary_block.lifetime);

      nanocbor_fmt_uint(enc,bundle->primary_block.crc);
    }

    //encoding canonical blocks
    struct bundle_canonical_block_t* tempPtr = bundle->other_blocks;
    while(tempPtr != NULL){
      if(tempPtr->crc_type != NOCRC){
        nanocbor_fmt_array(enc,6);
        nanocbor_fmt_int(enc,tempPtr->type);
        nanocbor_fmt_int(enc,tempPtr->block_number);
        nanocbor_fmt_int(enc,tempPtr->flags);
        nanocbor_fmt_int(enc,tempPtr->crc_type);
        //TODO: encoded as per the block specification (which is conflicting
        // with the one mentioned in the block specification)
        nanocbor_put_bstr(enc,tempPtr->block_data, sizeof(tempPtr->block_data));
        nanocbor_fmt_int(enc, tempPtr->crc);
      }
      else{
        nanocbor_fmt_array(enc,5);
        nanocbor_fmt_int(enc,tempPtr->type);
        nanocbor_fmt_int(enc,tempPtr->block_number);
        nanocbor_fmt_int(enc,tempPtr->flags);
        nanocbor_fmt_int(enc,tempPtr->crc_type);
        //TODO: encoded as per the block specification (which is conflicting
        // with the one mentioned in the block specification)
        nanocbor_put_bstr(enc,tempPtr->block_data, sizeof(tempPtr->block_data));
      }
      tempPtr = tempPtr->next;
    }
    nanocbor_fmt_end_indefinite(enc);
    return 1;
}

static void decode_primary_block_element(nanocbor_value_t *decoder, struct actual_bundle* bundle, uint8_t element)
{
  switch(element){
    case VERSION:
      {
        uint32_t temp;
        nanocbor_get_uint32(decoder, &temp);
        bundle->primary_block.version = temp;
      }
      break;
    case FLAGS_PRIMARY:
      {
        uint32_t temp;
        nanocbor_get_uint32(decoder, &temp);
         bundle->primary_block.flags = temp;
      }
      break;
    case CRC_TYPE_PRIMARY:
    {
      uint32_t temp;
      nanocbor_get_uint32(decoder, &temp);
      bundle->primary_block.crc_type = temp;
    }
    break;
    case EID:
    {
      size_t len;
      uint32_t endpt_scheme;
      nanocbor_value_t arr;
      nanocbor_enter_array(decoder, &arr);
      nanocbor_get_uint32(decoder, &endpt_scheme);
      bundle->primary_block.endpoint_scheme = endpt_scheme;
      nanocbor_get_tstr(decoder, (const uint8_t**)&bundle->primary_block.dest_eid,&len);
      nanocbor_leave_container(decoder, &arr);

      // nanocbor_value_t arr;
      nanocbor_enter_array(decoder, &arr);
      nanocbor_get_uint32(decoder, &endpt_scheme);
      bundle->primary_block.endpoint_scheme = endpt_scheme;
      nanocbor_get_tstr(decoder, (const uint8_t**)&bundle->primary_block.src_eid,&len);
      nanocbor_leave_container(decoder, &arr);

      // nanocbor_value_t arr;
      nanocbor_enter_array(decoder, &arr);
      nanocbor_get_uint32(decoder, &endpt_scheme);
      bundle->primary_block.endpoint_scheme = endpt_scheme;
      nanocbor_get_tstr(decoder, (const uint8_t**)&bundle->primary_block.report_eid,&len);
      nanocbor_leave_container(decoder, &arr);

    }
    break;
    case CREATION_TIMESTAMP:
    {
      nanocbor_value_t arr;
      nanocbor_enter_array(decoder, &arr);
      nanocbor_get_uint32(decoder, &bundle->primary_block.creation_timestamp[0]);
      nanocbor_get_uint32(decoder, &bundle->primary_block.creation_timestamp[1]);
      nanocbor_leave_container(decoder, &arr);
    }
    break;
    case LIFETIME:
    {
      uint32_t lifetime;
      nanocbor_get_uint32(decoder, &lifetime);
      bundle->primary_block.lifetime = lifetime;
    }
    break;
    case FRAGMENT_OFFSET:
    {
      if(is_fragment_bundle(bundle)){
        nanocbor_get_uint32(decoder, &bundle->primary_block.fragment_offset);
      }
      else{
        bundle->primary_block.fragment_offset=0;
      }
    }
    break;
    case TOTAL_APPLICATION_DATA_LENGTH:
    {
      if(is_fragment_bundle(bundle)){
        nanocbor_get_uint32(decoder, &bundle->primary_block.total_application_data_length);
      }
      else{
        bundle->primary_block.total_application_data_length=0;
      }
    }
    break;
    case CRC_PRIMARY:
    {
      nanocbor_get_uint32(decoder, &bundle->primary_block.crc);
    }
    break;
    default:
    {
      break;
    }
  }
}

static void decode_canonical_block_element(nanocbor_value_t* decoder, struct bundle_canonical_block_t* block, uint8_t element)
{
    switch(element){
      case TYPE:
      {
        uint32_t temp;
        nanocbor_get_uint32(decoder, &temp);
        block->type = temp;
      }
      break;
      case BLOCK_NUMBER:
      {
        uint32_t temp;
        nanocbor_get_uint32(decoder, &temp);
        block->block_number = temp;
      }
      break;
      case FLAGS_CANONICAL:
      {
        uint32_t temp;
        nanocbor_get_uint32(decoder, &temp);
        block->flags = temp;
      }
      break;
      case CRC_TYPE_CANONICAL:
      {
        uint32_t temp;
        nanocbor_get_uint32(decoder, &temp);
        block->crc_type = temp;
      }
      break;
      case BLOCK_DATA:
      {
        size_t len;
        nanocbor_get_bstr(decoder, (const uint8_t**)&block->block_data, &len);
      }
      break;
      case CRC_CANONICAL:
      {
        uint32_t temp;
        if(block->crc_type != NOCRC){
          nanocbor_get_uint32(decoder, &temp);
          block->crc = temp;
        }
      }
      break;
      default:
      {
        break;
      }
    }
}

//assuming space is preallocated for the bundle here
int bundle_decode(struct actual_bundle* bundle, uint8_t *buffer, size_t buf_len)
{
  nanocbor_value_t decoder;

  nanocbor_decoder_init(&decoder, buffer, buf_len);
  //moving the pointer in the decoder 1 byte ahead to ignore start of indefinite array thing
  decoder.cur++;

  //decoding and parsing the primary block
  nanocbor_value_t arr;
  nanocbor_enter_array(&decoder, &arr);
  decode_primary_block_element(&decoder, bundle, VERSION);
  decode_primary_block_element(&decoder, bundle, FLAGS_PRIMARY);
  decode_primary_block_element(&decoder, bundle, CRC_TYPE_PRIMARY);
  decode_primary_block_element(&decoder, bundle, EID);
  decode_primary_block_element(&decoder, bundle, CREATION_TIMESTAMP);
  decode_primary_block_element(&decoder, bundle, LIFETIME);
  decode_primary_block_element(&decoder, bundle, FRAGMENT_OFFSET);
  decode_primary_block_element(&decoder, bundle, TOTAL_APPLICATION_DATA_LENGTH);
  decode_primary_block_element(&decoder, bundle, CRC_PRIMARY);
  nanocbor_leave_container(&decoder, &arr);

  //decoding and parsing other canonical blocks
  while(!nanocbor_at_end(&decoder)){
    //TODO: Think of solution to prevent too much use of malloc everywhere(so that keeping track of memory becomes easier)
    struct bundle_canonical_block_t* block = (struct bundle_canonical_block_t*) malloc(sizeof(struct bundle_canonical_block_t));
    nanocbor_value_t arr;
    nanocbor_enter_array(&decoder, &arr);
    decode_canonical_block_element(&decoder, block, TYPE);
    decode_canonical_block_element(&decoder, block, BLOCK_NUMBER);
    decode_canonical_block_element(&decoder, block, FLAGS_CANONICAL);
    decode_canonical_block_element(&decoder, block, CRC_TYPE_CANONICAL);
    decode_canonical_block_element(&decoder, block, BLOCK_DATA);
    decode_canonical_block_element(&decoder, block, CRC_CANONICAL);
    nanocbor_leave_container(&decoder, &arr);
    insert_block_in_bundle(bundle, block);
    bundle->num_of_blocks++;
  }
  return 1;
}

uint16_t calculate_crc_16(uint8_t type)
{
  switch(type){
    case BUNDLE_BLOCK_TYPE_PRIMARY:
    {

    }
    break;
    case BUNDLE_BLOCK_TYPE_CANONICAL:
    {

    }
    break;
    default:
    {
      //TODO: Debug statement to send a block/bundle type for calculcation of crc
      return 0;
    }
    break;
  }
  return 0;
}

uint32_t calculate_crc_32(uint8_t type)
{
  switch(type){
    case BUNDLE_BLOCK_TYPE_PRIMARY:
    {

    }
    break;
    case BUNDLE_BLOCK_TYPE_CANONICAL:
    {

    }
    break;
    default:
    {
      //TODO: Debug statement to send a block/bundle type for calculcation of crc
      return 0;
    }
    break;
  }
  return 0;
}

void calculate_primary_flag(uint64_t *flag, bool is_fragment, bool dont_fragment)
{
  if(is_fragment){
    *flag |= FRAGMENT_IDENTIFICATION_MASK;
  }
  if(dont_fragment){
    *flag |= FRAGMENT_IDENTIFICATION_MASK << 2;
  }

  return ;
}

struct actual_bundle* create_bundle(void)
{
  struct actual_bundle* bundle = get_space_for_bundle();
  if(bundle == NULL) {
    DEBUG("No more space in the bundle storage for new bundles.\n");
    return NULL;
  }
  bundle->num_of_blocks=0;
  bundle->other_blocks=NULL;
  return bundle;
}


void fill_bundle(struct actual_bundle* bundle, int version, uint8_t endpoint_scheme, char* dst_eid, char* report_eid, int lifetime, int crc_type)
{
  // TODO: Change later
  int zero_val = 0;
  uint32_t zero_arr[2] = {0};

  //Local vars
  uint64_t primary_flag = 0;
  bool is_fragment= check_if_fragment_bundle();
  bool dont_fragment = true;

  calculate_primary_flag(&primary_flag, is_fragment, dont_fragment);
  DEBUG("bundle: value of flag while setting is '%lld'.\n", primary_flag);
  // DEBUG("bundle: Set version to %d.\n",bundle->primary_block.version);
  // Setting the primary block fields first
  if(!bundle_set_attribute(bundle, VERSION, &version)){
    DEBUG("bundle: Could not set version in bundle");
    return ;
  }
  // DEBUG("bundle: Set version to %d.\n",bundle->primary_block.version);
  // DEBUG("bundle: Set version to %d.\n",bundle->primary_block.version);

  if(!bundle_set_attribute(bundle, FLAGS_PRIMARY, &primary_flag)){
    DEBUG("bundle: Could not set bundle primary flag");
    return ;
  }
  // DEBUG("bundle: Set version to %d.\n",bundle->primary_block.version);

  if(!bundle_set_attribute(bundle, ENDPOINT_SCHEME, &endpoint_scheme)){
    DEBUG("bundle: Could not set bundle endpoint scheme");
    return ;
  }
  // DEBUG("bundle: Set version to %d.\n",bundle->primary_block.version);

  if(!bundle_set_attribute(bundle, CRC_TYPE_PRIMARY, &crc_type)){
    DEBUG("bundle: Could not set bundle crc_type");
    return ;
  }
  // DEBUG("bundle: Set version to %d.\n",bundle->primary_block.version);

  if(!bundle_set_attribute(bundle, SRC_EID, get_src_eid())){
    DEBUG("bundle: Could not set bundle src eid");
    return ;
  }
    // DEBUG("bundle: Set version to %d.\n",bundle->primary_block.version);

  if(!bundle_set_attribute(bundle, DST_EID, dst_eid)){
    DEBUG("bundle: Could not set bundle dst eid");
    return ;
  }
    // DEBUG("bundle: Set version to %d.\n",bundle->primary_block.version);

  if(!bundle_set_attribute(bundle, REPORT_EID, report_eid)){
    DEBUG("bundle: Could not set bundle report eid");
    return ;
  }
  // DEBUG("bundle: Set version to %d.\n",bundle->primary_block.version);

  if(!check_if_node_has_clock()){
    if(!bundle_set_attribute(bundle, CREATION_TIMESTAMP, zero_arr)){
      DEBUG("bundle: Could not set bundle creation time");
      return ;
    }
  }else{
    // TODO: Implement creation_timestamp thing is the node actually has clock
  }
  DEBUG("bundle: Set version to %d.\n",bundle->primary_block.version);

  if(!bundle_set_attribute(bundle, LIFETIME, &lifetime)){
    DEBUG("bundle: Could not set bundle lifetime");
    return ;
  }
  // DEBUG("bundle: Set version to %d.\n",bundle->primary_block.version);

  if(!is_fragment){
    if(!bundle_set_attribute(bundle, FRAGMENT_OFFSET, &zero_val)){
      DEBUG("bundle: Could not set bundle lifetime");
      return ;
    }
    if(!bundle_set_attribute(bundle, TOTAL_APPLICATION_DATA_LENGTH, &zero_val)){
      DEBUG("bundle: Could not set bundle lifetime");
      return ;
    }
  }
  // DEBUG("bundle: Set version to %d.\n",bundle->primary_block.version);

  switch(crc_type){
    case NOCRC:
      {
        if(!bundle_set_attribute(bundle, CRC_PRIMARY, &zero_val)){
          DEBUG("bundle: Could not set bundle crc");
          return ;
        }
        break;
      }
    case CRC_16:
    {
      uint32_t crc = calculate_crc_16(BUNDLE_BLOCK_TYPE_PRIMARY);
      if(!bundle_set_attribute(bundle, CRC_PRIMARY, &crc)){
        DEBUG("bundle: Could not set bundle crc");
        return ;
      }
      break;
    }
    case CRC_32:
    {
      uint32_t crc = calculate_crc_32(BUNDLE_BLOCK_TYPE_PRIMARY);
      if(!bundle_set_attribute(bundle, CRC_PRIMARY, &crc)){
        DEBUG("bundle: Could not set bundle crc");
        return ;
      }
      break;
    }
  }
  // DEBUG("bundle: Set version to %d.\n",bundle->primary_block.version);
}

struct bundle_primary_block_t* bundle_get_primary_block(struct actual_bundle* bundle)
{
  return &(bundle->primary_block);
}

struct bundle_canonical_block_t* bundle_get_payload_block(struct actual_bundle* bundle)
{
  return get_block_by_type(bundle, BUNDLE_BLOCK_TYPE_PAYLOAD);
}
struct bundle_canonical_block_t* get_block_by_type(struct actual_bundle* bundle, uint8_t block_type)
{
  struct bundle_canonical_block_t* temp = bundle->other_blocks;
  while(temp != NULL){
    if(temp->type == block_type){
      return temp;
    }
    temp = temp->next;
  }
  return NULL;
}

int bundle_add_block(struct actual_bundle* bundle, uint8_t type, uint8_t flags, uint8_t *data, uint8_t crc_type, size_t data_len)
{
  struct bundle_canonical_block_t block;
  block.type = type;
  block.flags = flags;
  block.block_number = get_next_block_number();
  block.crc_type = crc_type;
  switch(crc_type){
    case NOCRC:
      {
        block.crc=0;
      }
      break;
    case CRC_16:
      {
        block.crc= calculate_crc_16(BUNDLE_BLOCK_TYPE_CANONICAL);
      }
      break;
    case CRC_32:
      {
        block.crc = calculate_crc_32(BUNDLE_BLOCK_TYPE_CANONICAL);
      }
      break;
  }
  memcpy(block.block_data, data, data_len);
  block.next = NULL;
  insert_block_in_bundle(bundle, &block);
  return 1;
}

uint8_t bundle_get_attribute(struct actual_bundle* bundle, uint8_t type, void* val)
{
  switch(type){
    case VERSION:
    {
      val = &bundle->primary_block.version;
      return 1;
    }
    case FLAGS_PRIMARY:
    {
      val = &bundle->primary_block.flags;
      return 1;
    }
    case ENDPOINT_SCHEME:
    {
      val = &bundle->primary_block.endpoint_scheme;
      return 1;
    }
    case CRC_TYPE_PRIMARY:
    {
      val = &bundle->primary_block.crc_type;
      return 1;
    }
    case SRC_EID:
    {
      val = &bundle->primary_block.src_eid;
      return 1;
    }
    case DST_EID:
    {
      val = &bundle->primary_block.dest_eid;
      return 1;
    }
    case REPORT_EID:
    {
      val = &bundle->primary_block.report_eid;
      return 1;
    }
    case CREATION_TIMESTAMP:
    {
      val = &bundle->primary_block.creation_timestamp;
      return 1;
    }
    case LIFETIME:
    {
      val = &bundle->primary_block.lifetime;
      return 1;
    }
    case FRAGMENT_OFFSET:
    {
      val = &bundle->primary_block.fragment_offset;
      return 1;
    }
    case TOTAL_APPLICATION_DATA_LENGTH:
    {
      val = &bundle->primary_block.total_application_data_length;
      return 1;
    }
    case CRC_PRIMARY:
    {
      val = &bundle->primary_block.crc;
      return 1;
    }
  }
  printf("%s",(char*)val);
  return 0;
}
uint8_t bundle_set_attribute(struct actual_bundle* bundle, uint8_t type, void* val)
{
  switch(type){
    case VERSION:
    {
      // DEBUG("bundle: Setting version to %d.\n",*(uint8_t*)val);
      bundle->primary_block.version = *(uint8_t*)val;
      // DEBUG("bundle: Set version to %d at %p.\n",bundle->primary_block.version, &bundle->primary_block.version);
      // DEBUG("bundle: Set version to %d at %p.\n",bundle->primary_block.version, &bundle->primary_block.version);
      // DEBUG("*****************.\n");
      return 1;
    }
    case FLAGS_PRIMARY:
    {
      bundle->primary_block.flags = *(uint16_t*)val;
      return 1;
    }
    case ENDPOINT_SCHEME:
    {
      bundle->primary_block.endpoint_scheme = *(uint8_t*)val;
      return 1;
    }
    case CRC_TYPE_PRIMARY:
    {
      bundle->primary_block.crc_type = *(uint8_t*)val;
      return 1;
    }
    case SRC_EID:
    {
      bundle->primary_block.src_eid = (uint8_t*) val;
      return 1;
    }
    case DST_EID:
    {
      bundle->primary_block.dest_eid = (uint8_t*) val;
      return 1;
    }
    case REPORT_EID:
    {
      bundle->primary_block.report_eid = (uint8_t*) val;
      return 1;
    }
    case CREATION_TIMESTAMP:
    {
      memcpy(bundle->primary_block.creation_timestamp,(uint32_t*)val,2*sizeof(uint32_t));
      return 1;
    }
    case LIFETIME:
    {
      bundle->primary_block.lifetime = *(uint8_t*)val;
      return 1;
    }
    case FRAGMENT_OFFSET:
    {
      bundle->primary_block.fragment_offset=*(uint32_t*)val;
      return 1;
    }
    case TOTAL_APPLICATION_DATA_LENGTH:
    {
      bundle->primary_block.total_application_data_length=*(uint32_t*)val;
      return 1;
    }
    case CRC_PRIMARY:
    {
      bundle->primary_block.crc=*(uint32_t*)val;
      return 1;
    }
  }
  return 0;
}

void print_bundle(struct actual_bundle* bundle)
{
  (void) bundle;
  DEBUG("Printing primary block of bundle.\n");
  DEBUG("Bundle primary block version: %d\n", bundle->primary_block.version);
  DEBUG("Bundle primary block flags:");
  print_u64_dec(bundle->primary_block.flags);
  DEBUG("\n");
  DEBUG("Bundle primary block endpoint_scheme: %d\n", bundle->primary_block.endpoint_scheme);
  DEBUG("Bundle primary block crc_type: %d\n", bundle->primary_block.crc_type);
  DEBUG("Bundle primary block dest_eid: %s\n", (char*)bundle->primary_block.dest_eid);
  DEBUG("Bundle primary block src_eid: %s\n", (char*)bundle->primary_block.src_eid);
  if(bundle->primary_block.report_eid == NULL) {
    DEBUG("Bundle primary block report_eid: 0.\n");
  } else {
    DEBUG("Bundle primary block report_eid: %s\n",(char*)bundle->primary_block.report_eid);
  }
  // DEBUG("Bundle primary block report_eid: %s\n", bundle->primary_block.report_eid == NULL ? 0 :  (char*)bundle->primary_block.report_eid);
  DEBUG("Bundle primary block creation_timestamp: %ld, %ld\n", bundle->primary_block.creation_timestamp[0], bundle->primary_block.creation_timestamp[1]);
  DEBUG("Bundle primary block lifetime: %d\n", bundle->primary_block.lifetime);
  DEBUG("Bundle primary block fragment_offset: %ld\n", bundle->primary_block.fragment_offset);
  DEBUG("Bundle primary block total_application_data_length: %ld\n", bundle->primary_block.total_application_data_length);
  DEBUG("Bundle primary block crc: %ld\n", bundle->primary_block.crc);
  return ;
}

/*
 * Dummy functions to check various things
 * to be implemented later if need be
*/
char *get_src_eid(void)
{
  return DUMMY_EID;
}

bool check_if_fragment_bundle(void)
{
    return false;
}

bool check_if_node_has_clock(void)
{
  return false;
}
