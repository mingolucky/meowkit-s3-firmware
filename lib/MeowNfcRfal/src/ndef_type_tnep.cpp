
/**
  ******************************************************************************
  * @file           : ndef_type_uri.cpp
  * @brief          : NDEF TNEP (Tag NDEF Exchange Protocol record) types
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

#include <math.h>
#include "ndef_record.h"
#include "ndef_types.h"
#include "ndef_type_tnep.h"
#include "nfc_utils.h"


#if NDEF_TYPE_RTD_TNEP_SUPPORT

/*
 ******************************************************************************
 * GLOBAL DEFINES
 ******************************************************************************
 */


/*! TNEP defines */
#define NDEF_TNEP_SERVICE_NAME_URI_LENGTH_LENGTH            sizeof(uint8_t) /*!< Service Name URI length's length */

/*! TNEP Service Parameter defines */
#define NDEF_RTD_TNEP_SP_VERSION_OFFSET                     0U    /*!< TNEP Service Parameter TNEP Version offset                      */
#define NDEF_RTD_TNEP_SP_SERVICE_URI_LENGTH_OFFSET          1U    /*!< TNEP Service Parameter URI length offset                        */
#define NDEF_RTD_TNEP_SP_SERVICE_URI_OFFSET                 2U    /*!< TNEP Service Parameter URI offset                               */
#define NDEF_RTD_TNEP_SP_SERVICE_URI_LENGTH_MAX             255U  /*!< TNEP Service Parameter URI max length (bytes)                   */
#define NDEF_RTD_TNEP_SP_COMMUNICATION_MODE_OFFSET          2U    /*!< TNEP Service Parameter Communication Mode offset                */
#define NDEF_RTD_TNEP_SP_MIN_WAITING_TIME_OFFSET            3U    /*!< TNEP Service Parameter Minimum Waiting Time offset              */
#define NDEF_RTD_TNEP_SP_MAX_WT_EXTENSIONS_OFFSET           4U    /*!< TNEP Service Parameter Maximum Waiting Time Extensions offset   */
#define NDEF_RTD_TNEP_SP_MAX_MESSAGE_SIZE_OFFSET            5U    /*!< TNEP Service Parameter Maximum NDEF Message Size (bytes) offset */
#define NDEF_RTD_TNEP_SP_MINIMUM_LENGTH                     8U    /*!< TNEP Service Parameter minimum length (bytes)                   */

/*! TNEP Service Select defines */
#define NDEF_RTD_TNEP_SS_SERVICE_URI_LENGTH_OFFSET          0U    /*!< TNEP Service Select URI length offset                       */
#define NDEF_RTD_TNEP_SS_SERVICE_URI_OFFSET                 1U    /*!< TNEP Service Select URI offset                              */
#define NDEF_RTD_TNEP_SS_MINIMUM_LENGTH                     2U    /*!< TNEP Service Select minimum length (bytes)                  */

/*! TNEP Service Select defines */
#define NDEF_RTD_TNEP_STATUS_TYPE_OFFSET                    0U    /*!< TNEP Status Type offset                       */
#define NDEF_RTD_TNEP_STATUS_MINIMUM_LENGTH                 1U    /*!< TNEP Status minimum length (bytes)            */


/*
 ******************************************************************************
 * LOCAL VARIABLES
 ******************************************************************************
 */


/*! RTD TNEP Type strings */
static const uint8_t ndefRtdTypeTnepServiceParameter[] = "Tp";              /*!< Tnep Service Parameter Record Type          */
static const uint8_t ndefRtdTypeTnepServiceSelect[]    = "Ts";              /*!< Tnep Service Select Record Type             */
static const uint8_t ndefRtdTypeTnepStatus[]           = "Te";              /*!< Tnep Status Record Type                     */

const ndefConstBuffer8 bufRtdTypeTnepServiceParameter = { ndefRtdTypeTnepServiceParameter, sizeof(ndefRtdTypeTnepServiceParameter) - 1U }; /*!< TNEP Service Parameter Type Record buffer       */
const ndefConstBuffer8 bufRtdTypeTnepServiceSelect    = { ndefRtdTypeTnepServiceSelect,    sizeof(ndefRtdTypeTnepServiceSelect) - 1U };    /*!< TNEP Service Select Type Record buffer       */
const ndefConstBuffer8 bufRtdTypeTnepStatus           = { ndefRtdTypeTnepStatus,           sizeof(ndefRtdTypeTnepStatus) - 1U };           /*!< TNEP Status Type Record buffer       */


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
 * TNEP Service Parameter
 */

/*****************************************************************************/
static uint32_t ndefRtdTnepServiceParameterGetPayloadLength(const ndefType *type)
{
  const ndefTypeRtdTnepServiceParameter *rtdServiceParameter;

  if ((type == NULL) || (type->id != NDEF_TYPE_ID_RTD_TNEP_SERVICE_PARAMETER)) {
    return 0;
  }

  rtdServiceParameter = &type->data.tnepServiceParameter;

  return sizeof(rtdServiceParameter->tnepVersion)
         + NDEF_TNEP_SERVICE_NAME_URI_LENGTH_LENGTH
         + rtdServiceParameter->bufServiceNameUri.length
         + sizeof(rtdServiceParameter->communicationMode)
         + sizeof(rtdServiceParameter->minimumWaitingTime)
         + sizeof(rtdServiceParameter->maximumWaitingTimeExtensions)
         + sizeof(rtdServiceParameter->maximumNdefMessageSize);
}


/*****************************************************************************/
static const uint8_t *ndefRtdTnepServiceParameterToPayloadItem(const ndefType *type, ndefConstBuffer *bufItem, bool begin)
{
  static uint32_t item = 0;
  const ndefTypeRtdTnepServiceParameter *rtdServiceParameter;

  if ((type    == NULL) || (type->id != NDEF_TYPE_ID_RTD_TNEP_SERVICE_PARAMETER)
      || (bufItem == NULL)) {
    return NULL;
  }

  rtdServiceParameter = &type->data.tnepServiceParameter;

  if (begin == true) {
    item = 0;
  }

  switch (item) {
    case 0:
      /* TNEP version byte */
      bufItem->buffer = &rtdServiceParameter->tnepVersion;
      bufItem->length = sizeof(rtdServiceParameter->tnepVersion);
      break;

    case 1:
      /* Service URI length byte */
      bufItem->buffer = (const uint8_t *) & (rtdServiceParameter->bufServiceNameUri.length);
      bufItem->length = NDEF_TNEP_SERVICE_NAME_URI_LENGTH_LENGTH;
      break;

    case 2:
      /* Service URI string */
      bufItem->buffer = rtdServiceParameter->bufServiceNameUri.buffer;
      bufItem->length = rtdServiceParameter->bufServiceNameUri.length;
      break;

    case 3:
      /* TNEP communication Mode byte */
      bufItem->buffer = &rtdServiceParameter->communicationMode;
      bufItem->length = sizeof(rtdServiceParameter->communicationMode);
      break;

    case 4:
      /* Minimum waiting time byte */
      bufItem->buffer = &rtdServiceParameter->minimumWaitingTime;
      bufItem->length = sizeof(rtdServiceParameter->minimumWaitingTime);
      break;

    case 5:
      /* Maximum waiting time extensions byte */
      bufItem->buffer = &rtdServiceParameter->maximumWaitingTimeExtensions;
      bufItem->length = sizeof(rtdServiceParameter->maximumWaitingTimeExtensions);
      break;

    case 6:
      /* Maximum NDEF message size */
      bufItem->buffer = rtdServiceParameter->maximumNdefMessageSize;
      bufItem->length = sizeof(rtdServiceParameter->maximumNdefMessageSize);
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


#ifdef NDEF_PROVIDE_HELPER_FUNCTIONS
/*****************************************************************************/
uint8_t ndefRtdTnepServiceParameterComputeWtInt(float twait)
{
  return ceil(4 * ((log(twait) / 0.69314) + 1));
}


/*****************************************************************************/
float ndefRtdTnepServiceParameterComputeTwait(uint8_t wtInt)
{
  return powf(2, (((float)wtInt / 4) - 1));
}
#endif


/*****************************************************************************/
ReturnCode ndefRtdTnepServiceParameterInit(ndefType *type, uint8_t tnepVersion, const ndefConstBuffer *bufServiceUri, uint8_t comMode, uint8_t minWaitingTime, uint8_t maxExtensions, uint16_t maxMessageSize)
{
  ndefTypeRtdTnepServiceParameter *rtdServiceParameter;

  if ((type == NULL)    || (bufServiceUri == NULL)
      || (bufServiceUri->buffer == NULL) || (bufServiceUri->length == 0U)
      || (bufServiceUri->length > (NDEF_RTD_TNEP_SP_SERVICE_URI_LENGTH_MAX))) {
    return ERR_PARAM;
  }

  type->id               = NDEF_TYPE_ID_RTD_TNEP_SERVICE_PARAMETER;
  type->getPayloadLength = ndefRtdTnepServiceParameterGetPayloadLength;
  type->getPayloadItem   = ndefRtdTnepServiceParameterToPayloadItem;
  type->typeToRecord     = ndefRtdTnepServiceParameterToRecord;
  rtdServiceParameter    = &type->data.tnepServiceParameter;

  rtdServiceParameter->tnepVersion                  = tnepVersion;
  rtdServiceParameter->bufServiceNameUri.length     = bufServiceUri->length;
  rtdServiceParameter->bufServiceNameUri.buffer     = bufServiceUri->buffer;
  rtdServiceParameter->communicationMode            = comMode;
  rtdServiceParameter->minimumWaitingTime           = minWaitingTime;
  rtdServiceParameter->maximumWaitingTimeExtensions = maxExtensions;
  rtdServiceParameter->maximumNdefMessageSize[0]    = (uint8_t)(maxMessageSize >> 8U);
  rtdServiceParameter->maximumNdefMessageSize[1]    = (uint8_t)(maxMessageSize & 0xFFU);

  return ERR_NONE;
}


/*****************************************************************************/
ReturnCode ndefGetRtdTnepServiceParameter(const ndefType *type, uint8_t *tnepVersion, ndefConstBuffer *bufServiceUri, uint8_t *comMode, uint8_t *minWaitingTime, uint8_t *maxExtensions, uint16_t *maxMessageSize)
{
  const ndefTypeRtdTnepServiceParameter *rtdServiceParameter;

  if ((type == NULL) || (type->id != NDEF_TYPE_ID_RTD_TNEP_SERVICE_PARAMETER)
      || (bufServiceUri  == NULL) || (tnepVersion   == NULL) || (comMode        == NULL)
      || (minWaitingTime == NULL) || (maxExtensions == NULL) || (maxMessageSize == NULL)) {
    return ERR_PARAM;
  }

  rtdServiceParameter = &type->data.tnepServiceParameter;

  *tnepVersion          = rtdServiceParameter->tnepVersion;
  bufServiceUri->buffer = rtdServiceParameter->bufServiceNameUri.buffer;
  bufServiceUri->length = rtdServiceParameter->bufServiceNameUri.length;
  *comMode              = rtdServiceParameter->communicationMode;
  *minWaitingTime       = rtdServiceParameter->minimumWaitingTime;
  *maxExtensions        = rtdServiceParameter->maximumWaitingTimeExtensions;
  *maxMessageSize       = GETU16(rtdServiceParameter->maximumNdefMessageSize);

  return ERR_NONE;
}


/*****************************************************************************/
static ReturnCode ndefPayloadToRtdTnepServiceParameter(const ndefConstBuffer *bufTnepServiceParameter, ndefType *type)
{
  uint8_t tnepVersion;
  ndefConstBuffer bufServiceUri;
  uint8_t commMode;
  uint8_t minWaitingTime;
  uint8_t maxWaitingTimeExtensions;
  uint16_t maxNdefMessageSize;
  uint32_t maxNdefMessageSizeOffset;

  if ((bufTnepServiceParameter == NULL) || (bufTnepServiceParameter->buffer == NULL) || (bufTnepServiceParameter->length < (NDEF_RTD_TNEP_SP_MINIMUM_LENGTH)) ||
      (type                    == NULL)) {
    return ERR_PARAM;
  }

  /* Extract info from the payload */
  tnepVersion              = bufTnepServiceParameter->buffer[NDEF_RTD_TNEP_SP_VERSION_OFFSET];
  bufServiceUri.buffer     = &bufTnepServiceParameter->buffer[NDEF_RTD_TNEP_SP_SERVICE_URI_OFFSET];
  bufServiceUri.length     = bufTnepServiceParameter->buffer[NDEF_RTD_TNEP_SP_SERVICE_URI_LENGTH_OFFSET];
  commMode                 = bufTnepServiceParameter->buffer[NDEF_RTD_TNEP_SP_COMMUNICATION_MODE_OFFSET + bufServiceUri.length];
  minWaitingTime           = bufTnepServiceParameter->buffer[NDEF_RTD_TNEP_SP_MIN_WAITING_TIME_OFFSET + bufServiceUri.length];
  maxWaitingTimeExtensions = bufTnepServiceParameter->buffer[NDEF_RTD_TNEP_SP_MAX_WT_EXTENSIONS_OFFSET + bufServiceUri.length];
  maxNdefMessageSizeOffset = NDEF_RTD_TNEP_SP_MAX_MESSAGE_SIZE_OFFSET + bufServiceUri.length;
  maxNdefMessageSize       = GETU16(&bufTnepServiceParameter->buffer[maxNdefMessageSizeOffset]);

  return ndefRtdTnepServiceParameterInit(type, tnepVersion, &bufServiceUri, commMode, minWaitingTime, maxWaitingTimeExtensions, maxNdefMessageSize);
}


/*****************************************************************************/
ReturnCode ndefRecordToRtdTnepServiceParameter(const ndefRecord *record, ndefType *type)
{
  const ndefType *ndefData;

  if ((record == NULL) || (type == NULL)) {
    return ERR_PARAM;
  }

  if (! ndefRecordTypeMatch(record, NDEF_TNF_RTD_WELL_KNOWN_TYPE, &bufRtdTypeTnepServiceParameter)) { /* "Tp" */
    return ERR_PROTO;
  }

  ndefData = ndefRecordGetNdefType(record);
  if ((ndefData != NULL) && (ndefData->id == NDEF_TYPE_ID_RTD_TNEP_SERVICE_PARAMETER)) {
    (void)ST_MEMCPY(type, ndefData, sizeof(ndefType));
    return ERR_NONE;
  }

  if (record->bufPayload.length < NDEF_RTD_TNEP_SP_MINIMUM_LENGTH) {
    return ERR_PROTO;
  }

  return ndefPayloadToRtdTnepServiceParameter(&record->bufPayload, type);
}


/*****************************************************************************/
ReturnCode ndefRtdTnepServiceParameterToRecord(const ndefType *type, ndefRecord *record)
{
  if ((type   == NULL) || (type->id != NDEF_TYPE_ID_RTD_TNEP_SERVICE_PARAMETER) ||
      (record == NULL)) {
    return ERR_PARAM;
  }

  (void)ndefRecordReset(record);

  /* "Tp" */
  (void)ndefRecordSetType(record, NDEF_TNF_RTD_WELL_KNOWN_TYPE, &bufRtdTypeTnepServiceParameter);

  (void)ndefRecordSetNdefType(record, type);

  return ERR_NONE;
}


/*
 * TNEP Service Select
 */


/*****************************************************************************/
static uint32_t ndefRtdTnepServiceSelectGetPayloadLength(const ndefType *type)
{
  const ndefTypeRtdTnepServiceSelect *rtdServiceSelect;

  if ((type == NULL) || (type->id != NDEF_TYPE_ID_RTD_TNEP_SERVICE_SELECT)) {
    return 0;
  }

  rtdServiceSelect = &type->data.tnepServiceSelect;

  return NDEF_TNEP_SERVICE_NAME_URI_LENGTH_LENGTH + rtdServiceSelect->bufServiceNameUri.length;
}


/*****************************************************************************/
static const uint8_t *ndefRtdTnepServiceSelectToPayloadItem(const ndefType *type, ndefConstBuffer *bufItem, bool begin)
{
  static uint32_t item = 0;
  const ndefTypeRtdTnepServiceSelect *rtdServiceSelect;

  if ((type    == NULL) || (type->id != NDEF_TYPE_ID_RTD_TNEP_SERVICE_SELECT)
      || (bufItem == NULL)) {
    return NULL;
  }

  rtdServiceSelect = &type->data.tnepServiceSelect;

  if (begin == true) {
    item = 0;
  }

  switch (item) {
    case 0:
      /* Service URI length byte */
      bufItem->buffer = (const uint8_t *)&rtdServiceSelect->bufServiceNameUri.length;
      bufItem->length = NDEF_TNEP_SERVICE_NAME_URI_LENGTH_LENGTH;
      break;

    case 1:
      /* Service URI string */
      bufItem->buffer = rtdServiceSelect->bufServiceNameUri.buffer;
      bufItem->length = rtdServiceSelect->bufServiceNameUri.length;
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
ReturnCode ndefRtdTnepServiceSelectInit(ndefType *type, const ndefConstBuffer *bufServiceUri)
{
  ndefTypeRtdTnepServiceSelect *rtdServiceSelect;

  if ((type == NULL)       || (bufServiceUri == NULL)
      || (bufServiceUri->buffer == NULL) || (bufServiceUri->length == 0U)
      || (bufServiceUri->length > (NDEF_RTD_TNEP_SP_SERVICE_URI_LENGTH_MAX))) {
    return ERR_PARAM;
  }

  type->id               = NDEF_TYPE_ID_RTD_TNEP_SERVICE_SELECT;
  type->getPayloadLength = ndefRtdTnepServiceSelectGetPayloadLength;
  type->getPayloadItem   = ndefRtdTnepServiceSelectToPayloadItem;
  type->typeToRecord     = ndefRtdTnepServiceSelectToRecord;
  rtdServiceSelect       = &type->data.tnepServiceSelect;

  rtdServiceSelect->bufServiceNameUri.length = bufServiceUri->length;
  rtdServiceSelect->bufServiceNameUri.buffer = bufServiceUri->buffer;

  return ERR_NONE;
}


/*****************************************************************************/
ReturnCode ndefGetRtdTnepServiceSelect(const ndefType *type, ndefConstBuffer *bufServiceUri)
{
  const ndefTypeRtdTnepServiceSelect *rtdServiceSelect;

  if ((type          == NULL) || (type->id != NDEF_TYPE_ID_RTD_TNEP_SERVICE_SELECT) ||
      (bufServiceUri == NULL)) {
    return ERR_PARAM;
  }

  rtdServiceSelect = &type->data.tnepServiceSelect;

  bufServiceUri->buffer  = rtdServiceSelect->bufServiceNameUri.buffer;
  bufServiceUri->length  = rtdServiceSelect->bufServiceNameUri.length;

  return ERR_NONE;
}


/*****************************************************************************/
static ReturnCode ndefPayloadToRtdTnepServiceSelect(const ndefConstBuffer *bufTnepServiceSelect, ndefType *type)
{
  ndefConstBuffer bufServiceUri;

  if ((bufTnepServiceSelect == NULL) || (bufTnepServiceSelect->buffer == NULL) || (bufTnepServiceSelect->length < (NDEF_RTD_TNEP_SS_MINIMUM_LENGTH)) ||
      (type                 == NULL)) {
    return ERR_PARAM;
  }

  /* Extract info from the payload */
  bufServiceUri.buffer         = &bufTnepServiceSelect->buffer[NDEF_RTD_TNEP_SS_SERVICE_URI_OFFSET];
  bufServiceUri.length         = bufTnepServiceSelect->buffer[NDEF_RTD_TNEP_SS_SERVICE_URI_LENGTH_OFFSET];

  return ndefRtdTnepServiceSelectInit(type, &bufServiceUri);
}


/*****************************************************************************/
ReturnCode ndefRecordToRtdTnepServiceSelect(const ndefRecord *record, ndefType *type)
{
  const ndefType *ndefData;

  if ((record == NULL) || (type == NULL)) {
    return ERR_PARAM;
  }

  if (! ndefRecordTypeMatch(record, NDEF_TNF_RTD_WELL_KNOWN_TYPE, &bufRtdTypeTnepServiceSelect)) { /* "Ts" */
    return ERR_PROTO;
  }

  ndefData = ndefRecordGetNdefType(record);
  if ((ndefData != NULL) && (ndefData->id == NDEF_TYPE_ID_RTD_TNEP_SERVICE_SELECT)) {
    (void)ST_MEMCPY(type, ndefData, sizeof(ndefType));
    return ERR_NONE;
  }

  if (record->bufPayload.length < NDEF_RTD_TNEP_SS_MINIMUM_LENGTH) {
    return ERR_PROTO;
  }

  return ndefPayloadToRtdTnepServiceSelect(&record->bufPayload, type);
}


/*****************************************************************************/
ReturnCode ndefRtdTnepServiceSelectToRecord(const ndefType *type, ndefRecord *record)
{
  if ((type   == NULL) || (type->id != NDEF_TYPE_ID_RTD_TNEP_SERVICE_SELECT) ||
      (record == NULL)) {
    return ERR_PARAM;
  }

  (void)ndefRecordReset(record);

  /* "Ts" */
  (void)ndefRecordSetType(record, NDEF_TNF_RTD_WELL_KNOWN_TYPE, &bufRtdTypeTnepServiceSelect);

  (void)ndefRecordSetNdefType(record, type);

  return ERR_NONE;
}


/*
 * TNEP Status
 */


/*****************************************************************************/
static uint32_t ndefRtdTnepStatusGetPayloadLength(const ndefType *type)
{
  if ((type == NULL) || (type->id != NDEF_TYPE_ID_RTD_TNEP_STATUS)) {
    return 0;
  }

  return sizeof(type->data.tnepStatus.statusType);
}


/*****************************************************************************/
static const uint8_t *ndefRtdTnepStatusToPayloadItem(const ndefType *type, ndefConstBuffer *bufItem, bool begin)
{
  static uint32_t item = 0;
  const ndefTypeRtdTnepStatus *rtdStatus;

  if ((type == NULL) || (type->id != NDEF_TYPE_ID_RTD_TNEP_STATUS)
      || (bufItem == NULL)) {
    return NULL;
  }

  rtdStatus = &type->data.tnepStatus;

  if (begin == true) {
    item = 0;
  }

  switch (item) {
    case 0:
      /* Status type byte */
      bufItem->buffer = &rtdStatus->statusType;
      bufItem->length = sizeof(rtdStatus->statusType);
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
ReturnCode ndefRtdTnepStatusInit(ndefType *type, uint8_t statusType)
{
  ndefTypeRtdTnepStatus *rtdStatus;

  if (type == NULL) {
    return ERR_PARAM;
  }

  type->id               = NDEF_TYPE_ID_RTD_TNEP_STATUS;
  type->getPayloadLength = ndefRtdTnepStatusGetPayloadLength;
  type->getPayloadItem   = ndefRtdTnepStatusToPayloadItem;
  type->typeToRecord     = ndefRtdTnepStatusToRecord;
  rtdStatus              = &type->data.tnepStatus;

  rtdStatus->statusType = statusType;

  return ERR_NONE;
}


/*****************************************************************************/
ReturnCode ndefGetRtdTnepStatus(const ndefType *type, uint8_t *statusType)
{
  const ndefTypeRtdTnepStatus *rtdStatus;

  if ((type       == NULL) || (type->id != NDEF_TYPE_ID_RTD_TNEP_STATUS) ||
      (statusType == NULL)) {
    return ERR_PARAM;
  }

  rtdStatus = &type->data.tnepStatus;

  *statusType = rtdStatus->statusType;

  return ERR_NONE;
}


/*****************************************************************************/
static ReturnCode ndefPayloadToRtdTnepStatus(const ndefConstBuffer *bufTnepStatus, ndefType *type)
{
  uint8_t statusType;

  if ((bufTnepStatus == NULL) || (bufTnepStatus->buffer == NULL) || (bufTnepStatus->length < (NDEF_RTD_TNEP_STATUS_MINIMUM_LENGTH)) ||
      (type          == NULL)) {
    return ERR_PARAM;
  }

  /* Extract info from the payload */
  statusType = bufTnepStatus->buffer[NDEF_RTD_TNEP_STATUS_TYPE_OFFSET];

  return ndefRtdTnepStatusInit(type, statusType);
}


/*****************************************************************************/
ReturnCode ndefRecordToRtdTnepStatus(const ndefRecord *record, ndefType *type)
{
  const ndefType *ndefData;

  if ((record == NULL) || (type == NULL)) {
    return ERR_PARAM;
  }

  if (! ndefRecordTypeMatch(record, NDEF_TNF_RTD_WELL_KNOWN_TYPE, &bufRtdTypeTnepStatus)) { /* "Te" */
    return ERR_PROTO;
  }

  ndefData = ndefRecordGetNdefType(record);
  if ((ndefData != NULL) && (ndefData->id == NDEF_TYPE_ID_RTD_TNEP_STATUS)) {
    (void)ST_MEMCPY(type, ndefData, sizeof(ndefType));
    return ERR_NONE;
  }

  if (record->bufPayload.length < NDEF_RTD_TNEP_STATUS_MINIMUM_LENGTH) {
    return ERR_PROTO;
  }

  return ndefPayloadToRtdTnepStatus(&record->bufPayload, type);
}


/*****************************************************************************/
ReturnCode ndefRtdTnepStatusToRecord(const ndefType *type, ndefRecord *record)
{
  if ((type   == NULL) || (type->id != NDEF_TYPE_ID_RTD_TNEP_STATUS) ||
      (record == NULL)) {
    return ERR_PARAM;
  }

  (void)ndefRecordReset(record);

  /* "Te" */
  (void)ndefRecordSetType(record, NDEF_TNF_RTD_WELL_KNOWN_TYPE, &bufRtdTypeTnepStatus);

  (void)ndefRecordSetNdefType(record, type);

  return ERR_NONE;
}

#endif
