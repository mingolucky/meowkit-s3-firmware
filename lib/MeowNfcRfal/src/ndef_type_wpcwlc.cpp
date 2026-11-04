
/**
  ******************************************************************************
  * @file           : ndef_type_wpcwlc.cpp
  * @brief          :NDEF RTD RTD Wireless Power Consortium WLC Record (WPCWLC) type
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
#include "ndef_type_wpcwlc.h"
#include "nfc_utils.h"


#if NDEF_TYPE_RTD_WPCWLC_SUPPORT


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


/*! RTD Type strings */
static const uint8_t ndefRtdTypeWptWlc[] = "www.wirelesspowerconsortium.com:wlc"; /*!< External Type (Wireless Power Consortium WLC  Record)  */

const ndefConstBuffer8 bufRtdTypeWpcWlc  = { ndefRtdTypeWptWlc, sizeof(ndefRtdTypeWptWlc) - 1U };    /*!< WPCWLC External Type Record buffer  */


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
 * NFC Forum External Type (Wireless Power Consortium WLC Record)
 */


/*****************************************************************************/
static uint32_t ndefRtdWpcWlcPayloadGetLength(const ndefType *wpcWlc)
{
  const ndefTypeRtdWpcWlc *rtdWpcWlc;

  if ((wpcWlc == NULL) || (wpcWlc->id != NDEF_TYPE_ID_RTD_WPCWLC)) {
    return 0;
  }

  rtdWpcWlc = &wpcWlc->data.wpcWlc;

  return rtdWpcWlc->bufPayload.length;
}


/*****************************************************************************/
static const uint8_t *ndefRtdWpcWlcToPayloadItem(const ndefType *wpcWlc, ndefConstBuffer *bufItem, bool begin)
{
  static uint32_t item = 0;
  const ndefTypeRtdWpcWlc *rtdWpcWlc;

  if ((wpcWlc  == NULL) || (wpcWlc->id != NDEF_TYPE_ID_RTD_WPCWLC) ||
      (bufItem == NULL)) {
    return NULL;
  }

  rtdWpcWlc = &wpcWlc->data.wpcWlc;

  if (begin == true) {
    item = 0;
  }

  switch (item) {
    case 0:
      /* Ki Payload */
      bufItem->buffer = rtdWpcWlc->bufPayload.buffer;
      bufItem->length = rtdWpcWlc->bufPayload.length;
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
ReturnCode ndefRtdWpcWlcInit(ndefType *wpcWlc, const ndefConstBuffer *bufPayload)
{
  ndefTypeRtdWpcWlc *rtdWpcWlc;

  if ((wpcWlc == NULL) || (bufPayload == NULL)) {
    return ERR_PARAM;
  }

  wpcWlc->id               = NDEF_TYPE_ID_RTD_WPCWLC;
  wpcWlc->getPayloadLength = ndefRtdWpcWlcPayloadGetLength;
  wpcWlc->getPayloadItem   = ndefRtdWpcWlcToPayloadItem;
  wpcWlc->typeToRecord     = ndefRtdWpcWlcToRecord;
  rtdWpcWlc                = &wpcWlc->data.wpcWlc;

  rtdWpcWlc->bufPayload.buffer = bufPayload->buffer;
  rtdWpcWlc->bufPayload.length = bufPayload->length;

  return ERR_NONE;
}


/*****************************************************************************/
ReturnCode ndefGetRtdWpcWlc(const ndefType *wpcWlc, ndefConstBuffer *bufWpcWlc)
{
  const ndefTypeRtdWpcWlc *rtdWpcWlc;

  if ((wpcWlc    == NULL) || (wpcWlc->id != NDEF_TYPE_ID_RTD_WPCWLC) ||
      (bufWpcWlc == NULL)) {
    return ERR_PARAM;
  }

  rtdWpcWlc = &wpcWlc->data.wpcWlc;

  bufWpcWlc->buffer = rtdWpcWlc->bufPayload.buffer;
  bufWpcWlc->length = rtdWpcWlc->bufPayload.length;

  return ERR_NONE;
}


/*****************************************************************************/
ReturnCode ndefRecordToRtdWpcWlc(const ndefRecord *record, ndefType *wpcWlc)
{
  if ((record == NULL) || (wpcWlc == NULL)) {
    return ERR_PARAM;
  }

  if (! ndefRecordTypeMatch(record, NDEF_TNF_RTD_EXTERNAL_TYPE, &bufRtdTypeWpcWlc)) { /* "www.wirelesspowerconsortium.com:wlc" */
    return ERR_PROTO;
  }

  /* No constraint on payload length */

  return ndefRtdWpcWlcInit(wpcWlc, &record->bufPayload);
}


/*****************************************************************************/
ReturnCode ndefRtdWpcWlcToRecord(const ndefType *wpcWlc, ndefRecord *record)
{
  if ((wpcWlc == NULL) || (wpcWlc->id != NDEF_TYPE_ID_RTD_WPCWLC) ||
      (record == NULL)) {
    return ERR_PARAM;
  }

  (void)ndefRecordReset(record);

  /* "www.wirelesspowerconsortium.com:wlc" */
  (void)ndefRecordSetType(record, NDEF_TNF_RTD_EXTERNAL_TYPE, &bufRtdTypeWpcWlc);

  if (ndefRecordSetNdefType(record, wpcWlc) != ERR_NONE) {
    return ERR_PARAM;
  }

  return ERR_NONE;
}

#endif
