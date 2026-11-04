
/**
  ******************************************************************************
  * @file           : ndef_type_aar.cpp
  * @brief          : NDEF RTD Android Application Record (AAR) type
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
#include "ndef_type_aar.h"
#include "nfc_utils.h"


#if NDEF_TYPE_RTD_AAR_SUPPORT


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
static const uint8_t ndefRtdTypeAar[]            = "android.com:pkg"; /*!< External Type (Android Application Record)  */

const ndefConstBuffer8 bufRtdTypeAar             = { ndefRtdTypeAar,        sizeof(ndefRtdTypeAar) - 1U };        /*!< AAR External Type Record buffer       */


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
 * NFC Forum External Type (Android Application Record)
 */


/*****************************************************************************/
ReturnCode ndefRtdAarInit(ndefType *aar, const ndefConstBuffer *bufPayload)
{
  ndefTypeRtdAar *rtdAar;

  if ((aar == NULL) || (bufPayload == NULL)) {
    return ERR_PARAM;
  }

  aar->id               = NDEF_TYPE_ID_RTD_AAR;
  aar->getPayloadLength = NULL;
  aar->getPayloadItem   = NULL;
  aar->typeToRecord     = ndefRtdAarToRecord;
  rtdAar                = &aar->data.aar;

  rtdAar->bufType.buffer    = bufRtdTypeAar.buffer;
  rtdAar->bufType.length    = bufRtdTypeAar.length;
  rtdAar->bufPayload.buffer = bufPayload->buffer;
  rtdAar->bufPayload.length = bufPayload->length;

  return ERR_NONE;
}


/*****************************************************************************/
ReturnCode ndefGetRtdAar(const ndefType *aar, ndefConstBuffer *bufAarString)
{
  const ndefTypeRtdAar *rtdAar;

  if ((aar          == NULL) || (aar->id != NDEF_TYPE_ID_RTD_AAR) ||
      (bufAarString == NULL)) {
    return ERR_PARAM;
  }

  rtdAar = &aar->data.aar;

  bufAarString->buffer = rtdAar->bufPayload.buffer;
  bufAarString->length = rtdAar->bufPayload.length;

  return ERR_NONE;
}


/*****************************************************************************/
ReturnCode ndefRecordToRtdAar(const ndefRecord *record, ndefType *aar)
{
  if ((record == NULL) || (aar == NULL)) {
    return ERR_PARAM;
  }

  if (! ndefRecordTypeMatch(record, NDEF_TNF_RTD_EXTERNAL_TYPE, &bufRtdTypeAar)) { /* "android.com:pkg" */
    return ERR_PROTO;
  }

  /* No constraint on payload length */

  return ndefRtdAarInit(aar, &record->bufPayload);
}


/*****************************************************************************/
ReturnCode ndefRtdAarToRecord(const ndefType *aar, ndefRecord *record)
{
  const ndefTypeRtdAar *rtdAar;

  if ((aar    == NULL) || (aar->id != NDEF_TYPE_ID_RTD_AAR) ||
      (record == NULL)) {
    return ERR_PARAM;
  }

  rtdAar = &aar->data.aar;

  (void)ndefRecordReset(record);

  /* "android.com:pkg" */
  (void)ndefRecordSetType(record, NDEF_TNF_RTD_EXTERNAL_TYPE, &bufRtdTypeAar);

  (void)ndefRecordSetPayload(record, &rtdAar->bufPayload);

  return ERR_NONE;
}

#endif
