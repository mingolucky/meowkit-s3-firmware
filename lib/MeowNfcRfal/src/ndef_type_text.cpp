
/**
  ******************************************************************************
  * @file           : ndef_type_text.cpp
  * @brief          : NDEF RTD Text type
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
#include "ndef_type_text.h"
#include "nfc_utils.h"


#if NDEF_TYPE_RTD_TEXT_SUPPORT


/*
 ******************************************************************************
 * GLOBAL DEFINES
 ******************************************************************************
 */


/*! Text defines */
#define NDEF_RTD_TEXT_STATUS_OFFSET              0U    /*!< Text status offset */
#define NDEF_RTD_TEXT_LANGUAGE_OFFSET            1U    /*!< Text language offset */

#define NDEF_RTD_TEXT_LANGUAGE_CODE_LEN_MASK  0x3FU    /*!< IANA language code mask (length coded on 6 bits) */

#define NDEF_RTD_TEXT_PAYLOAD_LENGTH_MIN         (sizeof(uint8_t) + sizeof(uint8_t))   /*!< Minimum Text Payload length */


/*
 ******************************************************************************
 * LOCAL VARIABLES
 ******************************************************************************
 */


/*! RTD Text Type string */
static const uint8_t ndefRtdTypeText[]           = "T";               /*!< Text Record Type               {0x54}       */

const ndefConstBuffer8 bufRtdTypeText            = { ndefRtdTypeText,       sizeof(ndefRtdTypeText) - 1U };       /*!< Text Record Type buffer               */


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
 * Text
 */


/*****************************************************************************/
static uint32_t ndefRtdTextPayloadGetLength(const ndefType *text)
{
  const ndefTypeRtdText *rtdText;

  if ((text == NULL) || (text->id != NDEF_TYPE_ID_RTD_TEXT)) {
    return 0;
  }

  rtdText = &text->data.text;

  return sizeof(rtdText->status) + rtdText->bufLanguageCode.length + rtdText->bufSentence.length;
}


/*****************************************************************************/
static const uint8_t *ndefRtdTextToPayloadItem(const ndefType *text, ndefConstBuffer *bufItem, bool begin)
{
  static uint32_t item = 0;
  const ndefTypeRtdText *rtdText;

  if ((text    == NULL) || (text->id != NDEF_TYPE_ID_RTD_TEXT) ||
      (bufItem == NULL)) {
    return NULL;
  }

  rtdText = &text->data.text;

  if (begin == true) {
    item = 0;
  }

  switch (item) {
    case 0:
      /* Status byte */
      bufItem->buffer = &rtdText->status;
      bufItem->length = sizeof(rtdText->status);
      break;

    case 1:
      /* Language Code */
      bufItem->buffer = rtdText->bufLanguageCode.buffer;
      bufItem->length = rtdText->bufLanguageCode.length;
      break;

    case 2:
      /* Actual text */
      bufItem->buffer = rtdText->bufSentence.buffer;
      bufItem->length = rtdText->bufSentence.length;
      break;

    default:
      bufItem->buffer = NULL;
      bufItem->length = 0;
      break;
  }

  /* Move to next item for next call */
  item++;

  return bufItem->buffer;
}


/*****************************************************************************/
ReturnCode ndefRtdTextInit(ndefType *text, uint8_t utfEncoding, const ndefConstBuffer8 *bufLanguageCode, const ndefConstBuffer *bufSentence)
{
  ndefTypeRtdText *rtdText;

  if ((text            == NULL) ||
      (bufLanguageCode == NULL) || (bufLanguageCode->buffer == NULL) || (bufLanguageCode->length == 0U) ||
      (bufSentence     == NULL) || (bufSentence->buffer     == NULL) || (bufSentence->length     == 0U)) {
    return ERR_PARAM;
  }

  if (bufLanguageCode->length > NDEF_RTD_TEXT_LANGUAGE_CODE_LEN_MASK) {
    return ERR_PROTO;
  }

  if ((utfEncoding != TEXT_ENCODING_UTF8) && (utfEncoding != TEXT_ENCODING_UTF16)) {
    return ERR_PARAM;
  }

  text->id               = NDEF_TYPE_ID_RTD_TEXT;
  text->getPayloadLength = ndefRtdTextPayloadGetLength;
  text->getPayloadItem   = ndefRtdTextToPayloadItem;
  text->typeToRecord     = ndefRtdTextToRecord;
  rtdText                = &text->data.text;

  rtdText->status = (utfEncoding << NDEF_TEXT_ENCODING_SHIFT) | (bufLanguageCode->length & NDEF_RTD_TEXT_LANGUAGE_CODE_LEN_MASK);

  rtdText->bufLanguageCode.buffer = bufLanguageCode->buffer;
  rtdText->bufLanguageCode.length = bufLanguageCode->length;

  rtdText->bufSentence.buffer = bufSentence->buffer;
  rtdText->bufSentence.length = bufSentence->length;

  return ERR_NONE;
}


/*****************************************************************************/
ReturnCode ndefGetRtdText(const ndefType *text, uint8_t *utfEncoding, ndefConstBuffer8 *bufLanguageCode, ndefConstBuffer *bufSentence)
{
  const ndefTypeRtdText *rtdText;

  if ((text        == NULL) || (text->id != NDEF_TYPE_ID_RTD_TEXT) ||
      (utfEncoding == NULL) || (bufLanguageCode == NULL) || (bufSentence == NULL)) {
    return ERR_PARAM;
  }

  rtdText = &text->data.text;

  *utfEncoding            = (rtdText->status >> NDEF_TEXT_ENCODING_SHIFT) & 1U;

  bufLanguageCode->buffer = rtdText->bufLanguageCode.buffer;
  bufLanguageCode->length = rtdText->bufLanguageCode.length;

  bufSentence->buffer     = rtdText->bufSentence.buffer;
  bufSentence->length     = rtdText->bufSentence.length;

  return ERR_NONE;
}


/*****************************************************************************/
static ReturnCode ndefPayloadToRtdText(const ndefConstBuffer *bufText, ndefType *text)
{
  ndefTypeRtdText *rtdText;
  uint8_t status;
  uint8_t languageCodeLength;

  if ((bufText == NULL) || (bufText->buffer == NULL) || (bufText->length == 0U) ||
      (text    == NULL)) {
    return ERR_PARAM;
  }

  if (bufText->length < NDEF_RTD_TEXT_PAYLOAD_LENGTH_MIN) { /* Text Payload Min */
    return ERR_PROTO;
  }

  text->id               = NDEF_TYPE_ID_RTD_TEXT;
  text->getPayloadLength = ndefRtdTextPayloadGetLength;
  text->getPayloadItem   = ndefRtdTextToPayloadItem;
  text->typeToRecord     = ndefRtdTextToRecord;
  rtdText                = &text->data.text;

  /* Extract info from the payload */
  status = bufText->buffer[NDEF_RTD_TEXT_STATUS_OFFSET];

  rtdText->status = status;

  /* Extract info from the status byte */
  //uint8_t textUtfEncoding          = (status & NDEF_TEXT_ENCODING_MASK) >> NDEF_TEXT_ENCODING_SHIFT;
  languageCodeLength = (status & NDEF_RTD_TEXT_LANGUAGE_CODE_LEN_MASK);

  rtdText->bufLanguageCode.buffer = &(bufText->buffer[NDEF_RTD_TEXT_LANGUAGE_OFFSET]);
  rtdText->bufLanguageCode.length = languageCodeLength;

  rtdText->bufSentence.buffer = &(bufText->buffer[NDEF_RTD_TEXT_LANGUAGE_OFFSET + languageCodeLength]);
  rtdText->bufSentence.length = bufText->length - sizeof(status) - languageCodeLength;

  return ERR_NONE;
}


/*****************************************************************************/
ReturnCode ndefRecordToRtdText(const ndefRecord *record, ndefType *text)
{
  const ndefType *type;

  if ((record == NULL) || (text == NULL)) {
    return ERR_PARAM;
  }

  if (! ndefRecordTypeMatch(record, NDEF_TNF_RTD_WELL_KNOWN_TYPE, &bufRtdTypeText)) { /* "T" */
    return ERR_PROTO;
  }

  type = ndefRecordGetNdefType(record);
  if ((type != NULL) && (type->id == NDEF_TYPE_ID_RTD_TEXT)) {
    (void)ST_MEMCPY(text, type, sizeof(ndefType));
    return ERR_NONE;
  }

  return ndefPayloadToRtdText(&record->bufPayload, text);
}


/*****************************************************************************/
ReturnCode ndefRtdTextToRecord(const ndefType *text, ndefRecord *record)
{
  if ((text   == NULL) || (text->id != NDEF_TYPE_ID_RTD_TEXT) ||
      (record == NULL)) {
    return ERR_PARAM;
  }

  (void)ndefRecordReset(record);

  /* "T" */
  (void)ndefRecordSetType(record, NDEF_TNF_RTD_WELL_KNOWN_TYPE, &bufRtdTypeText);

  if (ndefRecordSetNdefType(record, text) != ERR_NONE) {
    return ERR_PARAM;
  }

  return ERR_NONE;
}

#endif
