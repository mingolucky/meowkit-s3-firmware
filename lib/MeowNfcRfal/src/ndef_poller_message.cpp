
/**
  ******************************************************************************
  * @file           : ndef_poller_message.cpp
  * @brief          : Provides NDEF methods and definitions to access NFC Forum Tags
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

#include "ndef_poller.h"

/*
 ******************************************************************************
 * ENABLE SWITCH
 ******************************************************************************
 */

/*
 ******************************************************************************
 * GLOBAL DEFINES
 ******************************************************************************
 */

/*
 ******************************************************************************
 * GLOBAL TYPES
 ******************************************************************************
 */

/*
 ******************************************************************************
 * GLOBAL MACROS
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
 * GLOBAL VARIABLE DEFINITIONS
 ******************************************************************************
 */

/*
 ******************************************************************************
 * GLOBAL FUNCTIONS
 ******************************************************************************
 */


#if NDEF_FEATURE_FULL_API

/*******************************************************************************/
static ReturnCode ndefPollerWriteRecord(ndefContext *ctx, const ndefRecord *record, uint32_t *recordOffset)
{
  ReturnCode      err;
  uint8_t         recordHeaderBuf[NDEF_RECORD_HEADER_LEN];
  ndefBuffer      bufHeader;
  ndefConstBuffer bufPayloadItem;
  uint32_t        offset;
  bool            firstPayloadItem;

  if ((ctx == NULL) || (record == NULL) || (recordOffset == NULL)) {
    return ERR_PARAM;
  }

  offset = *recordOffset;

  bufHeader.buffer = recordHeaderBuf;
  bufHeader.length = sizeof(recordHeaderBuf);
  (void)ndefRecordEncodeHeader(record, &bufHeader);
  err = ndefPollerWriteBytes(ctx, offset, bufHeader.buffer, bufHeader.length);
  if (err != ERR_NONE) {
    /* Conclude procedure */
    return err;
  }
  offset += bufHeader.length;

  ndefConstBuffer8 bufType;
  ndefRecordGetType(record, NULL, &bufType);
  if (bufType.length != 0U) {
    err = ndefPollerWriteBytes(ctx, offset, bufType.buffer, bufType.length);
    if (err != ERR_NONE) {
      /* Conclude procedure */
      return err;
    }
    offset += record->typeLength;
  }

  ndefConstBuffer8 bufId;
  ndefRecordGetId(record, &bufId);
  if (bufId.length != 0U) {
    err = ndefPollerWriteBytes(ctx, offset, bufId.buffer, bufId.length);
    if (err != ERR_NONE) {
      /* Conclude procedure */
      return err;
    }
    offset += record->idLength;
  }
  if (ndefRecordGetPayloadLength(record) != 0U) {
    firstPayloadItem = true;
    while (ndefRecordGetPayloadItem(record, &bufPayloadItem, firstPayloadItem) != NULL) {
      firstPayloadItem = false;
      err = ndefPollerWriteBytes(ctx, offset, bufPayloadItem.buffer, bufPayloadItem.length);
      if (err != ERR_NONE) {
        /* Conclude procedure */
        return err;
      }
      offset += bufPayloadItem.length;
    }
  }

  *recordOffset = offset;

  return ERR_NONE;
}

/*******************************************************************************/
ReturnCode ndefPollerWriteMessage(ndefContext *ctx, const ndefMessage *message)
{
  ReturnCode      err;
  ndefMessageInfo info;
  ndefRecord     *record;
  uint32_t        offset;

  if ((ctx == NULL) || (message == NULL)) {
    return ERR_PARAM;
  }

  if ((ctx->state != NDEF_STATE_INITIALIZED) && (ctx->state != NDEF_STATE_READWRITE)) {
    return ERR_WRONG_STATE;
  }

  (void)ndefMessageGetInfo(message, &info);

  /* Verify length of the NDEF message */
  err = ndefPollerCheckAvailableSpace(ctx, info.length);
  if (err != ERR_NONE) {
    /* Conclude procedure */
    return ERR_PARAM;
  }

  /* Reset L-Field/NLEN field */
  err = ndefPollerBeginWriteMessage(ctx, info.length);
  if (err != ERR_NONE) {
    /* Conclude procedure */
    ctx->state = NDEF_STATE_INVALID;
    return err;
  }

  if (info.length != 0U) {
    offset = ctx->messageOffset;

    record = ndefMessageGetFirstRecord(message);
    while (record != NULL) {
      err = ndefPollerWriteRecord(ctx, record, &offset);
      if (err != ERR_NONE) {
        /* Conclude procedure */
        ctx->state = NDEF_STATE_INVALID;
        return err;
      }

      record = ndefMessageGetNextRecord(record);
    }

    err = ndefPollerEndWriteMessage(ctx, info.length);
    if (err != ERR_NONE) {
      /* Conclude procedure */
      ctx->state = NDEF_STATE_INVALID;
      return err;
    }

    /* Procedure complete: Set Read/Write state */
    ctx->state = NDEF_STATE_READWRITE;
  } else {
    /* Procedure complete: Set Initialized state */
    ctx->state = NDEF_STATE_INITIALIZED;
  }

  return ERR_NONE;
}

#endif /* NDEF_FEATURE_FULL_API */
