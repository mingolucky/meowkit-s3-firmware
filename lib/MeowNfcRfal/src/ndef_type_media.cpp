
/**
  ******************************************************************************
  * @file           : ndef_type_media.cpp
  * @brief          : NDEF MIME types
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
#include "ndef_type_media.h"
#include "nfc_utils.h"


#if NDEF_TYPE_MEDIA_SUPPORT


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
 * Media
 */


/*****************************************************************************/
ReturnCode ndefMediaInit(ndefType *media, const ndefConstBuffer8 *bufType, const ndefConstBuffer *bufPayload)
{
  ndefTypeMedia *typeMedia;

  if ((media == NULL) || (bufType == NULL) || (bufPayload == NULL)) {
    return ERR_PARAM;
  }

  media->id               = NDEF_TYPE_ID_MEDIA;
  media->getPayloadLength = NULL;
  media->getPayloadItem   = NULL;
  media->typeToRecord     = ndefMediaToRecord;
  typeMedia               = &media->data.media;

  typeMedia->bufType.buffer    = bufType->buffer;
  typeMedia->bufType.length    = bufType->length;
  typeMedia->bufPayload.buffer = bufPayload->buffer;
  typeMedia->bufPayload.length = bufPayload->length;

  return ERR_NONE;
}


/*****************************************************************************/
ReturnCode ndefGetMedia(const ndefType *media, ndefConstBuffer8 *bufType, ndefConstBuffer *bufPayload)
{
  const ndefTypeMedia *typeMedia;

  if ((media   == NULL) || (media->id != NDEF_TYPE_ID_MEDIA) ||
      (bufType == NULL) || (bufPayload == NULL)) {
    return ERR_PARAM;
  }

  typeMedia = &media->data.media;

  bufType->buffer    = typeMedia->bufType.buffer;
  bufType->length    = typeMedia->bufType.length;

  bufPayload->buffer = typeMedia->bufPayload.buffer;
  bufPayload->length = typeMedia->bufPayload.length;

  return ERR_NONE;
}


/*****************************************************************************/
ReturnCode ndefRecordToMedia(const ndefRecord *record, ndefType *media)
{
  const ndefType *type;
  ndefConstBuffer8 bufType;

  if ((record == NULL) || (media == NULL)) {
    return ERR_PARAM;
  }

  if (ndefHeaderTNF(record) != NDEF_TNF_MEDIA_TYPE) {
    return ERR_PROTO;
  }

  type = ndefRecordGetNdefType(record);
  if ((type != NULL) && (type->id == NDEF_TYPE_ID_MEDIA)) {
    (void)ST_MEMCPY(media, type, sizeof(ndefType));
    return ERR_NONE;
  }

  bufType.buffer = record->type;
  bufType.length = record->typeLength;

  return ndefMediaInit(media, &bufType, &record->bufPayload);
}


/*****************************************************************************/
ReturnCode ndefMediaToRecord(const ndefType *media, ndefRecord *record)
{
  const ndefTypeMedia *typeMedia;

  if ((media  == NULL) || (media->id != NDEF_TYPE_ID_MEDIA) ||
      (record == NULL)) {
    return ERR_PARAM;
  }

  typeMedia = &media->data.media;

  (void)ndefRecordReset(record);

  (void)ndefRecordSetType(record, NDEF_TNF_MEDIA_TYPE, &typeMedia->bufType);

  (void)ndefRecordSetPayload(record, &typeMedia->bufPayload);

  return ERR_NONE;
}

#endif
