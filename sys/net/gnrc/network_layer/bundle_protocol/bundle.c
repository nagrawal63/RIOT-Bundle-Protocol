
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
        nanocbor_fmt_array(enc, 2);
        nanocbor_fmt_uint(enc, bundle->primary_block.dst_num);
        nanocbor_fmt_uint(enc, bundle->primary_block.service_num);
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
          nanocbor_fmt_array(enc, 2);
          nanocbor_fmt_uint(enc, bundle->primary_block.src_num);
          nanocbor_fmt_uint(enc, bundle->primary_block.service_num);
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
        nanocbor_fmt_array(enc, 2);
        nanocbor_fmt_uint(enc, bundle->primary_block.report_num);
        nanocbor_fmt_uint(enc, bundle->primary_block.service_num);
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
        nanocbor_fmt_array(enc, 2);
        nanocbor_fmt_uint(enc, bundle->primary_block.dst_num);
        nanocbor_fmt_uint(enc, bundle->primary_block.service_num);
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
        nanocbor_fmt_array(enc, 2);
        nanocbor_fmt_uint(enc, bundle->primary_block.src_num);
        nanocbor_fmt_uint(enc, bundle->primary_block.service_num);
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
        nanocbor_fmt_array(enc, 2);
        nanocbor_fmt_uint(enc, bundle->primary_block.report_num);
        nanocbor_fmt_uint(enc, bundle->primary_block.service_num);
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
        // DEBUG("bundle: decoding bundle VERSION.\n");
        uint32_t temp;
        nanocbor_get_uint32(decoder, &temp);
        bundle->primary_block.version = temp;
        // DEBUG("bundle: version extracted = %lu and set as %d.\n",temp, bundle->primary_block.version);
      }
      break;
    case FLAGS_PRIMARY:
      {
        // DEBUG("bundle: decoding bundle FLAGS_PRIMARY.\n");
        uint32_t temp;
        nanocbor_get_uint32(decoder, &temp);
         bundle->primary_block.flags = temp;
      }
      break;
    case CRC_TYPE_PRIMARY:
    {
      // DEBUG("bundle: decoding bundle CRC_TYPE_PRIMARY.\n");
      uint32_t temp;
      nanocbor_get_uint32(decoder, &temp);
      bundle->primary_block.crc_type = temp;
    }
    break;
    case EID:
    {
      // DEBUG("bundle: decoding bundle EID.\n");
      size_t len;
      uint32_t endpt_scheme;
      nanocbor_value_t arr1;
      nanocbor_enter_array(decoder, &arr1);
      nanocbor_get_uint32(&arr1, &endpt_scheme);
      bundle->primary_block.endpoint_scheme = endpt_scheme;
      if (bundle->primary_block.endpoint_scheme == DTN) {
        nanocbor_get_tstr(&arr1, (const uint8_t**)&bundle->primary_block.dest_eid,&len);
        // DEBUG("bundle: length of decoded dst eid: %d with eid: %s.\n", len, bundle->primary_block.dest_eid);
      }
      else if (bundle->primary_block.endpoint_scheme == IPN) {
        nanocbor_value_t dst_num_arr;
        uint32_t temp;
        nanocbor_enter_array(&arr1, &dst_num_arr);
        nanocbor_get_uint32(&dst_num_arr, &temp);
        bundle->primary_block.dst_num = temp;
        nanocbor_get_uint32(&dst_num_arr, &temp);
        bundle->primary_block.service_num = temp;
        nanocbor_leave_container(&arr1, &dst_num_arr);
        // DEBUG("bundle: decoded dst num: %lu and service num: %lu.\n", bundle->primary_block.dst_num, bundle->primary_block.service_num);
      }
      nanocbor_leave_container(decoder, &arr1);

      nanocbor_value_t arr2;
      nanocbor_enter_array(decoder, &arr2);
      nanocbor_get_uint32(&arr2, &endpt_scheme);
      bundle->primary_block.endpoint_scheme = endpt_scheme;
      if (bundle->primary_block.endpoint_scheme == DTN) {
        nanocbor_get_tstr(&arr2, (const uint8_t**)&bundle->primary_block.src_eid,&len);
        // DEBUG("bundle: length of decoded src eid: %d with eid: %s.\n", len, bundle->primary_block.src_eid);
      }
      else if (bundle->primary_block.endpoint_scheme == IPN) {
        nanocbor_value_t src_num_arr;
        uint32_t temp;
        nanocbor_enter_array(&arr2, &src_num_arr);
        nanocbor_get_uint32(&src_num_arr, &temp);
        bundle->primary_block.src_num = temp;
        nanocbor_get_uint32(&src_num_arr, &temp);
        bundle->primary_block.service_num = temp;
        nanocbor_leave_container(&arr2, &src_num_arr);
        // DEBUG("bundle: decoded src num: %lu and service num : %lu.\n", bundle->primary_block.src_num, bundle->primary_block.service_num);
      }
      nanocbor_leave_container(decoder, &arr2);

      nanocbor_value_t arr3;
      nanocbor_enter_array(decoder, &arr3);
      nanocbor_get_uint32(&arr3, &endpt_scheme);
      bundle->primary_block.endpoint_scheme = endpt_scheme;
      if (bundle->primary_block.endpoint_scheme == DTN) {
        nanocbor_get_tstr(&arr3, (const uint8_t**)&bundle->primary_block.report_eid,&len);
        // DEBUG("bundle: length of decoded report eid: %d with eid: %s.\n", len, bundle->primary_block.report_eid);
      }
      else if (bundle->primary_block.endpoint_scheme == IPN) {
        nanocbor_value_t report_num_arr;
        uint32_t temp;
        nanocbor_enter_array(&arr3, &report_num_arr);
        nanocbor_get_uint32(&report_num_arr, &temp);
        bundle->primary_block.report_num = temp;
        nanocbor_get_uint32(&report_num_arr, &temp);
        bundle->primary_block.service_num = temp;
        nanocbor_leave_container(&arr3, &report_num_arr);
        // DEBUG("bundle: decoded report num: %lu and service num: %lu.\n", bundle->primary_block.report_num, bundle->primary_block.service_num);
      }
      nanocbor_leave_container(decoder, &arr3);
      // DEBUG("bundle: decoded src num: %lu and service num : %lu.\n", bundle->primary_block.src_num, bundle->primary_block.service_num);
      // DEBUG("bundle: decoded service num: %lu.\n", bundle->primary_block.service_num);
    }
    break;
    case CREATION_TIMESTAMP:
    {
      // DEBUG("bundle: decoding bundle CREATION_TIMESTAMP.\n");
      nanocbor_value_t arr;
      nanocbor_enter_array(decoder, &arr);
      nanocbor_get_uint32(&arr, &bundle->primary_block.creation_timestamp[0]);
      nanocbor_get_uint32(&arr, &bundle->primary_block.creation_timestamp[1]);
      nanocbor_leave_container(decoder, &arr);
    }
    break;
    case LIFETIME:
    {
      // DEBUG("bundle: decoding bundle LIFETIME.\n");
      uint32_t lifetime;
      nanocbor_get_uint32(decoder, &lifetime);
      bundle->primary_block.lifetime = lifetime;
    }
    break;
    case FRAGMENT_OFFSET:
    {
      // DEBUG("bundle: decoding bundle FRAGMENT_OFFSET.\n");
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
      // DEBUG("bundle: decoding bundle TOTAL_APPLICATION_DATA_LENGTH.\n");
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
      // DEBUG("bundle: decoding bundle CRC_PRIMARY.\n");
      nanocbor_get_uint32(decoder, &bundle->primary_block.crc);
    }
    break;
    default:
    {
      DEBUG("bundle: inside default of bundle decoding.\n");
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
  DEBUG("bundle: Trying to decode bundle.\n");
  nanocbor_value_t decoder;

  nanocbor_decoder_init(&decoder, buffer, buf_len);
  //moving the pointer in the decoder 1 byte ahead to ignore start of indefinite array thing
  decoder.cur++;
  // DEBUG("bundle: starting bundle at %02x.\n", *decoder.cur);
  //decoding and parsing the primary block
  nanocbor_value_t arr;
  nanocbor_enter_array(&decoder, &arr);
  decode_primary_block_element(&arr, bundle, VERSION);
  // DEBUG("bundle: printing decoded version: %d.\n", bundle->primary_block.version);
  // DEBUG("bundle: inside bundle at %02x, %02x.\n", *decoder.cur, *arr.cur);
  decode_primary_block_element(&arr, bundle, FLAGS_PRIMARY);
  // DEBUG("bundle: inside bundle at %02x, %02x.\n", *decoder.cur, *arr.cur);
  decode_primary_block_element(&arr, bundle, CRC_TYPE_PRIMARY);
  // DEBUG("bundle: inside bundle at %02x, %02x.\n", *decoder.cur, *arr.cur);
  decode_primary_block_element(&arr, bundle, EID);
  // DEBUG("bundle: inside bundle at %02x, %02x.\n", *decoder.cur, *arr.cur);
  decode_primary_block_element(&arr, bundle, CREATION_TIMESTAMP);
  // DEBUG("bundle: inside bundle at %02x, %02x.\n", *decoder.cur, *arr.cur);
  decode_primary_block_element(&arr, bundle, LIFETIME);
  // DEBUG("bundle: inside bundle at %02x, %02x.\n", *decoder.cur, *arr.cur);
  if(is_fragment_bundle(bundle)) {
    decode_primary_block_element(&arr, bundle, FRAGMENT_OFFSET);
    // DEBUG("bundle: inside bundle at %02x, %02x.\n", *decoder.cur, *arr.cur);
    decode_primary_block_element(&arr, bundle, TOTAL_APPLICATION_DATA_LENGTH);
    // DEBUG("bundle: inside bundle at %02x, %02x.\n", *decoder.cur, *arr.cur);
  }
  decode_primary_block_element(&arr, bundle, CRC_PRIMARY);
  // DEBUG("bundle: inside bundle at %02x, %02x.\n", *decoder.cur, *arr.cur);
  nanocbor_leave_container(&decoder, &arr);
  // DEBUG("value of src num: %lu.\n", bundle->primary_block.src_num);
  //decoding and parsing other canonical blocks
  while(!(*decoder.cur == 0xFF && *(decoder.cur+1) == 0x00)){
    // DEBUG("bundle: Inside decoding of canonical block and currently scanning %02x.\n", *decoder.cur);
    //TODO: Think of solution to prevent too much use of malloc everywhere(so that keeping track of memory becomes easier)
    struct bundle_canonical_block_t* block = (struct bundle_canonical_block_t*) malloc(sizeof(struct bundle_canonical_block_t));
    nanocbor_value_t arr;
    nanocbor_enter_array(&decoder, &arr);
    decode_canonical_block_element(&arr, block, TYPE);
    decode_canonical_block_element(&arr, block, BLOCK_NUMBER);
    decode_canonical_block_element(&arr, block, FLAGS_CANONICAL);
    decode_canonical_block_element(&arr, block, CRC_TYPE_CANONICAL);
    decode_canonical_block_element(&arr, block, BLOCK_DATA);
    decode_canonical_block_element(&arr, block, CRC_CANONICAL);
    nanocbor_leave_container(&decoder, &arr);
    insert_block_in_bundle(bundle, block);
    bundle->num_of_blocks++;
  }
  DEBUG("bundle: Finished decoding bundle.\n");
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


void fill_bundle(struct actual_bundle* bundle, int version, uint8_t endpoint_scheme, char* dst_eid, char* report_eid, int lifetime, int crc_type, char* service_num)
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
    DEBUG("bundle: Could not set version in bundle.\n");
    return ;
  }
  // DEBUG("bundle: Set version to %d.\n",bundle->primary_block.version);
  // DEBUG("bundle: Set version to %d.\n",bundle->primary_block.version);

  if(!bundle_set_attribute(bundle, FLAGS_PRIMARY, &primary_flag)){
    DEBUG("bundle: Could not set bundle primary flag.\n");
    return ;
  }
  // DEBUG("bundle: Set version to %d.\n",bundle->primary_block.version);

  if(!bundle_set_attribute(bundle, ENDPOINT_SCHEME, &endpoint_scheme)){
    DEBUG("bundle: Could not set bundle endpoint scheme.\n");
    return ;
  }
  // DEBUG("bundle: Set version to %d.\n",bundle->primary_block.version);

  if(!bundle_set_attribute(bundle, CRC_TYPE_PRIMARY, &crc_type)){
    DEBUG("bundle: Could not set bundle crc_type.\n");
    return ;
  }
  // DEBUG("bundle: Set version to %d.\n",bundle->primary_block.version);
  if (bundle->primary_block.endpoint_scheme == DTN) {
    if(!bundle_set_attribute(bundle, SRC_EID, get_src_eid())){
      DEBUG("bundle: Could not set bundle src eid.\n");
      return ;
    }
      // DEBUG("bundle: Set version to %d.\n",bundle->primary_block.version);

    if(!bundle_set_attribute(bundle, DST_EID, dst_eid)){
      DEBUG("bundle: Could not set bundle dst eid.\n");
      return ;
    }
      // DEBUG("bundle: Set version to %d.\n",bundle->primary_block.version);

    if(!bundle_set_attribute(bundle, REPORT_EID, report_eid)){
      DEBUG("bundle: Could not set bundle report eid.\n");
      return ;
    }
  }
  else if (bundle->primary_block.endpoint_scheme == IPN) {
    if(!bundle_set_attribute(bundle, SRC_NUM, get_src_num())){
      DEBUG("bundle: Could not set bundle src num.\n");
      return ;
    }
      // DEBUG("bundle: Set version to %d.\n",bundle->primary_block.version);

    if(!bundle_set_attribute(bundle, DST_NUM, dst_eid)){
      DEBUG("bundle: Could not set bundle dst num.\n");
      return ;
    }
      // DEBUG("bundle: Set version to %d.\n",bundle->primary_block.version);

    if(!bundle_set_attribute(bundle, REPORT_NUM, report_eid)){
      DEBUG("bundle: Could not set bundle report num.\n");
      return ;
    }
    assert(service_num != NULL);
    if(!bundle_set_attribute(bundle, SERVICE_NUM, service_num)) {
      DEBUG("bundle: Could not set bundle service num.\n");
      return ;
    }
  }
  // DEBUG("bundle: Set version to %d.\n",bundle->primary_block.version);

  if(!check_if_node_has_clock()){
    if(!bundle_set_attribute(bundle, CREATION_TIMESTAMP, zero_arr)){
      DEBUG("bundle: Could not set bundle creation time.\n");
      return ;
    }
  }else{
    // TODO: Implement creation_timestamp thing is the node actually has clock
  }
  DEBUG("bundle: Set version to %d.\n",bundle->primary_block.version);

  if(!bundle_set_attribute(bundle, LIFETIME, &lifetime)){
    DEBUG("bundle: Could not set bundle lifetime.\n");
    return ;
  }
  // DEBUG("bundle: Set version to %d.\n",bundle->primary_block.version);

  if(!is_fragment){
    if(!bundle_set_attribute(bundle, FRAGMENT_OFFSET, &zero_val)){
      DEBUG("bundle: Could not set bundle lifetime.\n");
      return ;
    }
    if(!bundle_set_attribute(bundle, TOTAL_APPLICATION_DATA_LENGTH, &zero_val)){
      DEBUG("bundle: Could not set bundle lifetime.\n");
      return ;
    }
  }
  // DEBUG("bundle: Set version to %d.\n",bundle->primary_block.version);

  switch(crc_type){
    case NOCRC:
      {
        if(!bundle_set_attribute(bundle, CRC_PRIMARY, &zero_val)){
          DEBUG("bundle: Could not set bundle crc.\n");
          return ;
        }
        break;
      }
    case CRC_16:
    {
      uint32_t crc = calculate_crc_16(BUNDLE_BLOCK_TYPE_PRIMARY);
      if(!bundle_set_attribute(bundle, CRC_PRIMARY, &crc)){
        DEBUG("bundle: Could not set bundle crc.\n");
        return ;
      }
      break;
    }
    case CRC_32:
    {
      uint32_t crc = calculate_crc_32(BUNDLE_BLOCK_TYPE_PRIMARY);
      if(!bundle_set_attribute(bundle, CRC_PRIMARY, &crc)){
        DEBUG("bundle: Could not set bundle crc.\n");
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
    case SRC_NUM:
    {
      val = &bundle->primary_block.src_num;
      return 1;
    }
    case DST_NUM:
    {
      val = &bundle->primary_block.dst_num;
      return 1;
    }
    case REPORT_NUM:
    {
      val = &bundle->primary_block.report_num;
      return 1;
    }
    case SERVICE_NUM:
    {
      val = &bundle->primary_block.service_num;
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
    case SRC_NUM:
    {
      bundle->primary_block.src_num = atoi((char*)val);
      return 1;
    }
    case DST_NUM:
    {
      bundle->primary_block.dst_num = atoi((char*)val);
      return 1;
    }
    case REPORT_NUM:
    {
      bundle->primary_block.report_num = atoi((char*)val);
      return 1;
    }
    case SERVICE_NUM:
    {
      bundle->primary_block.service_num = atoi((char*)val);
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
  if(bundle->primary_block.endpoint_scheme == DTN) {
    DEBUG("Bundle primary block dest_eid: %s\n", (char*)bundle->primary_block.dest_eid);
    DEBUG("Bundle primary block src_eid: %s\n", (char*)bundle->primary_block.src_eid);
    if(bundle->primary_block.report_eid == NULL) {
      DEBUG("Bundle primary block report_eid: 0.\n");
    } else {
      DEBUG("Bundle primary block report_eid: %s\n",(char*)bundle->primary_block.report_eid);
    }
  }
  else if (bundle->primary_block.endpoint_scheme == IPN) {
    DEBUG("Bundle primary block dest_num: %lu\n", bundle->primary_block.dst_num);
    DEBUG("Bundle primary block src_num: %lu\n", bundle->primary_block.src_num);
    DEBUG("Bundle primary block report_num: %lu\n",bundle->primary_block.report_num);
    DEBUG("Bundle primary block service_num: %lu\n",bundle->primary_block.service_num);
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
char *get_src_num(void)
{
  return DUMMY_SRC_NUM;
}
bool check_if_fragment_bundle(void)
{
    return false;
}

bool check_if_node_has_clock(void)
{
  return false;
}
