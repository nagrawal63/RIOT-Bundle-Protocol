/**
 * @ingroup     Bundle protocol
 * @{
 *
 * @file
 * @brief       Bundle implementation (Implements filling, encoding, decoding of bundle) 
 *
 * @author      Nishchay Agrawal <agrawal.nishchay5@gmail.com>
 *
 * @}
 */
#include "checksum/ucrc16.h"
#include "checksum/fletcher32.h"
#include "byteorder.h"

#include "net/gnrc/bundle_protocol/bundle.h"
#include "net/gnrc/bundle_protocol/bundle_storage.h"
#include "od.h"

#define ENABLE_DEBUG (0)
#include "debug.h"

static uint8_t sequence_num = 0;

static bool is_fragment_bundle(struct actual_bundle* bundle);
static int decode_primary_block_element(nanocbor_value_t *decoder, struct actual_bundle* bundle, uint8_t element);
static int decode_canonical_block_element(nanocbor_value_t* decoder, struct bundle_canonical_block_t* block, uint8_t element);

bool is_same_bundle(struct actual_bundle* current_bundle, struct actual_bundle* compare_to_bundle)
{
  if (current_bundle->primary_block.endpoint_scheme == DTN && compare_to_bundle->primary_block.endpoint_scheme == DTN) {
    if (strcmp((char*)current_bundle->primary_block.src_eid,(char*)compare_to_bundle->primary_block.src_eid) != 0){
      return false;
    }
  }
  else if (current_bundle->primary_block.endpoint_scheme == IPN && compare_to_bundle->primary_block.endpoint_scheme == IPN) {
    if (current_bundle->primary_block.src_num != compare_to_bundle->primary_block.src_num) {
      return false;
    }
  }
  else {
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
  
  nanocbor_fmt_array_indefinite(enc);

  //parsing and encoding primary block
  encode_primary_block(bundle, enc);
  //encoding canonical blocks
  struct bundle_canonical_block_t* tempPtr = bundle->other_blocks;
  int i = 0;
  while(i < bundle->num_of_blocks){
    encode_canonical_block(&tempPtr[i], enc);
    i++;
  }
  nanocbor_fmt_end_indefinite(enc);
  return 1;
}

static int decode_primary_block_element(nanocbor_value_t *decoder, struct actual_bundle* bundle, uint8_t element)
{
  switch(element){
    case VERSION:
      {
        uint32_t temp;
        if (nanocbor_get_uint32(decoder, &temp) >= 0) {
          bundle->primary_block.version = temp;
          return 0;
        }
        else {
          return ERROR;
        }
      }
      break;
    case FLAGS_PRIMARY:
      {
        uint32_t temp;
        if (nanocbor_get_uint32(decoder, &temp) >= 0) {
          bundle->primary_block.flags = temp;
          return 0;
        }
        else {
          return ERROR;
        }
      }
      break;
    case CRC_TYPE_PRIMARY:
    {
      uint32_t temp;
      if (nanocbor_get_uint32(decoder, &temp) >= 0) {
        bundle->primary_block.crc_type = temp;
        return 0;
      }
      else {
        return ERROR;
      }
    }
    break;
    case EID:
    {
      size_t len;
      uint32_t endpt_scheme;
      nanocbor_value_t arr1;
      if (nanocbor_enter_array(decoder, &arr1) >= 0) {
        nanocbor_get_uint32(&arr1, &endpt_scheme);
      }
      bundle->primary_block.endpoint_scheme = endpt_scheme;
      if (bundle->primary_block.endpoint_scheme == DTN) {
        nanocbor_get_tstr(&arr1, (const uint8_t**)&bundle->primary_block.dest_eid,&len);

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

      }
      nanocbor_leave_container(decoder, &arr1);

      nanocbor_value_t arr2;
      nanocbor_enter_array(decoder, &arr2);
      nanocbor_get_uint32(&arr2, &endpt_scheme);
      bundle->primary_block.endpoint_scheme = endpt_scheme;
      if (bundle->primary_block.endpoint_scheme == DTN) {
        nanocbor_get_tstr(&arr2, (const uint8_t**)&bundle->primary_block.src_eid,&len);

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

      }
      nanocbor_leave_container(decoder, &arr2);

      nanocbor_value_t arr3;
      nanocbor_enter_array(decoder, &arr3);
      nanocbor_get_uint32(&arr3, &endpt_scheme);
      bundle->primary_block.endpoint_scheme = endpt_scheme;
      if (bundle->primary_block.endpoint_scheme == DTN) {
        nanocbor_get_tstr(&arr3, (const uint8_t**)&bundle->primary_block.report_eid,&len);

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

      }
      nanocbor_leave_container(decoder, &arr3);
    }
    break;
    case CREATION_TIMESTAMP:
    {
      nanocbor_value_t arr;
      nanocbor_enter_array(decoder, &arr);
      nanocbor_get_uint32(&arr, &bundle->primary_block.creation_timestamp[0]);
      nanocbor_get_uint32(&arr, &bundle->primary_block.creation_timestamp[1]);
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
      DEBUG("bundle: inside default of bundle decoding.\n");
      break;
    }
  }
  return 0;
}

static int decode_canonical_block_element(nanocbor_value_t* decoder, struct bundle_canonical_block_t* block, uint8_t element)
{
    switch(element){
      case TYPE:
      {
        uint32_t temp;
        if (nanocbor_get_uint32(decoder, &temp) >= 0){
          block->type = temp;
          return 0;
        }
        else {
          return ERROR;
        }
      }
      break;
      case BLOCK_NUMBER:
      {
        uint32_t temp;
        if (nanocbor_get_uint32(decoder, &temp) >= 0){
          block->block_number = temp;
          return 0;
        }
        else {
          return ERROR;
        }
      }
      break;
      case FLAGS_CANONICAL:
      {
        uint32_t temp;
        if (nanocbor_get_uint32(decoder, &temp) >= 0) {
          block->flags = temp;
          return 0;
        }
        else {
          return ERROR;
        }
      }
      break;
      case CRC_TYPE_CANONICAL:
      {
        uint32_t temp;
        if (nanocbor_get_uint32(decoder, &temp) >= 0){
          block->crc_type = temp;
          return 0;
        }
        else {
          return ERROR;
        }
      }
      break;
      case BLOCK_DATA:
      {
        size_t len;
        const uint8_t *buf = NULL;
        if(nanocbor_get_bstr(decoder, &buf, &len) >= 0 && buf) {
          block->data_len = len;
          // memcpy(block->block_data, buf, len);
          sprintf((char*)block->block_data, "%s", buf);
          return 0;
        }
        else {
          return ERROR;
        }
      }
      break;
      case CRC_CANONICAL:
      {
        uint32_t temp;
        if(block->crc_type != NOCRC){
          if (nanocbor_get_uint32(decoder, &temp) >= 0) {
            block->crc = temp;
            return 0;
          }
          else {
            return ERROR;
          }
        }
      }
      break;
      default:
      {
        return 0;
        break;
      }
    }
    return 0;
}

//assuming space is preallocated for the bundle here
int bundle_decode(struct actual_bundle* bundle, uint8_t *buffer, size_t buf_len)
{
  DEBUG("bundle: Trying to decode bundle.\n");
  nanocbor_value_t decoder;

  nanocbor_decoder_init(&decoder, buffer, buf_len);

  if (*decoder.cur != 0x9f) {
    return ERROR;
  }
  //moving the pointer in the decoder 1 byte ahead to ignore start of indefinite array thing
  decoder.cur++;
  //decoding and parsing the primary block
  nanocbor_value_t arr;
  nanocbor_enter_array(&decoder, &arr);
  
  decode_primary_block_element(&arr, bundle, VERSION);
  decode_primary_block_element(&arr, bundle, FLAGS_PRIMARY);
  decode_primary_block_element(&arr, bundle, CRC_TYPE_PRIMARY);
  decode_primary_block_element(&arr, bundle, EID);
  decode_primary_block_element(&arr, bundle, CREATION_TIMESTAMP);
  decode_primary_block_element(&arr, bundle, LIFETIME);
  if(is_fragment_bundle(bundle)) {
    decode_primary_block_element(&arr, bundle, FRAGMENT_OFFSET);
    decode_primary_block_element(&arr, bundle, TOTAL_APPLICATION_DATA_LENGTH);
  }
  if(bundle->primary_block.crc_type != NOCRC) {
    decode_primary_block_element(&arr, bundle, CRC_PRIMARY);
    uint32_t crc_bundle = bundle->primary_block.crc;
    bundle->primary_block.crc = 0x00000000;
    bool valid_bundle = verify_checksum(bundle, BUNDLE_BLOCK_TYPE_PRIMARY, crc_bundle);
    if (valid_bundle) {
      bundle_set_attribute(bundle, CRC_TYPE_PRIMARY, &crc_bundle);
    }
  }
  else {
    bundle->primary_block.crc = 0x00000000;
  }
  nanocbor_leave_container(&decoder, &arr);

  //decoding and parsing other canonical blocks
  while(!(*decoder.cur == 0xFF && (buffer+buf_len-1) == decoder.cur) && bundle->num_of_blocks < MAX_NUM_OF_BLOCKS){
    struct bundle_canonical_block_t* block = &bundle->other_blocks[bundle->num_of_blocks];
    nanocbor_value_t arr;
    nanocbor_enter_array(&decoder, &arr);
    decode_canonical_block_element(&arr, block, TYPE);
    decode_canonical_block_element(&arr, block, BLOCK_NUMBER);
    decode_canonical_block_element(&arr, block, FLAGS_CANONICAL);
    decode_canonical_block_element(&arr, block, CRC_TYPE_CANONICAL);
    decode_canonical_block_element(&arr, block, BLOCK_DATA);
    decode_canonical_block_element(&arr, block, CRC_CANONICAL);
    nanocbor_leave_container(&decoder, &arr);
    bundle->num_of_blocks++;
  }
  if (*decoder.cur != 0xFF && bundle->num_of_blocks >= MAX_NUM_OF_BLOCKS) {
    return ERROR;
  }
  return 1;
}

int encode_primary_block(struct actual_bundle *bundle, nanocbor_encoder_t *enc)
{
  bool isFragment= is_fragment_bundle(bundle);
  if (!isFragment && bundle->primary_block.crc_type == NOCRC) {
    nanocbor_fmt_array(enc, 8);
    nanocbor_fmt_uint(enc, bundle->primary_block.version);
    nanocbor_fmt_uint(enc, bundle->primary_block.flags);
    nanocbor_fmt_uint(enc, bundle->primary_block.crc_type);
    //encoding destination endpoint
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
  }
  else if (!isFragment && bundle->primary_block.crc_type != NOCRC) {
    nanocbor_fmt_array(enc, 9);
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
  else if (isFragment && bundle->primary_block.crc_type == NOCRC) {
    nanocbor_fmt_array(enc, 10);
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
  /* 
    isFragment && bundle->primary-block.crc_type != NOCRC
   */
  else {
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
  return OK;
}

int encode_canonical_block(struct bundle_canonical_block_t *canonical_block, nanocbor_encoder_t *enc)
{
  if (canonical_block->crc_type == NOCRC) {
    nanocbor_fmt_array(enc,5);
    nanocbor_fmt_int(enc,canonical_block->type);
    nanocbor_fmt_int(enc,canonical_block->block_number);
    nanocbor_fmt_int(enc,canonical_block->flags);
    nanocbor_fmt_int(enc,canonical_block->crc_type);
    nanocbor_put_bstr(enc,canonical_block->block_data, canonical_block->data_len);
  }
  else {
    nanocbor_fmt_array(enc,6);
    nanocbor_fmt_int(enc,canonical_block->type);
    nanocbor_fmt_int(enc,canonical_block->block_number);
    nanocbor_fmt_int(enc,canonical_block->flags);
    nanocbor_fmt_int(enc,canonical_block->crc_type);
    nanocbor_put_bstr(enc,canonical_block->block_data, canonical_block->data_len);
    nanocbor_fmt_int(enc, canonical_block->crc);
  }
  return OK;
}

uint16_t calculate_crc_16(uint8_t type, void *block)
{
  switch(type){
    case BUNDLE_BLOCK_TYPE_PRIMARY:
    {
      uint8_t *data;
      nanocbor_encoder_t enc;
      size_t required_size;

      nanocbor_encoder_init(&enc, NULL, 0);
      encode_primary_block((struct actual_bundle *)block, &enc);
      required_size = nanocbor_encoded_len(&enc);
      data = malloc(required_size);
      nanocbor_encoder_init(&enc, data, required_size);
      encode_primary_block((struct actual_bundle *)block, &enc);

      uint16_t crc_val = ucrc16_calc_be(data, required_size, CRC16_FUNCTION, 0xFFFF);
      DEBUG("bundle: crc_val = %u for data = ", crc_val);
      for(int i=0;i<(int)required_size;i++){
        printf("%02x",data[i]);
      }
      printf(" .\n");
      free(data);
      return crc_val;
    }
    break;
    case BUNDLE_BLOCK_TYPE_CANONICAL:
    {
      uint8_t *data;
      nanocbor_encoder_t enc;
      size_t required_size;

      nanocbor_encoder_init(&enc, NULL, 0);
      encode_canonical_block((struct bundle_canonical_block_t *)block, &enc);
      required_size = nanocbor_encoded_len(&enc);
      data = malloc(required_size);
      nanocbor_encoder_init(&enc, data, required_size);
      encode_canonical_block((struct bundle_canonical_block_t *)block, &enc);
      free(data);

      return byteorder_htons(ucrc16_calc_be(data, required_size, CRC16_FUNCTION, 0xFFFF)).u16;
    }
    break;
    default:
    {
      return 0;
    }
    break;
  }
  return 0;
}

uint32_t calculate_crc_32(uint8_t type, void *block)
{
  switch(type){
    case BUNDLE_BLOCK_TYPE_PRIMARY:
    {
      uint8_t *data;
      nanocbor_encoder_t enc;
      size_t required_size;

      nanocbor_encoder_init(&enc, NULL, 0);
      encode_primary_block((struct actual_bundle *)block, &enc);
      required_size = nanocbor_encoded_len(&enc);
      data = malloc(required_size);
      nanocbor_encoder_init(&enc, data, required_size);
      encode_primary_block((struct actual_bundle *)block, &enc);
      uint32_t crc_val = crc32_func(data, required_size, 0x00000000, CRC32_FUNCTION);
      DEBUG("bundle: crc32_val = %lu for data = ", crc_val);
      for(int i=0;i<(int)required_size;i++){
        printf("%02x",data[i]);
      }
      printf(" .\n");
      free(data);
      return crc_val;
    }
    break;
    case BUNDLE_BLOCK_TYPE_CANONICAL:
    {
      uint8_t *data;
      nanocbor_encoder_t enc;
      size_t required_size;

      nanocbor_encoder_init(&enc, NULL, 0);
      encode_canonical_block((struct bundle_canonical_block_t *)block, &enc);
      required_size = nanocbor_encoded_len(&enc);
      data = malloc(required_size);
      nanocbor_encoder_init(&enc, data, required_size);
      encode_canonical_block((struct bundle_canonical_block_t *)block, &enc);
      uint32_t crc_val = crc32_func(data, required_size, 0x00000000, CRC32_FUNCTION);
      DEBUG("bundle: crc32_val = %lu for data = ", crc_val);
      for(int i=0;i<(int)required_size;i++){
        printf("%02x",data[i]);
      }
      printf(" .\n");
      free(data);
      return crc_val;
    }
    break;
    default:
    {
      return 0;
    }
    break;
  }
  return 0;
}

bool verify_checksum(void *block, uint8_t type, uint32_t crc) 
{
  switch(type){
    case BUNDLE_BLOCK_TYPE_PRIMARY:
    {
      uint8_t *data;
      nanocbor_encoder_t enc;
      size_t required_size;
      uint32_t crc_val = 0;

      nanocbor_encoder_init(&enc, NULL, 0);
      encode_primary_block((struct actual_bundle *)block, &enc);
      required_size = nanocbor_encoded_len(&enc);
      data = malloc(required_size);
      nanocbor_encoder_init(&enc, data, required_size);
      encode_primary_block((struct actual_bundle *)block, &enc);

      if (((struct actual_bundle *)block)->primary_block.crc_type == CRC_16){
        crc_val = ucrc16_calc_be(data, required_size, CRC16_FUNCTION, 0xFFFF);
      }
      else if (((struct actual_bundle *)block)->primary_block.crc_type == CRC_32) {
        crc_val = crc32_func(data, required_size, 0x00000000, CRC32_FUNCTION);
      }
      free(data);
      if (crc_val == crc) {
        DEBUG("bundle: crc check passed.\n");
        return true;
      }
      else {
        return false;
      }
    }
    break;
    case BUNDLE_BLOCK_TYPE_CANONICAL:
    {
      uint8_t *data;
      nanocbor_encoder_t enc;
      size_t required_size;
      uint32_t crc_val = 0;

      nanocbor_encoder_init(&enc, NULL, 0);
      encode_canonical_block((struct bundle_canonical_block_t *)block, &enc);
      required_size = nanocbor_encoded_len(&enc);
      data = malloc(required_size);
      nanocbor_encoder_init(&enc, data, required_size);
      encode_canonical_block((struct bundle_canonical_block_t *)block, &enc);

      if (((struct actual_bundle *)block)->primary_block.crc_type == CRC_16){
        crc_val = ucrc16_calc_be(data, required_size, CRC16_FUNCTION, 0xFFFF);
      }
      else if (((struct actual_bundle *)block)->primary_block.crc_type == CRC_32) {
        crc_val = crc32_func(data, required_size, 0x00000000, CRC32_FUNCTION);
      }
      free(data);
      if (crc_val == crc) {
        DEBUG("bundle: canonical crc check passed.\n");
        return true;
      }
      else {
        return false;
      }
    }
    break;
    default:
    {
      return false;
    }
    break;
  }

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

int calculate_canonical_flag(uint64_t *flag, bool replicate_block)
{
  (void) replicate_block;
  *flag = 0;
  return 0;
}

struct actual_bundle* create_bundle(void)
{
  struct actual_bundle* bundle = get_space_for_bundle();
  if(bundle == NULL) {
    DEBUG("No more space in the bundle storage for new bundles.\n");
    return NULL;
  }
  bundle->num_of_blocks=0;
  bundle->local_creation_time = xtimer_usec_from_ticks(xtimer_now());
  return bundle;
}


int fill_bundle(struct actual_bundle* bundle, int version, uint8_t endpoint_scheme, char* dst_eid, char* report_eid, uint32_t lifetime, int crc_type, char* service_num)
{
  if (strcmp(dst_eid, get_src_num()) == 0) {
    DEBUG("bundle: Source and destination address cannot be same.\n");
    return ERROR;
  }
  int zero_val = 0;
  uint32_t creation_timestamp_arr[2] = {0, sequence_num++};

  //Local vars
  uint64_t primary_flag = 0;
  bool is_fragment= check_if_fragment_bundle();
  bool dont_fragment = true;

  bundle->previous_endpoint_num = INVALID_EID;
  calculate_primary_flag(&primary_flag, is_fragment, dont_fragment);
  
  // Setting the primary block fields first
  if(!bundle_set_attribute(bundle, VERSION, &version)){
    DEBUG("bundle: Could not set version in bundle.\n");
    return ERROR;
  }

  if(!bundle_set_attribute(bundle, FLAGS_PRIMARY, &primary_flag)){
    DEBUG("bundle: Could not set bundle primary flag.\n");
    return ERROR;
  }

  if(!bundle_set_attribute(bundle, ENDPOINT_SCHEME, &endpoint_scheme)){
    DEBUG("bundle: Could not set bundle endpoint scheme.\n");
    return ERROR;
  }

  if(!bundle_set_attribute(bundle, CRC_TYPE_PRIMARY, &crc_type)){
    DEBUG("bundle: Could not set bundle crc_type.\n");
    return ERROR;
  }
  
  if (bundle->primary_block.endpoint_scheme == DTN) {
    if(!bundle_set_attribute(bundle, SRC_EID, get_src_eid())){
      DEBUG("bundle: Could not set bundle src eid.\n");
      return ERROR;
    }

    if(!bundle_set_attribute(bundle, DST_EID, dst_eid)){
      DEBUG("bundle: Could not set bundle dst eid.\n");
      return ERROR;
    }

    if(!bundle_set_attribute(bundle, REPORT_EID, report_eid)){
      DEBUG("bundle: Could not set bundle report eid.\n");
      return ERROR;
    }
  }
  else if (bundle->primary_block.endpoint_scheme == IPN) {
    if(!bundle_set_attribute(bundle, SRC_NUM, get_src_num())){
      DEBUG("bundle: Could not set bundle src num.\n");
      return ERROR;
    }

    if(!bundle_set_attribute(bundle, DST_NUM, dst_eid)){
      DEBUG("bundle: Could not set bundle dst num.\n");
      return ERROR;
    }

    if(!bundle_set_attribute(bundle, REPORT_NUM, report_eid)){
      DEBUG("bundle: Could not set bundle report num.\n");
      return ERROR;
    }
    assert(service_num != NULL);
    if(!bundle_set_attribute(bundle, SERVICE_NUM, service_num)) {
      DEBUG("bundle: Could not set bundle service num.\n");
      return ERROR;
    }
  }

  if(!check_if_node_has_clock()){
    if(!bundle_set_attribute(bundle, CREATION_TIMESTAMP, creation_timestamp_arr)){
      DEBUG("bundle: Could not set bundle creation time.\n");
      return ERROR;
    }
  }else{
    // TODO: Implement creation_timestamp thing if the node actually has clock
  }

  if(!bundle_set_attribute(bundle, LIFETIME, &lifetime)){
    DEBUG("bundle: Could not set bundle lifetime.\n");
    return ERROR;
  }

  if(!is_fragment){
    if(!bundle_set_attribute(bundle, FRAGMENT_OFFSET, &zero_val)){
      DEBUG("bundle: Could not set bundle lifetime.\n");
      return ERROR;
    }
    if(!bundle_set_attribute(bundle, TOTAL_APPLICATION_DATA_LENGTH, &zero_val)){
      DEBUG("bundle: Could not set bundle lifetime.\n");
      return ERROR;
    }
  }
  
  /*
    Sets the crc value to be zero by default and is required to be set later inside the convergence layer after encoding 
    the bundle for the first time
  */
  uint32_t zero_crc = 0x00000000;
  switch(crc_type){
    case NOCRC:
      {
        if(!bundle_set_attribute(bundle, CRC_PRIMARY, &zero_crc)){
          DEBUG("bundle: Could not set bundle crc.\n");
          return ERROR;
        }
        break;
      }
    case CRC_16:
    {
      if(!bundle_set_attribute(bundle, CRC_PRIMARY, &zero_crc)){
        DEBUG("bundle: Could not set bundle crc.\n");
        return ERROR;
      }
      uint16_t crc = calculate_crc_16(BUNDLE_BLOCK_TYPE_PRIMARY, bundle);
      if(!bundle_set_attribute(bundle, CRC_PRIMARY, &crc)){
        DEBUG("bundle: Could not set bundle crc.\n");
        return ERROR;
      }
      break;
    }
    case CRC_32:
    {
      uint32_t crc = calculate_crc_32(BUNDLE_BLOCK_TYPE_PRIMARY, bundle);
      if(!bundle_set_attribute(bundle, CRC_PRIMARY, &crc)){
        DEBUG("bundle: Could not set bundle crc.\n");
        return ERROR;
      }
      break;
    }
  }
  return OK;
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
  int i = 0;
  while(temp != NULL && i < bundle->num_of_blocks && i < MAX_NUM_OF_BLOCKS ){
    temp = &bundle->other_blocks[i];
    DEBUG("bundle: Going to compare block of type: %d with %d.\n", temp->type, block_type);
    if(temp->type == block_type){
      return temp;
    }
    i++;
  }
  return NULL;
}

int bundle_add_block(struct actual_bundle* bundle, uint8_t type, uint64_t flags, uint8_t *data, uint8_t crc_type, size_t data_len)
{
  if (bundle->num_of_blocks == MAX_NUM_OF_BLOCKS) {
    DEBUG("bundle: Cannot add more blocks to bundle.\n");
    return ERROR;
  }
  struct bundle_canonical_block_t *block = &bundle->other_blocks[bundle->num_of_blocks];
  block->type = type;
  block->flags = flags;
  block->block_number = get_next_block_number();
  block->crc_type = crc_type;
  switch(crc_type){
    case NOCRC:
      {
        block->crc=0;
      }
      break;
    case CRC_16:
      {
        block->crc = calculate_crc_16(BUNDLE_BLOCK_TYPE_CANONICAL, block);
      }
      break;
    case CRC_32:
      {
        block->crc = calculate_crc_32(BUNDLE_BLOCK_TYPE_CANONICAL, block);
      }
      break;
  }
  memcpy(block->block_data, data, data_len);
  block->data_len = data_len;
  bundle->num_of_blocks++;
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
      bundle->primary_block.version = *(uint8_t*)val;
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
      bundle->primary_block.lifetime = *(uint32_t*)val;
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
  DEBUG("Printing bundle created at %lu.\n", bundle->local_creation_time);
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
  DEBUG("Bundle primary block lifetime: %lu\n", bundle->primary_block.lifetime);
  DEBUG("Bundle primary block fragment_offset: %ld\n", bundle->primary_block.fragment_offset);
  DEBUG("Bundle primary block total_application_data_length: %ld\n", bundle->primary_block.total_application_data_length);
  DEBUG("Bundle primary block crc: %ld\n", bundle->primary_block.crc);

  struct bundle_canonical_block_t *temp = bundle->other_blocks;
  int i = 0;
  while (i < bundle->num_of_blocks) {
    DEBUG("Bundle canonical block of type: %d with data_len: %u and data: \n", temp[i].type, temp[i].data_len);
    od_hex_dump(temp[i].block_data, temp[i].data_len, OD_WIDTH_DEFAULT);

    i++;
  }
  return ;
}

int increment_bundle_age(struct bundle_canonical_block_t *bundle_age_block, struct actual_bundle *bundle) {
  uint32_t usecs_from_bundle = strtoul((char*)bundle_age_block->block_data, NULL, bundle_age_block->data_len);
  uint32_t updated_time = usecs_from_bundle + (xtimer_now().ticks32 - bundle->local_creation_time);
  if(updated_time > bundle->primary_block.lifetime) {
    DEBUG("bundle: lifetime of bundle expired.\n");
    set_retention_constraint(bundle, NO_RETENTION_CONSTRAINT);
    // delete_bundle(bundle);
    return ERROR;
  }
  sprintf((char*)bundle_age_block->block_data, "%lu", updated_time);
  return OK;
}

int reset_bundle_age(struct bundle_canonical_block_t *bundle_age_block, uint32_t original_age) {
  sprintf((char*)bundle_age_block->block_data, "%lu", original_age);
  return OK;
}

bool is_expired_bundle(struct actual_bundle *bundle) {
  struct bundle_canonical_block_t *block = get_block_by_type(bundle, BUNDLE_BLOCK_TYPE_BUNDLE_AGE);
  if (block != NULL) {
    uint32_t usecs_from_bundle = strtoul((char*)block->block_data, NULL, block->data_len);
    if ((usecs_from_bundle + (xtimer_now().ticks32 - bundle->local_creation_time)) >= bundle->primary_block.lifetime) {
      DEBUG("bundle: Bundle is expired with current age: %lu and lifetime : %lu.\n", (usecs_from_bundle + (xtimer_now().ticks32 - bundle->local_creation_time))
                                                                                    , bundle->primary_block.lifetime);
      return true;
    }
    else {
      return false;
    }
  }
  else {
    return false;
  }
}

void set_retention_constraint(struct actual_bundle *bundle, uint8_t constraint) {
  bundle->retention_constraint = constraint;
}

uint8_t get_retention_constraint(struct actual_bundle *bundle) {
  return bundle->retention_constraint;
}


uint32_t crc32_func(const void* data, size_t length, uint32_t previousCrc32, uint32_t polynomial) {

  uint32_t crc = ~previousCrc32; 
  unsigned char* current = (unsigned char*) data;
  while (length--)
  {
    crc ^= *current++;
    for (unsigned int j = 0; j < 8; j++)
      if (crc & 1)
        crc = (crc >> 1) ^ polynomial;
      else
        crc =  crc >> 1;
  }
  return ~crc;
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
