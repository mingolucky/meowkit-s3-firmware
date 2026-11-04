
/**
  ******************************************************************************
  * @file           : ndef_type_flat.cpp
  * @brief          : NDEF Empty type
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/*
 ******************************************************************************
 * INCLUDES
 ******************************************************************************
 */

#include "ndef_record.h"
#include "ndef_types.h"
#include "ndef_type_flat.h"
#include "nfc_utils.h"


#if NDEF_TYPE_FLAT_SUPPORT


/*
 ******************************************************************************
 * GLOBAL DEFINES
 ******************************************************************************
 */


/*
 ******************************************************************************
 * LOCAL VARIABLES
 ******************************************************************************
 */


/*
 ******************************************************************************
 * LOCAL FUNCTION PROTOTYPES
 ******************************************************************************
 */


/*
 ******************************************************************************
 * GLOBAL FUNCTIONS
 ******************************************************************************
 */


/*
 * Flat payload record
 */


/*****************************************************************************/
static uint32_t ndefFlatPayloadTypePayloadGetLength(const ndefType *type)
{
  if ((type == NULL) || (type->id != NDEF_TYPE_ID_FLAT)) {
    return 0;
  }

  return type->data.bufPayload.length;
}


/*****************************************************************************/
static const uint8_t *ndefFlatPayloadTypePayloadItem(const ndefType *type, ndefConstBuffer *bufItem, bool begin)
{
  if ((type == NULL) || (type->id != NDEF_TYPE_ID_FLAT)) {
    return NULL;
  }

  if ((begin == true) && (bufItem != NULL)) {
    bufItem->buffer = type->data.bufPayload.buffer;
    bufItem->length = type->data.bufPayload.length;

    return bufItem->buffer;
  }

  return NULL;
}


/*****************************************************************************/
ReturnCode ndefFlatPayloadTypeInit(ndefType *type, const ndefConstBuffer *bufPayload)
{
  if ((type == NULL) || (bufPayload == NULL)) {
    return ERR_PARAM;
  }

  type->id               = NDEF_TYPE_ID_FLAT;
  type->getPayloadLength = ndefFlatPayloadTypePayloadGetLength;
  type->getPayloadItem   = ndefFlatPayloadTypePayloadItem;
  type->typeToRecord     = ndefFlatPayloadTypeToRecord;

  type->data.bufPayload.buffer = bufPayload->buffer;
  type->data.bufPayload.length = bufPayload->length;

  return ERR_NONE;
}


/*****************************************************************************/
ReturnCode ndefGetFlatPayloadType(const ndefType *type, ndefConstBuffer *bufPayload)
{
  if ((type     == NULL) || (type->id != NDEF_TYPE_ID_FLAT) ||
      (bufPayload == NULL)) {
    return ERR_PARAM;
  }

  bufPayload->buffer = type->data.bufPayload.buffer ;
  bufPayload->length = type->data.bufPayload.length ;

  return ERR_NONE;
}


/*****************************************************************************/
ReturnCode ndefRecordToFlatPayloadType(const ndefRecord *record, ndefType *type)
{
  const ndefType *ndefData;

  if ((record == NULL) || (type == NULL)) {
    return ERR_PARAM;
  }

  ndefData = ndefRecordGetNdefType(record);
  if ((ndefData != NULL) && (ndefData->id == NDEF_TYPE_ID_FLAT)) {
    (void)ST_MEMCPY(type, ndefData, sizeof(ndefType));
    return ERR_NONE;
  }

  ndefConstBuffer bufPayload;
  ReturnCode err = ndefRecordGetPayload(record, &bufPayload);
  if (err != ERR_NONE) {
    return err;
  }

  return ndefFlatPayloadTypeInit(type, &bufPayload);
}


/*****************************************************************************/
ReturnCode ndefFlatPayloadTypeToRecord(const ndefType *type, ndefRecord *record)
{
  if ((type   == NULL) || (type->id != NDEF_TYPE_ID_FLAT) ||
      (record == NULL)) {
    return ERR_PARAM;
  }

  (void)ndefRecordReset(record);

  /* Do not initialize Type string */

  if (ndefRecordSetNdefType(record, type) != ERR_NONE) {
    return ERR_PARAM;
  }

  return ERR_NONE;
}

#endif
