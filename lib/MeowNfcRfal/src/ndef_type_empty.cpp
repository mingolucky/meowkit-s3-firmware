
/**
  ******************************************************************************
  * @file           : ndef_type_empty.cpp
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
#include "ndef_type_empty.h"
#include "nfc_utils.h"


#if NDEF_TYPE_EMPTY_SUPPORT


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
 * Empty record
 */


/*****************************************************************************/
static uint32_t ndefEmptyTypePayloadGetLength(const ndefType *empty)
{
  NO_WARNING(empty);

  return 0;
}


/*****************************************************************************/
static const uint8_t *ndefEmptyTypePayloadItem(const ndefType *empty, ndefConstBuffer *bufItem, bool begin)
{
  NO_WARNING(begin);

  if ((empty == NULL) || (empty->id != NDEF_TYPE_ID_EMPTY)) {
    return NULL;
  }

  if (bufItem != NULL) {
    bufItem->buffer = NULL;
    bufItem->length = 0;
  }

  return NULL;
}


/*****************************************************************************/
ReturnCode ndefEmptyTypeInit(ndefType *empty)
{
  if (empty == NULL) {
    return ERR_PARAM;
  }

  empty->id               = NDEF_TYPE_ID_EMPTY;
  empty->getPayloadLength = ndefEmptyTypePayloadGetLength;
  empty->getPayloadItem   = ndefEmptyTypePayloadItem;
  empty->typeToRecord     = ndefEmptyTypeToRecord;

  return ERR_NONE;
}


/*****************************************************************************/
ReturnCode ndefRecordToEmptyType(const ndefRecord *record, ndefType *empty)
{
  ndefConstBuffer8 bufEmpty = { NULL, 0 };

  if ((record == NULL) || (empty == NULL)) {
    return ERR_PARAM;
  }

  if (! ndefRecordTypeMatch(record, NDEF_TNF_EMPTY, &bufEmpty)) {
    return ERR_PARAM;
  }

  if ((record->idLength          != 0U) || (record->id                != NULL) ||
      (record->bufPayload.length != 0U) || (record->bufPayload.buffer != NULL)) {
    return ERR_PARAM;
  }

  return ndefEmptyTypeInit(empty);
}


/*****************************************************************************/
ReturnCode ndefEmptyTypeToRecord(const ndefType *empty, ndefRecord *record)
{
  if ((empty  == NULL) || (empty->id != NDEF_TYPE_ID_EMPTY) ||
      (record == NULL)) {
    return ERR_PARAM;
  }

  (void)ndefRecordReset(record);

  if (ndefRecordSetNdefType(record, empty) != ERR_NONE) {
    return ERR_PARAM;
  }

  return ERR_NONE;
}

#endif
