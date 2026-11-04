
/**
  ******************************************************************************
  * @file           : ndef_message.cpp
  * @brief          : NDEF message
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
#include "ndef_message.h"
#include "nfc_utils.h"


/*
 ******************************************************************************
 * GLOBAL DEFINES
 ******************************************************************************
 */

#define NDEF_MAX_RECORD          10U    /*!< Maximum number of records */

/*
 ******************************************************************************
 * GLOBAL TYPES
 ******************************************************************************
 */


/*
 ******************************************************************************
 * LOCAL VARIABLES
 ******************************************************************************
 */
static uint8_t ndefRecordPoolIndex = 0;


/*
 ******************************************************************************
 * LOCAL FUNCTION PROTOTYPES
 ******************************************************************************
 */


/*****************************************************************************/
static ndefRecord *ndefAllocRecord(void)
{
  static ndefRecord ndefRecordPool[NDEF_MAX_RECORD];

  if (ndefRecordPoolIndex >= NDEF_MAX_RECORD) {
    return NULL;
  }

  return &ndefRecordPool[ndefRecordPoolIndex++];
}


/*
 ******************************************************************************
 * GLOBAL FUNCTIONS
 ******************************************************************************
 */
/*****************************************************************************/


ReturnCode ndefMessageInit(ndefMessage *message)
{
  if (message == NULL) {
    return ERR_PARAM;
  }

  message->record           = NULL;
  message->info.length      = 0;
  message->info.recordCount = 0;

  ndefRecordPoolIndex = 0;

  return ERR_NONE;
}


/*****************************************************************************/
ReturnCode ndefMessageGetInfo(const ndefMessage *message, ndefMessageInfo *info)
{
  ndefRecord *record;
  uint32_t    length      = 0;
  uint32_t    recordCount = 0;

  if ((message == NULL) || (info == NULL)) {
    return ERR_PARAM;
  }

  record = message->record;

  while (record != NULL) {
    length += ndefRecordGetLength(record);
    recordCount++;

    record = record->next;
  }

  info->length      = length;
  info->recordCount = recordCount;

  return ERR_NONE;
}


/*****************************************************************************/
uint32_t ndefMessageGetRecordCount(const ndefMessage *message)
{
  ndefMessageInfo info;

  if (ndefMessageGetInfo(message, &info) == ERR_NONE) {
    return info.recordCount;
  }

  return 0;
}


/*****************************************************************************/
ReturnCode ndefMessageAppend(ndefMessage *message, ndefRecord *record)
{
  if ((message == NULL) || (record == NULL)) {
    return ERR_PARAM;
  }

  /* Clear the Message Begin bit */
  ndefHeaderClearMB(record);

  /* Record is appended so it is the last in the list, set the Message End bit */
  ndefHeaderSetME(record);

  record->next = NULL;

  if (message->record == NULL) {
    /* Set the Message Begin bit for the first record only */
    ndefHeaderSetMB(record);

    message->record = record;
  } else {
    ndefRecord *current = message->record;

    /* Go through the list of records */
    while (current->next != NULL) {
      current = current->next;
    }

    /* Clear the Message End bit to the record before the one being appended */
    ndefHeaderClearME(current);

    /* Append to the last record */
    current->next = record;
  }

  message->info.length      += ndefRecordGetLength(record);
  message->info.recordCount += 1U;

  return ERR_NONE;
}


/*****************************************************************************/
ReturnCode ndefMessageDecode(const ndefConstBuffer *bufPayload, ndefMessage *message)
{
  ReturnCode err;
  uint32_t offset;

  if ((bufPayload == NULL) || (bufPayload->buffer == NULL)) {
    return ERR_PARAM;
  }

  err = ndefMessageInit(message);
  if (err != ERR_NONE) {
    return err;
  }

  offset = 0;
  while (offset < bufPayload->length) {
    ndefConstBuffer bufRecord;
    ndefRecord *record = ndefAllocRecord();
    if (record == NULL) {
      return ERR_NOMEM;
    }
    bufRecord.buffer = &bufPayload->buffer[offset];
    bufRecord.length =  bufPayload->length - offset;
    err = ndefRecordDecode(&bufRecord, record);
    if (err != ERR_NONE) {
      return err;
    }
    offset += ndefRecordGetLength(record);

    err = ndefMessageAppend(message, record);
    if (err != ERR_NONE) {
      return err;
    }
  }

  return ERR_NONE;
}


#if NDEF_FEATURE_FULL_API
/*****************************************************************************/
ReturnCode ndefMessageEncode(const ndefMessage *message, ndefBuffer *bufPayload)
{
  ReturnCode      err;
  ndefMessageInfo info;
  ndefRecord     *record;
  uint32_t        offset;
  uint32_t        remainingLength;

  if ((bufPayload == NULL) || (bufPayload->buffer == NULL)) {
    return ERR_PARAM;
  }

  err = ndefMessageGetInfo(message, &info);
  if ((err != ERR_NONE) || (bufPayload->length < info.length)) {
    bufPayload->length = info.length;
    return ERR_NOMEM;
  }

  /* Get the first record */
  record          = ndefMessageGetFirstRecord(message);
  offset          = 0;
  remainingLength = bufPayload->length;

  while (record != NULL) {
    ndefBuffer bufRecord;
    bufRecord.buffer = &bufPayload->buffer[offset];
    bufRecord.length = remainingLength;
    err = ndefRecordEncode(record, &bufRecord);
    if (err != ERR_NONE) {
      bufPayload->length = info.length;
      return err;
    }
    offset          += bufRecord.length;
    remainingLength -= bufRecord.length;

    record = ndefMessageGetNextRecord(record);
  }

  bufPayload->length = offset;
  return ERR_NONE;
}
#endif


#if NDEF_FEATURE_FULL_API
/*****************************************************************************/
ndefRecord *ndefMessageFindRecordType(ndefMessage *message, uint8_t tnf, const ndefConstBuffer8 *bufType)
{
  ndefRecord *record;

  record = ndefMessageGetFirstRecord(message);

  while (record != NULL) {
    if (ndefRecordTypeMatch(record, tnf, bufType) == true) {
      return record;
    }

    record = ndefMessageGetNextRecord(record);
  }

  return record;
}
#endif
