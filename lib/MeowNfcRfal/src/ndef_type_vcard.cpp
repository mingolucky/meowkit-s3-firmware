
/**
  ******************************************************************************
  * @file           : ndef_type_vcard.cpp
  * @brief          : NDEF MIME vCard type
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
#include "ndef_type_vcard.h"
#include "nfc_utils.h"


#if NDEF_TYPE_VCARD_SUPPORT


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


/*! vCard Type strings */
static const uint8_t ndefMediaTypeVCard[]    = "text/x-vCard";       /*!< vCard Type */

const ndefConstBuffer8 bufMediaTypeVCard     = { ndefMediaTypeVCard, sizeof(ndefMediaTypeVCard) - 1U     };  /*!< vCard Type buffer    */


/*! vCard delimiters */
static const uint8_t COLON[]     = ":";
static const uint8_t SEMICOLON[] = ";";
static const uint8_t NEWLINE[]   = "\r\n";
static const uint8_t LINEFEED[]  = "\n";

static const ndefConstBuffer bufColon     = { COLON,     sizeof(COLON)     - 1U }; /*!< ":"    */
static const ndefConstBuffer bufSemicolon = { SEMICOLON, sizeof(SEMICOLON) - 1U }; /*!< ";"    */
static const ndefConstBuffer bufNewLine   = { NEWLINE,   sizeof(NEWLINE)   - 1U }; /*!< "\r\n" */
static const ndefConstBuffer bufLineFeed  = { LINEFEED,  sizeof(LINEFEED)  - 1U }; /*!< "\n"   */

/*! vCard Payload minimal length (BEGIN:VCARD + VERSION:2.1 + END:VCARD) */
#define NDEF_VCARD_PAYLOAD_LENGTH_MIN    ( sizeof("BEGIN:VCARD") - 1U + sizeof("VERSION:2.1") - 1U + sizeof("END:VCARD") - 1U )


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
 * VCard
 */


/* vCard Helper functions */


/*****************************************************************************/
bool ndefBufferMatch(const ndefConstBuffer *buf1, const ndefConstBuffer *buf2)
{
  if ((buf1 == NULL) || (buf2 == NULL)) {
    return false;
  } else if ((buf1->buffer == buf2->buffer) &&
             (buf1->length == buf2->length)) {
    return true;
  } else {
    if ((buf1->length != 0U) &&
        (buf1->length == buf2->length) &&
        (ST_BYTECMP(buf1->buffer, buf2->buffer, buf1->length) == 0)) {
      return true;
    }
  }

  return false;
}


/*****************************************************************************/
static ReturnCode ndefBufferFind(const ndefConstBuffer *bufPayload, const ndefConstBuffer *bufMarker, uint32_t *offset)
{
  if ((bufPayload == NULL) || (bufPayload->buffer == NULL) ||
      (bufMarker  == NULL) || (bufMarker->buffer  == NULL)) {
    return ERR_PARAM;
  }

  uint32_t i = 0;
  while ((i + bufMarker->length) <= bufPayload->length) {
    ndefConstBuffer bufOffset;
    bufOffset.buffer = &bufPayload->buffer[i];
    bufOffset.length = bufMarker->length;

    if (ndefBufferMatch(&bufOffset, bufMarker) == true) {
      if (offset != NULL) {
        *offset = i;
      }
      return ERR_NONE;
    }
    i++;
  }

  return ERR_NOTFOUND;
}


/*****************************************************************************/
static ReturnCode ndefVCardGetPropertyType(const ndefConstBuffer *bufProperty, ndefConstBuffer *bufType)
{
  ReturnCode err;

  if ((bufProperty == NULL) || (bufType == NULL)) {
    return ERR_PARAM;
  }

  /* Look for the type delimiter semicolon ":" */
  uint32_t colonOffset;
  err = ndefBufferFind(bufProperty, &bufColon, &colonOffset);
  if (err != ERR_NONE) {
    return err;
  }

  bufType->buffer = bufProperty->buffer;
  bufType->length = colonOffset;

  /* Look for the subtype delimiter semicolon ";", if any */
  uint32_t semicolonOffset;
  err = ndefBufferFind(bufProperty, &bufSemicolon, &semicolonOffset);
  if (err == ERR_NONE) {
    bufType->length = MIN(semicolonOffset, colonOffset); /* Type is ahead ";" or ":" */
  }

  return ERR_NONE;
}


/*****************************************************************************/
static ReturnCode ndefVCardGetPropertySubtype(const ndefConstBuffer *bufProperty, ndefConstBuffer *bufSubtype)
{
  ReturnCode err;

  ///* These parameters check are not needed as long as this function is only
  //   called internally (being static) because these parameters are checked by the caller */
  //if ( (bufProperty == NULL) || (bufSubtype == NULL) )
  //{
  //    return ERR_PARAM;
  //}

  /* Look for the type delimiter colon ":" */
  uint32_t colonOffset;
  err = ndefBufferFind(bufProperty, &bufColon, &colonOffset);
  if (err != ERR_NONE) {
    return err;
  }

  /* Look for the subtype delimiter semicolon ";" */
  uint32_t semicolonOffset;
  err = ndefBufferFind(bufProperty, &bufSemicolon, &semicolonOffset);
  if (err != ERR_NONE) {
    return err;
  }

  /* The subtype is between the first semicolon ";" delimiter and ":" delimiter */
  if (semicolonOffset < colonOffset) {
    bufSubtype->buffer = &bufProperty->buffer[semicolonOffset + bufSemicolon.length];
    bufSubtype->length = colonOffset - (semicolonOffset + bufSemicolon.length);
    return ERR_NONE;
  }

  return ERR_NOTFOUND;
}


/*****************************************************************************/
static ReturnCode ndefVCardGetPropertyEOL(const ndefConstBuffer *bufProperty, ndefConstBuffer *bufEOL)
{
  ReturnCode err;
  uint32_t offset;

  if ((bufProperty == NULL) || (bufEOL == NULL)) {
    return ERR_PARAM;
  }

  /* Look for "\r\n" */
  err = ndefBufferFind(bufProperty, &bufNewLine, &offset); /* "\r\n" */
  if (err == ERR_NONE) {
    bufEOL->buffer = bufNewLine.buffer;
    bufEOL->length = bufNewLine.length;
  } else {
    /* Look for "\n" */
    err = ndefBufferFind(bufProperty, &bufLineFeed, &offset); /* "\n" */
    if (err == ERR_NONE) {
      bufEOL->buffer = bufLineFeed.buffer;
      bufEOL->length = bufLineFeed.length;
    } else {
      bufEOL->buffer = NULL;
      bufEOL->length = 0;
    }
  }

  return ERR_NONE;
}


/*****************************************************************************/
static ReturnCode ndefVCardGetPropertyValue(const ndefConstBuffer *bufProperty, ndefConstBuffer *bufValue)
{
  ReturnCode err;

  if ((bufProperty == NULL) || (bufProperty->buffer == NULL) ||
      (bufValue    == NULL)) {
    return ERR_PARAM;
  }

  /* Look for the type delimiter colon ":" */
  uint32_t colonOffset;
  err = ndefBufferFind(bufProperty, &bufColon, &colonOffset);
  if (err != ERR_NONE) {
    return err;
  }

  /* Look for the End-Of-Line */
  ndefConstBuffer bufEOL;
  err = ndefVCardGetPropertyEOL(bufProperty, &bufEOL);
  if (err != ERR_NONE) {
    return err;
  }

  /* Value between ":" and End-Of-Line */
  bufValue->buffer = &bufProperty->buffer[colonOffset + bufColon.length];
  bufValue->length =  bufProperty->length - (colonOffset + bufColon.length + bufEOL.length);

  return ERR_NONE;
}


/*****************************************************************************/
ReturnCode ndefVCardParseProperty(const ndefConstBuffer *bufProperty, ndefConstBuffer *bufType, ndefConstBuffer *bufSubtype, ndefConstBuffer *bufValue)
{
  ReturnCode err;

  if ((bufProperty == NULL) ||
      (bufType     == NULL) || (bufSubtype == NULL) || (bufValue == NULL)) {
    return ERR_PARAM;
  }

  err = ndefVCardGetPropertyType(bufProperty, bufType);
  if (err != ERR_NONE) {
    return err;
  }

  err = ndefVCardGetPropertySubtype(bufProperty, bufSubtype);
  if (err != ERR_NONE) {
    /* Not all properties have a subtype */
    bufSubtype->buffer = NULL;
    bufSubtype->length = 0;
  }

  err = ndefVCardGetPropertyValue(bufProperty, bufValue);
  if (err != ERR_NONE) {
    return err;
  }

  return ERR_NONE;
}


/*****************************************************************************/
ReturnCode ndefVCardSetProperty(ndefTypeVCard *vCard, const ndefConstBuffer *bufProperty)
{
  ReturnCode err;

  if ((vCard == NULL) || (bufProperty == NULL)) {
    return ERR_PARAM;
  }

  /* Check the property contains a type */
  ndefConstBuffer bufPropertyType;
  err = ndefVCardGetPropertyType(bufProperty, &bufPropertyType);
  if (err != ERR_NONE) {
    return err;
  }

  for (uint32_t i = 0; i < (uint32_t)SIZEOF_ARRAY(vCard->propertyBuffer); i++) {
    /* Find first free property */
    if (vCard->propertyBuffer[i] == NULL) {
      /* Append it */
      vCard->propertyBuffer[i] = bufProperty->buffer;
      vCard->propertyLength[i] = (uint8_t)bufProperty->length;
      return ERR_NONE;
    } else {
      /* Or update existing one */
      ndefConstBuffer vCardProperty;
      vCardProperty.buffer = vCard->propertyBuffer[i];
      vCardProperty.length = (uint8_t)vCard->propertyLength[i];

      ndefConstBuffer vCardPropertyType;
      err = ndefVCardGetPropertyType(&vCardProperty, &vCardPropertyType);
      if (err != ERR_NONE) {
        return err;
      }

      if (ndefBufferMatch(&vCardPropertyType, &bufPropertyType) == true) {
        vCard->propertyBuffer[i] = bufProperty->buffer;
        vCard->propertyLength[i] = (uint8_t)bufProperty->length;
        return ERR_NONE;
      }
    }
  }

  return ERR_NOMEM;
}


/*****************************************************************************/
ReturnCode ndefVCardGetProperty(const ndefTypeVCard *vCard, const ndefConstBuffer *bufType, ndefConstBuffer *bufProperty)
{
  ReturnCode err;

  if ((vCard   == NULL) ||
      (bufType == NULL) || (bufType->buffer == NULL)) {
    return ERR_PARAM;
  }

  for (uint32_t i = 0; i < SIZEOF_ARRAY(vCard->propertyBuffer); i++) {
    ndefConstBuffer bufLine;
    bufLine.buffer = vCard->propertyBuffer[i];
    bufLine.length = vCard->propertyLength[i];

    ndefConstBuffer bufLineType;
    err = ndefVCardGetPropertyType(&bufLine, &bufLineType);
    if (err != ERR_NONE) {
      return ERR_NOTFOUND;
    }

    if (ndefBufferMatch(&bufLineType, bufType) == true) {
      if (bufProperty != NULL) {
        bufProperty->buffer = bufLine.buffer;
        bufProperty->length = bufLine.length;
      }
      return ERR_NONE;
    }
  }

  return ERR_NOTFOUND;
}


/*****************************************************************************/
static uint32_t ndefVCardPayloadGetLength(const ndefType *type)
{
  const ndefTypeVCard *ndefData;

  if ((type == NULL) || (type->id != NDEF_TYPE_ID_MEDIA_VCARD)) {
    return 0;
  }

  ndefData = &type->data.vCard;

  uint32_t payloadLength = 0;
  for (uint32_t i = 0; i < SIZEOF_ARRAY(ndefData->propertyBuffer); i++) {
    payloadLength += ndefData->propertyLength[i];
  }

  return payloadLength;
}


/*****************************************************************************/
ReturnCode ndefVCardReset(ndefTypeVCard *vCard)
{
  if (vCard == NULL) {
    return ERR_PARAM;
  }

  /* Initialize every property */
  for (uint32_t i = 0; i < (uint32_t)SIZEOF_ARRAY(vCard->propertyBuffer); i++) {
    vCard->propertyBuffer[i] = NULL;
    vCard->propertyLength[i] = 0;
  }

  return ERR_NONE;
}


/*****************************************************************************/
static const uint8_t *ndefVCardToPayloadItem(const ndefType *type, ndefConstBuffer *bufItem, bool begin)
{
  static uint32_t item = 0;
  const ndefTypeVCard *ndefData;

  if ((type    == NULL) || (type->id != NDEF_TYPE_ID_MEDIA_VCARD) ||
      (bufItem == NULL)) {
    return NULL;
  }

  ndefData = &type->data.vCard;

  bufItem->buffer = NULL;
  bufItem->length = 0;

  /* Initialization */
  if (begin == true) {
    item = 0;
  }

  while (item < (uint32_t)SIZEOF_ARRAY(ndefData->propertyBuffer)) {
    bufItem->buffer = ndefData->propertyBuffer[item];
    bufItem->length = ndefData->propertyLength[item];

    item++;
    return bufItem->buffer;
  }

  return bufItem->buffer;
}


/*****************************************************************************/
ReturnCode ndefVCardInit(ndefType *type, const ndefTypeVCard *vCard)
{
  ndefTypeVCard *ndefData;

  if ((type == NULL) || (vCard == NULL)) {
    return ERR_PARAM;
  }

  type->id               = NDEF_TYPE_ID_MEDIA_VCARD;
  type->getPayloadLength = ndefVCardPayloadGetLength;
  type->getPayloadItem   = ndefVCardToPayloadItem;
  type->typeToRecord     = ndefVCardToRecord;
  ndefData               = &type->data.vCard;

  /* Copy in a bulk */
  (void)ST_MEMCPY(ndefData, vCard, sizeof(ndefTypeVCard));

  return ERR_NONE;
}


/*****************************************************************************/
ReturnCode ndefGetVCard(const ndefType *type, ndefTypeVCard *vCard)
{
  const ndefTypeVCard *ndefData;

  if ((type  == NULL) || (type->id != NDEF_TYPE_ID_MEDIA_VCARD) ||
      (vCard == NULL)) {
    return ERR_PARAM;
  }

  ndefData = &type->data.vCard;

  /* Copy in a bulk */
  (void)ST_MEMCPY(vCard, ndefData, sizeof(ndefTypeVCard));

  return ERR_NONE;
}


/*****************************************************************************/
static ReturnCode ndefVCardGetLine(const ndefConstBuffer *bufPayload, ndefConstBuffer *bufLine)
{
  ReturnCode err;
  uint32_t offset;

  if ((bufPayload == NULL) || (bufLine == NULL)) {
    return ERR_PARAM;
  }

  /* Look for "\r\n" */
  err = ndefBufferFind(bufPayload, &bufNewLine, &offset); /* "\r\n" */
  if (err == ERR_NONE) {
    /* Return up to the marker */
    bufLine->buffer = bufPayload->buffer;
    bufLine->length = offset + bufNewLine.length;
  } else {
    /* Look for "\n" */
    err = ndefBufferFind(bufPayload, &bufLineFeed, &offset); /* "\n" */
    if (err == ERR_NONE) {
      /* Return up to the marker */
      bufLine->buffer = bufPayload->buffer;
      bufLine->length = offset + bufLineFeed.length;
    } else {
      /* Return up to the end of the payload */
      bufLine->buffer = bufPayload->buffer;
      bufLine->length = bufPayload->length;
    }
  }

  return ERR_NONE;
}


/*****************************************************************************/
static ReturnCode ndefPayloadToVcard(const ndefConstBuffer *bufPayload, ndefType *type)
{
  /*! vCard keyword types */
  static const uint8_t TYPE_BEGIN[]   = "BEGIN";
  static const uint8_t TYPE_END[]     = "END";
  static const uint8_t TYPE_VERSION[] = "VERSION";
  /*static const uint8_t VALUE_VCARD[]  = "VCARD";*/
  /*static const uint8_t VALUE_2_1[]    = "2.1";*/

  static const ndefConstBuffer bufTypeBegin   = { TYPE_BEGIN,   sizeof(TYPE_BEGIN)   - 1U }; /*!< "BEGIN"   */
  static const ndefConstBuffer bufTypeEnd     = { TYPE_END,     sizeof(TYPE_END)     - 1U }; /*!< "END"     */
  static const ndefConstBuffer bufTypeVersion = { TYPE_VERSION, sizeof(TYPE_VERSION) - 1U }; /*!< "VERSION" */
  /*static const ndefConstBuffer bufValueVCard  = { VALUE_VCARD,  sizeof(VALUE_VCARD)  - 1U }; *//*!< "VCARD"   */
  /*static const ndefConstBuffer bufValue_2_1   = { VALUE_2_1,    sizeof(VALUE_2_1)    - 1U }; *//*!< "2.1"     */

  ReturnCode err;
  ndefTypeVCard *ndefData;

  ndefConstBuffer bufRemaining;
  ndefConstBuffer bufLine;

  if ((bufPayload == NULL) || (bufPayload->buffer == NULL) ||
      (type       == NULL)) {
    return ERR_PARAM;
  }

  if (bufPayload->length < NDEF_VCARD_PAYLOAD_LENGTH_MIN) { /* vCard Payload Min */
    return ERR_PROTO;
  }

  type->id               = NDEF_TYPE_ID_MEDIA_VCARD;
  type->getPayloadLength = ndefVCardPayloadGetLength;
  type->getPayloadItem   = ndefVCardToPayloadItem;
  type->typeToRecord     = ndefVCardToRecord;
  ndefData               = &type->data.vCard;

  /* Reset the vCard before parsing the payload */
  if (ndefVCardReset(ndefData) != ERR_NONE) {
    return ERR_PARAM;
  }

  uint32_t offset = 0;
  while (offset < bufPayload->length) {
    /* Parse the remaining to find an "end of line" or reach the end of payload */
    bufRemaining.buffer = &bufPayload->buffer[offset];
    bufRemaining.length = bufPayload->length - offset;

    err = ndefVCardGetLine(&bufRemaining, &bufLine);
    if (err != ERR_NONE) {
      return err;
    }

    err = ndefVCardSetProperty(ndefData, &bufLine);
    if (err != ERR_NONE) {
      return err;
    }

    /* Move to the next line */
    offset += bufLine.length;
  }

  /* Check BEGIN, VERSION and END types were found */
  ReturnCode err_begin   = ndefVCardGetProperty(ndefData, &bufTypeBegin, NULL);
  ReturnCode err_version = ndefVCardGetProperty(ndefData, &bufTypeVersion, NULL);
  ReturnCode err_end     = ndefVCardGetProperty(ndefData, &bufTypeEnd, NULL);
  if ((err_begin != ERR_NONE) || (err_version != ERR_NONE) || (err_end != ERR_NONE)) {
    return ERR_SYNTAX;
  }

  return ERR_NONE;
}


/*****************************************************************************/
ReturnCode ndefRecordToVCard(const ndefRecord *record, ndefType *type)
{
  const ndefType *ndefData;

  if ((record == NULL) || (type == NULL)) {
    return ERR_PARAM;
  }

  if (! ndefRecordTypeMatch(record, NDEF_TNF_MEDIA_TYPE, &bufMediaTypeVCard)) { /* "text/x-vCard" */
    return ERR_PROTO;
  }

  ndefData = ndefRecordGetNdefType(record);
  if ((ndefData != NULL) && (type->id == NDEF_TYPE_ID_MEDIA_VCARD)) {
    (void)ST_MEMCPY(type, ndefData, sizeof(ndefType));
    return ERR_NONE;
  }

  return ndefPayloadToVcard(&record->bufPayload, type);
}


/*****************************************************************************/
ReturnCode ndefVCardToRecord(const ndefType *type, ndefRecord *record)
{
  if ((type   == NULL) || (type->id != NDEF_TYPE_ID_MEDIA_VCARD) ||
      (record == NULL)) {
    return ERR_PARAM;
  }

  (void)ndefRecordReset(record);

  (void)ndefRecordSetType(record, NDEF_TNF_MEDIA_TYPE, &bufMediaTypeVCard);

  if (ndefRecordSetNdefType(record, type) != ERR_NONE) {
    return ERR_PARAM;
  }

  return ERR_NONE;
}

#endif
