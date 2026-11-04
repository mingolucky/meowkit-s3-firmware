
/**
  ******************************************************************************
  * @file           : ndef_type_wlc.cpp
  * @brief          : NDEF WLC (Wireless Charging) types
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
#include "ndef_type_wlc.h"
#include "nfc_utils.h"


#if NDEF_TYPE_RTD_WLC_SUPPORT


/*
 ******************************************************************************
 * GLOBAL DEFINES
 ******************************************************************************
 */


#define NDEF_TYPE_RTD_WLC_PAYLOAD_LENGTH                    6U   /*!< WLC Capability/Poll Info/Listen Control Records Payload Length */

#define NDEF_TYPE_RTD_WLC_STATUS_INFO_MIN_PAYLOAD_LENGTH    1U   /*!< WLC Status Info Record Payload min length */
#define NDEF_TYPE_RTD_WLC_STATUS_INFO_MAX_PAYLOAD_LENGTH    9U   /*!< WLC Status Info Record Payload max length */


/* WLC Capability */
#define NDEF_WLC_CAPABILITY_PROTOCOL_VERSION_OFFSET         0U   /*!< WLC Capability Protocol Version Offset */
#define NDEF_WLC_CAPABILITY_CONFIG_OFFSET                   1U   /*!< WLC Capability Config Offset */
#define NDEF_WLC_CAPABILITY_WT_INT_OFFSET                   2U   /*!< WLC Capability WT INT Offset */
#define NDEF_WLC_CAPABILITY_NDEF_RD_WT_OFFSET               3U   /*!< WLC Capability NDEF RD WT Offset */
#define NDEF_WLC_CAPABILITY_NDEF_WR_TO_INT_OFFSET           4U   /*!< WLC Capability NDEF WR TO INT Offset */
#define NDEF_WLC_CAPABILITY_NDEF_WR_WT_INT_OFFSET           5U   /*!< WLC Capability NDEF WR WT INT Offset */

/* WLC Config: Protocol Version */
//#define NDEF_WLC_CAPABILITY_PROTOCOL_MAJOR_VERSION_SHIFT    4U  /*!< WLC Capability Protocol Major Version bit shift */
//#define NDEF_WLC_CAPABILITY_PROTOCOL_VERSION_MASK         0xFU  /*!< WLC Capability Protocol Major Version mask */

/* WLC Config: Mode Req */
#define NDEF_WLC_CAPABILITY_CONFIG_MODE_REQ_SHIFT           6U /*! WLC Capability Config Req Mode bit shift */
#define NDEF_WLC_CAPABILITY_CONFIG_MODE_REQ_MASK            3U /*! WLC Capability Config Req Mode mask */

/* WLC Config: Nego Wait Retry */
#define NDEF_WLC_CAPABILITY_CONFIG_WAIT_TIME_RETRY_SHIFT    2U /*! WLC Capability config Nego Wait Retry bit shift */
#define NDEF_WLC_CAPABILITY_CONFIG_WAIT_TIME_RETRY_MASK   0xFU /*! WLC Capability config Nego Wait Retry mask */

/* WLC Config: Nego Wait Retry flag */
#define NDEF_WLC_CAPABILITY_CONFIG_NEGO_WAIT_SHIFT          1U /*! WLC Capability config Nego Wait bit shift */
#define NDEF_WLC_CAPABILITY_CONFIG_NEGO_WAIT_MASK           1U /*! WLC Capability config Nego Wait mask */

/* WLC Config: Rd Conf flag */
#define NDEF_WLC_CAPABILITY_CONFIG_RD_CONF_SHIFT            0U /*! WLC Capability config Rd Conf bit shift */
#define NDEF_WLC_CAPABILITY_CONFIG_RD_CONF_MASK             1U /*! WLC Capability config Rd Conf mask */

/* WLC Config: CapWtInt RFU */
#define NDEF_WLC_CAPABILITY_CONFIG_CAP_WT_INT_RFU_SHIFT     5U /*! WLC Capability config CapWtInt RFU bit shift */
#define NDEF_WLC_CAPABILITY_CONFIG_CAP_WT_INT_RFU_MASK    0x7U /*! WLC Capability config CapWtInt RFU mask */

/* WLC Config: CapWtInt */
#define NDEF_WLC_CAPABILITY_CONFIG_CAP_WT_INT_SHIFT         0U /*! WLC Capability config CapWtInt bit shift */
#define NDEF_WLC_CAPABILITY_CONFIG_CAP_WT_INT_MASK       0x1FU /*! WLC Capability config CapWtInt mask */

//#define NDEF_WLC_CAPABILITY_CONFIG_CAP_WT_INT_MAX        0x13U /*! WLC Capability config CapWtInt max value */
//#define NDEF_WLC_CAPABILITY_CONFIG_CAP_WT_INT_MASK       0x1FU /*! WLC Capability config CapWtInt mask */

//#define NDEF_WLC_CAPABILITY_CONFIG_RD_CONF_MASK             1U /*! WLC Capability config Rd Conf mask */

/* WLC Status and Information */
#define NDEF_WLC_STATUSINFO_CONTROL_BYTE_1_OFFSET           0U /*! WLC Status and Info Control byte 1 offset */

/* WLC Poll Info */
#define NDEF_WLC_POLL_INFO_PTX_OFFSET                       0U   /*!< WLC Poll Info P Tx Offset */
#define NDEF_WLC_POLL_INFO_WLC_P_CAPABILITY_OFFSET          1U   /*!< WLC Poll Info WLC-P Capability Offset */
#define NDEF_WLC_POLL_INFO_POWER_CLASS_OFFSET               1U   /*!< WLC Poll Info Power Class Offset */
#define NDEF_WLC_POLL_INFO_TOT_POWER_STEPS_OFFSET           2U   /*!< WLC Poll Info Total Power Steps Offset */
#define NDEF_WLC_POLL_INFO_CUR_POWER_STEP_OFFSET            3U   /*!< WLC Poll Info Current Power Step Offset */
#define NDEF_WLC_POLL_INFO_NEXT_MIN_STEP_INC_OFFSET         4U   /*!< WLC Poll Info Next Min Step Inc Offset */
#define NDEF_WLC_POLL_INFO_NEXT_MIN_STEP_DEC_OFFSET         5U   /*!< WLC Poll Info Next Min Step Dec Offset */

#define NDEF_WLC_POLL_INFO_WLC_P_CAPABILITY_SHIFT           4U   /*!< WLC Poll Info WLC-P Capability bit shift */
#define NDEF_WLC_POLL_INFO_WLC_P_CAPABILITY_MASK          0xFU   /*!< WLC Poll Info WLC-P Capability mask */

/* WLC Listen Ctl */
#define NDEF_WLC_LISTEN_CTL_STATUS_INFO_OFFSET              0U   /*!< WLC Listen Control Status Info Offset */
#define NDEF_WLC_LISTEN_CTL_WPT_CONFIG_OFFSET               1U   /*!< WLC Listen Control WPT Config Offset */
#define NDEF_WLC_LISTEN_CTL_POWER_ADJ_REQ_OFFSET            2U   /*!< WLC Listen Control Power Adjust Request Offset */
#define NDEF_WLC_LISTEN_CTL_BATTERY_LEVEL_OFFSET            3U   /*!< WLC Listen Control Battery Level Offset */
#define NDEF_WLC_LISTEN_CTL_DRV_INFO_OFFSET                 4U   /*!< WLC Listen Control Drv Info Offset */
#define NDEF_WLC_LISTEN_CTL_HOLD_OFF_WT_INT_OFFSET          5U   /*!< WLC Listen Control Hold Off WT INT Offset */
#define NDEF_WLC_LISTEN_CTL_ERROR_INFO_OFFSET               5U   /*!< WLC Listen Control Error Info Offset */

/* WLC Listen Ctl: Status Info */
#define NDEF_WLC_LISTEN_CTL_STATUS_INFO_ERROR_SHIFT          7U /*! WLC Listen Control Status Error Flag bit shift */
#define NDEF_WLC_LISTEN_CTL_STATUS_INFO_ERROR_MASK           1U /*! WLC Listen Control Status Error Flag mask */

/* WLC Listen Ctl: Battery Status */
#define NDEF_WLC_LISTEN_CTL_STATUS_INFO_BATTERY_SHIFT       3U /*! WLC Listen Control Battery Status bit shift */
#define NDEF_WLC_LISTEN_CTL_STATUS_INFO_BATTERY_MASK        3U /*! WLC Listen Control Battery Status mask */

/* WLC Listen Ctl: Status Counter */
#define NDEF_WLC_LISTEN_CTL_STATUS_INFO_COUNTER_SHIFT       0U /*! WLC Listen Control Status Counter bit shift */
#define NDEF_WLC_LISTEN_CTL_STATUS_INFO_COUNTER_MASK      0x7U /*! WLC Listen Control Status Counter Status mask */

/* WLC Listen Ctl: WPT Config Req */
#define NDEF_WLC_LISTEN_CTL_WPT_CONFIG_REQ_SHIFT            6U /*! WLC Listen Control WPT Config Req bit shift */
#define NDEF_WLC_LISTEN_CTL_WPT_CONFIG_REQ_MASK             3U /*! WLC Listen Control WPT Config Req mask */

/* WLC Listen Ctl: WPT Config Duration */
#define NDEF_WLC_LISTEN_CTL_WPT_CONFIG_DURATION_SHIFT       1U /*! WLC Listen Control WPT Config Duration bit shift */
#define NDEF_WLC_LISTEN_CTL_WPT_CONFIG_DURATION_MASK     0x1FU /*! WLC Listen Control WPT Config Duration mask */

/* WLC Listen Ctl: WPT Config Info Request */
#define NDEF_WLC_LISTEN_CTL_WPT_CONFIG_INFO_REQ_SHIFT       0U /*! WLC Listen Control WPT Config Info Request bit shift */
#define NDEF_WLC_LISTEN_CTL_WPT_CONFIG_INFO_REQ_MASK        1U /*! WLC Listen Control WPT Config Info Request mask */

/* WLC Listen Ctl: DRV Info Flag */
#define NDEF_WLC_LISTEN_CTL_DRV_INFO_FLAG_SHIFT             6U /*! WLC Listen Control Drv Info Flag bit shift */
#define NDEF_WLC_LISTEN_CTL_DRV_INFO_FLAG_MASK              3U /*! WLC Listen Control Drv Info Flag mask */

/* WLC Listen Ctl: DRV Info Int */
#define NDEF_WLC_LISTEN_CTL_DRV_INFO_INT_SHIFT              0U /*! WLC Listen Control Drv Info Int bit shift */
#define NDEF_WLC_LISTEN_CTL_DRV_INFO_INT_MASK            0x3FU /*! WLC Listen Control Drv Info Int mask */

/* WLC Listen Ctl: Error Info/WLC Protocol Error */
#define NDEF_WLC_LISTEN_CTL_ERROR_INFO_PROTOCOL_SHIFT       1U /*! WLC Listen Control Error Info Protocol bit shift */
#define NDEF_WLC_LISTEN_CTL_ERROR_INFO_PROTOCOL_MASK        1U /*! WLC Listen Control Error Info Protocol mask */

/* WLC Listen Ctl: Error Info/Temperature */
#define NDEF_WLC_LISTEN_CTL_ERROR_INFO_TEMPERATURE_SHIFT    0U /*! WLC Listen Control Error Info Temperature bit shift */
#define NDEF_WLC_LISTEN_CTL_ERROR_INFO_TEMPERATURE_MASK     1U /*! WLC Listen Control Error Info Temperature mask */


/*
 ******************************************************************************
 * LOCAL VARIABLES
 ******************************************************************************
 */


/*! RTD WLC Type strings */
static const uint8_t ndefRtdTypeWlcCapability[] = "WLCCAP";  /*!< WLC Capability Record Type             */
static const uint8_t ndefRtdTypeWlcStatusInfo[] = "WLCSTAI"; /*!< WLC Status and Information Record Type */
static const uint8_t ndefRtdTypeWlcPollInfo[]   = "WLCINF";  /*!< WLC Poll Info Record Type              */
static const uint8_t ndefRtdTypeWlcListenCtl[]  = "WLCCTL";  /*!< WLC Listen Control Record Type         */

const ndefConstBuffer8 bufTypeRtdWlcCapability = { ndefRtdTypeWlcCapability, sizeof(ndefRtdTypeWlcCapability) - 1U };    /*!< WLC Capabiilty Type Record buffer       */
const ndefConstBuffer8 bufTypeRtdWlcStatusInfo = { ndefRtdTypeWlcStatusInfo, sizeof(ndefRtdTypeWlcStatusInfo) - 1U };    /*!< WLC Capabiilty Type Record buffer       */
const ndefConstBuffer8 bufTypeRtdWlcPollInfo   = { ndefRtdTypeWlcPollInfo,   sizeof(ndefRtdTypeWlcPollInfo) - 1U };      /*!< WLC Poll Information Type Record buffer */
const ndefConstBuffer8 bufTypeRtdWlcListenCtl  = { ndefRtdTypeWlcListenCtl,  sizeof(ndefRtdTypeWlcListenCtl) - 1U };     /*!< WLC Listen Control Type Record buffer   */


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


/*****************************************************************************/
/*
 * WLC Capability
 */
/*****************************************************************************/


/*****************************************************************************/
static uint32_t ndefRtdWlcCapabilityGetPayloadLength(const ndefType *type)
{
  if ((type == NULL) || (type->id != NDEF_TYPE_ID_RTD_WLCCAP)) {
    return 0;
  }

  return NDEF_TYPE_RTD_WLC_PAYLOAD_LENGTH;
}


/*****************************************************************************/
static const uint8_t *ndefRtdWlcCapabilityGetPayloadItem(const ndefType *type, ndefConstBuffer *bufItem, bool begin)
{
  static uint32_t item = 0;
  static uint8_t temp = 0;
  const ndefTypeRtdWlcCapability *ndefData;

  if ((type    == NULL) || (type->id != NDEF_TYPE_ID_RTD_WLCCAP) ||
      (bufItem == NULL)) {
    return NULL;
  }

  ndefData = &type->data.wlcCapability;

  if (begin == true) {
    item = 0;
  }

  /* Prepare bufItem->length: Each field is 1 byte, except when complete */
  bufItem->length = sizeof(uint8_t);

  switch (item) {
    case 0: /* WLC Protocol Version */
      bufItem->buffer = &ndefData->wlcProtocolVersion;
      break;
    case 1: { /* WLC Config */
        temp =
          ((ndefData->wlcConfigModeReq       & NDEF_WLC_CAPABILITY_CONFIG_MODE_REQ_MASK)        << NDEF_WLC_CAPABILITY_CONFIG_MODE_REQ_SHIFT)
          | ((ndefData->wlcConfigWaitTimeRetry & NDEF_WLC_CAPABILITY_CONFIG_WAIT_TIME_RETRY_MASK) << NDEF_WLC_CAPABILITY_CONFIG_WAIT_TIME_RETRY_SHIFT)
          | ((ndefData->wlcConfigNegoWait      & NDEF_WLC_CAPABILITY_CONFIG_NEGO_WAIT_MASK)       << NDEF_WLC_CAPABILITY_CONFIG_NEGO_WAIT_SHIFT)
          | ((ndefData->wlcConfigRdConf        & NDEF_WLC_CAPABILITY_CONFIG_RD_CONF_MASK)         << NDEF_WLC_CAPABILITY_CONFIG_RD_CONF_SHIFT);

        bufItem->buffer = &temp;
        break;
      }
    case 2: /* Cap Wt Int */
      temp =
        ((ndefData->capWtIntRfu & NDEF_WLC_CAPABILITY_CONFIG_CAP_WT_INT_RFU_MASK) << NDEF_WLC_CAPABILITY_CONFIG_CAP_WT_INT_RFU_SHIFT)
        | ((ndefData->capWtInt    & NDEF_WLC_CAPABILITY_CONFIG_CAP_WT_INT_MASK)     << NDEF_WLC_CAPABILITY_CONFIG_CAP_WT_INT_SHIFT);

      bufItem->buffer = &temp;
      break;
    case 3: /* NDEF Rd Wt */
      bufItem->buffer = &ndefData->ndefRdWt;
      break;
    case 4: /* NDEF Write To Int */
      bufItem->buffer = &ndefData->ndefWriteToInt;
      break;
    case 5: /* NDEF Write Wt Int */
      bufItem->buffer = &ndefData->ndefWriteWtInt;
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
ReturnCode ndefRtdWlcCapabilityInit(ndefType *type, const ndefTypeRtdWlcCapability *param)
{
  ndefTypeRtdWlcCapability *ndefData;

  if ((type == NULL) || (param == NULL)) {
    return ERR_PARAM;
  }

  type->id               = NDEF_TYPE_ID_RTD_WLCCAP;
  type->getPayloadLength = ndefRtdWlcCapabilityGetPayloadLength;
  type->getPayloadItem   = ndefRtdWlcCapabilityGetPayloadItem;
  type->typeToRecord     = ndefRtdWlcCapabilityToRecord;
  ndefData               = &type->data.wlcCapability;

  (void)ST_MEMCPY(ndefData, param, sizeof(ndefTypeRtdWlcCapability));

  return ERR_NONE;
}


/*****************************************************************************/
ReturnCode ndefGetRtdWlcCapability(const ndefType *type, ndefTypeRtdWlcCapability *param)
{
  const ndefTypeRtdWlcCapability *ndefData;

  if ((type  == NULL) || (type->id != NDEF_TYPE_ID_RTD_WLCCAP) ||
      (param == NULL)) {
    return ERR_PARAM;
  }

  ndefData = &type->data.wlcCapability;

  (void)ST_MEMCPY(param, ndefData, sizeof(ndefTypeRtdWlcCapability));

  return ERR_NONE;
}


/*****************************************************************************/
static ReturnCode ndefPayloadToRtdWlcCapability(const ndefConstBuffer *bufPayload, ndefType *type)
{
  ndefTypeRtdWlcCapability *ndefData;

  if ((bufPayload == NULL) || (bufPayload->buffer == NULL) ||
      (type       == NULL)) {
    return ERR_PARAM;
  }

  if (bufPayload->length != NDEF_TYPE_RTD_WLC_PAYLOAD_LENGTH) {
    return ERR_PARAM;
  }

  type->id               = NDEF_TYPE_ID_RTD_WLCCAP;
  type->getPayloadLength = ndefRtdWlcCapabilityGetPayloadLength;
  type->getPayloadItem   = ndefRtdWlcCapabilityGetPayloadItem;
  type->typeToRecord     = ndefRtdWlcCapabilityToRecord;
  ndefData               = &type->data.wlcCapability;

  ndefData->wlcProtocolVersion     = bufPayload->buffer[NDEF_WLC_CAPABILITY_PROTOCOL_VERSION_OFFSET];

  uint8_t config = bufPayload->buffer[NDEF_WLC_CAPABILITY_CONFIG_OFFSET];
  ndefData->wlcConfigModeReq       = (config >> NDEF_WLC_CAPABILITY_CONFIG_MODE_REQ_SHIFT)        & NDEF_WLC_CAPABILITY_CONFIG_MODE_REQ_MASK;
  ndefData->wlcConfigWaitTimeRetry = (config >> NDEF_WLC_CAPABILITY_CONFIG_WAIT_TIME_RETRY_SHIFT) & NDEF_WLC_CAPABILITY_CONFIG_WAIT_TIME_RETRY_MASK;
  ndefData->wlcConfigNegoWait      = (config >> NDEF_WLC_CAPABILITY_CONFIG_NEGO_WAIT_SHIFT)       & NDEF_WLC_CAPABILITY_CONFIG_NEGO_WAIT_MASK;
  ndefData->wlcConfigRdConf        = (config >> NDEF_WLC_CAPABILITY_CONFIG_RD_CONF_SHIFT)         & NDEF_WLC_CAPABILITY_CONFIG_RD_CONF_MASK;

  uint8_t capWtInt = bufPayload->buffer[NDEF_WLC_CAPABILITY_WT_INT_OFFSET];
  ndefData->capWtIntRfu            = (capWtInt >> NDEF_WLC_CAPABILITY_CONFIG_CAP_WT_INT_RFU_SHIFT) & NDEF_WLC_CAPABILITY_CONFIG_CAP_WT_INT_RFU_MASK;
  ndefData->capWtInt               = (capWtInt >> NDEF_WLC_CAPABILITY_CONFIG_CAP_WT_INT_SHIFT)     & NDEF_WLC_CAPABILITY_CONFIG_CAP_WT_INT_MASK;
  ndefData->ndefRdWt               = bufPayload->buffer[NDEF_WLC_CAPABILITY_NDEF_RD_WT_OFFSET];
  ndefData->ndefWriteToInt         = bufPayload->buffer[NDEF_WLC_CAPABILITY_NDEF_WR_TO_INT_OFFSET];
  ndefData->ndefWriteWtInt         = bufPayload->buffer[NDEF_WLC_CAPABILITY_NDEF_WR_WT_INT_OFFSET];

  return ERR_NONE;
}


/*****************************************************************************/
ReturnCode ndefRecordToRtdWlcCapability(const ndefRecord *record, ndefType *type)
{
  const ndefType *ndefData;

  if ((record == NULL) || (type == NULL)) {
    return ERR_PARAM;
  }

  /* NDEF TNF and String type */
  if (! ndefRecordTypeMatch(record, NDEF_TNF_RTD_WELL_KNOWN_TYPE, &bufTypeRtdWlcCapability)) {
    return ERR_PROTO;
  }

  ndefData = ndefRecordGetNdefType(record);
  if ((ndefData != NULL) && (ndefData->id == NDEF_TYPE_ID_RTD_WLCCAP)) {
    (void)ST_MEMCPY(type, ndefData, sizeof(ndefType));
    return ERR_NONE;
  }

  return ndefPayloadToRtdWlcCapability(&record->bufPayload, type);
}


/*****************************************************************************/
ReturnCode ndefRtdWlcCapabilityToRecord(const ndefType *type, ndefRecord *record)
{
  if ((type   == NULL) || (type->id != NDEF_TYPE_ID_RTD_WLCCAP) ||
      (record == NULL)) {
    return ERR_PARAM;
  }

  (void)ndefRecordReset(record);

  /* String type */
  (void)ndefRecordSetType(record, NDEF_TNF_RTD_WELL_KNOWN_TYPE, &bufTypeRtdWlcCapability);

  if (ndefRecordSetNdefType(record, type) != ERR_NONE) {
    return ERR_PARAM;
  }

  return ERR_NONE;
}


/*****************************************************************************/
/*
 * WLC Status and Information
 */
/*****************************************************************************/


/*****************************************************************************/
static uint32_t ndefRtdWlcStatusInfoGetPayloadLength(const ndefType *type)
{
  uint32_t length;

  if ((type == NULL) || (type->id != NDEF_TYPE_ID_RTD_WLCSTAI)) {
    return 0;
  }

  const ndefTypeRtdWlcStatusInfo *ndefData = &type->data.wlcStatusInfo;

  length = sizeof(uint8_t); /* Control byte 1 */

  /* Count the number of bits set in the Control byte 1 to get the number of following bytes */
  for (uint32_t i = 0; i < 8U; i++) {
    if ((ndefData->controlByte1 & (1U << i)) != 0U) {
      length += sizeof(uint8_t);
    }
  }

  return length;
}


/*****************************************************************************/
static const uint8_t *ndefRtdWlcStatusInfoGetPayloadItem(const ndefType *type, ndefConstBuffer *bufItem, bool begin)
{
  static uint32_t item = 0;
  const ndefTypeRtdWlcStatusInfo *ndefData;

  if ((type    == NULL) || (type->id != NDEF_TYPE_ID_RTD_WLCSTAI) ||
      (bufItem == NULL)) {
    return NULL;
  }

  ndefData = &type->data.wlcStatusInfo;

  if (begin == true) {
    item = 0;
  }

  /* Prepare bufItem->length: Each field is 1 byte, except when complete */
  bufItem->length = sizeof(uint8_t);

  switch (item) {
    case 0: /* Control Byte 1 */
      bufItem->buffer = &ndefData->controlByte1;
      item++;
      break;
    case 1: /* Battery Level */
      if ((ndefData->controlByte1 & NDEF_WLC_STATUSINFO_CONTROLBYTE1_BATTERY_LEVEL_MASK) != 0U) {
        bufItem->buffer = &ndefData->batteryLevel;
        item = 2;
        break;
      }
    /* fall through */
    case 2: /* Receive Power */
      if ((ndefData->controlByte1 & NDEF_WLC_STATUSINFO_CONTROLBYTE1_RECEIVE_POWER_MASK) != 0U) {
        bufItem->buffer = &ndefData->receivePower;
        item = 3;
        break;
      }
    /* fall through */
    case 3: /* Receive Voltage */
      if ((ndefData->controlByte1 & NDEF_WLC_STATUSINFO_CONTROLBYTE1_RECEIVE_VOLTAGE_MASK) != 0U) {
        bufItem->buffer = &ndefData->receiveVoltage;
        item = 4;
        break;
      }
    /* fall through */
    case 4: /* Receive Current */
      if ((ndefData->controlByte1 & NDEF_WLC_STATUSINFO_CONTROLBYTE1_RECEIVE_CURRENT_MASK) != 0U) {
        bufItem->buffer = &ndefData->receiveCurrent;
        item = 5;
        break;
      }
    /* fall through */
    case 5: /* Receive Current */
      if ((ndefData->controlByte1 & NDEF_WLC_STATUSINFO_CONTROLBYTE1_TEMPERATURE_BATTERY_MASK) != 0U) {
        bufItem->buffer = &ndefData->temperatureBattery;
        item = 6;
        break;
      }
    /* fall through */
    case 6: /* Receive Current */
      if ((ndefData->controlByte1 & NDEF_WLC_STATUSINFO_CONTROLBYTE1_TEMPERATURE_WLCL_MASK) != 0U) {
        bufItem->buffer = &ndefData->temperatureWlcl;
        item = 7;
        break;
      }
    /* fall through */
    case 7: /* Receive Current */
      if ((ndefData->controlByte1 & NDEF_WLC_STATUSINFO_CONTROLBYTE1_RFU_MASK) != 0U) {
        bufItem->buffer = &ndefData->rfu;
        item = 8;
        break;
      }
    /* fall through */
    case 8: /* Receive Current */
      if ((ndefData->controlByte1 & NDEF_WLC_STATUSINFO_CONTROLBYTE1_CONTROL_BYTE_2_MASK) != 0U) {
        bufItem->buffer = &ndefData->controlByte2;
        item = 9;
        break;
      }
    /* fall through */
    default:
      bufItem->buffer = NULL;
      bufItem->length = 0;
      break;
  }

  return bufItem->buffer;
}


/*****************************************************************************/
ReturnCode ndefRtdWlcStatusInfoInit(ndefType *type, const ndefTypeRtdWlcStatusInfo *param)
{
  ndefTypeRtdWlcStatusInfo *ndefData;

  if ((type == NULL) || (param == NULL)) {
    return ERR_PARAM;
  }

  type->id               = NDEF_TYPE_ID_RTD_WLCSTAI;
  type->getPayloadLength = ndefRtdWlcStatusInfoGetPayloadLength;
  type->getPayloadItem   = ndefRtdWlcStatusInfoGetPayloadItem;
  type->typeToRecord     = ndefRtdWlcStatusInfoToRecord;
  ndefData               = &type->data.wlcStatusInfo;

  (void)ST_MEMCPY(ndefData, param, sizeof(ndefTypeRtdWlcStatusInfo));

  return ERR_NONE;
}


/*****************************************************************************/
ReturnCode ndefGetRtdWlcStatusInfo(const ndefType *type, ndefTypeRtdWlcStatusInfo *param)
{
  const ndefTypeRtdWlcStatusInfo *ndefData;

  if ((type  == NULL) || (type->id != NDEF_TYPE_ID_RTD_WLCSTAI) ||
      (param == NULL)) {
    return ERR_PARAM;
  }

  ndefData = &type->data.wlcStatusInfo;

  (void)ST_MEMCPY(param, ndefData, sizeof(ndefTypeRtdWlcStatusInfo));

  return ERR_NONE;
}


/*****************************************************************************/
static ReturnCode ndefPayloadToRtdWlcStatusInfo(const ndefConstBuffer *bufPayload, ndefType *type)
{
  ndefTypeRtdWlcStatusInfo *ndefData;

  if ((bufPayload == NULL) || (bufPayload->buffer == NULL) ||
      (type       == NULL)) {
    return ERR_PARAM;
  }

  if ((bufPayload->length < NDEF_TYPE_RTD_WLC_STATUS_INFO_MIN_PAYLOAD_LENGTH) ||
      (bufPayload->length > NDEF_TYPE_RTD_WLC_STATUS_INFO_MAX_PAYLOAD_LENGTH)) {
    return ERR_PROTO;
  }

  type->id               = NDEF_TYPE_ID_RTD_WLCSTAI;
  type->getPayloadLength = ndefRtdWlcStatusInfoGetPayloadLength;
  type->getPayloadItem   = ndefRtdWlcStatusInfoGetPayloadItem;
  type->typeToRecord     = ndefRtdWlcStatusInfoToRecord;
  ndefData               = &type->data.wlcStatusInfo;

  uint32_t offset = NDEF_WLC_STATUSINFO_CONTROL_BYTE_1_OFFSET;

  uint8_t controlByte1 = bufPayload->buffer[offset];
  offset++;

  /* Initialize each field */
  ndefData->controlByte1       = controlByte1;
  ndefData->batteryLevel       = 0;
  ndefData->receivePower       = 0;
  ndefData->receiveVoltage     = 0;
  ndefData->receiveCurrent     = 0;
  ndefData->temperatureBattery = 0;
  ndefData->temperatureWlcl    = 0;
  ndefData->rfu                = 0;
  ndefData->controlByte2       = 0;

  /* Check each bit in Control byte 1 and read the appropriate byte */
  if ((controlByte1 & NDEF_WLC_STATUSINFO_CONTROLBYTE1_BATTERY_LEVEL_MASK) != 0U) {
    ndefData->batteryLevel = bufPayload->buffer[offset];
    offset++;
  }

  if ((controlByte1 & NDEF_WLC_STATUSINFO_CONTROLBYTE1_RECEIVE_POWER_MASK) != 0U) {
    ndefData->receivePower = bufPayload->buffer[offset];
    offset++;
  }

  if ((controlByte1 & NDEF_WLC_STATUSINFO_CONTROLBYTE1_RECEIVE_VOLTAGE_MASK) != 0U) {
    ndefData->receiveVoltage = bufPayload->buffer[offset];
    offset++;
  }

  if ((controlByte1 & NDEF_WLC_STATUSINFO_CONTROLBYTE1_RECEIVE_CURRENT_MASK) != 0U) {
    ndefData->receiveCurrent = bufPayload->buffer[offset];
    offset++;
  }

  if ((controlByte1 & NDEF_WLC_STATUSINFO_CONTROLBYTE1_TEMPERATURE_BATTERY_MASK) != 0U) {
    ndefData->temperatureBattery = bufPayload->buffer[offset];
    offset++;
  }

  if ((controlByte1 & NDEF_WLC_STATUSINFO_CONTROLBYTE1_TEMPERATURE_WLCL_MASK) != 0U) {
    ndefData->temperatureWlcl = bufPayload->buffer[offset];
    offset++;
  }

  if ((controlByte1 & NDEF_WLC_STATUSINFO_CONTROLBYTE1_RFU_MASK) != 0U) {
    ndefData->rfu = bufPayload->buffer[offset];
    offset++;
  }

  if ((controlByte1 & NDEF_WLC_STATUSINFO_CONTROLBYTE1_CONTROL_BYTE_2_MASK) != 0U) {
    ndefData->controlByte2 = bufPayload->buffer[offset];
    /*offset++;*/
  }

  return ERR_NONE;
}


/*****************************************************************************/
ReturnCode ndefRecordToRtdWlcStatusInfo(const ndefRecord *record, ndefType *type)
{
  const ndefType *ndefData;

  if ((record == NULL) || (type == NULL)) {
    return ERR_PARAM;
  }

  /* NDEF TNF and String type */
  if (! ndefRecordTypeMatch(record, NDEF_TNF_RTD_WELL_KNOWN_TYPE, &bufTypeRtdWlcStatusInfo)) {
    return ERR_PROTO;
  }

  ndefData = ndefRecordGetNdefType(record);
  if ((ndefData != NULL) && (ndefData->id == NDEF_TYPE_ID_RTD_WLCSTAI)) {
    (void)ST_MEMCPY(type, ndefData, sizeof(ndefType));
    return ERR_NONE;
  }

  return ndefPayloadToRtdWlcStatusInfo(&record->bufPayload, type);
}


/*****************************************************************************/
ReturnCode ndefRtdWlcStatusInfoToRecord(const ndefType *type, ndefRecord *record)
{
  if ((type   == NULL) || (type->id != NDEF_TYPE_ID_RTD_WLCSTAI) ||
      (record == NULL)) {
    return ERR_PARAM;
  }

  (void)ndefRecordReset(record);

  /* String type */
  (void)ndefRecordSetType(record, NDEF_TNF_RTD_WELL_KNOWN_TYPE, &bufTypeRtdWlcStatusInfo);

  if (ndefRecordSetNdefType(record, type) != ERR_NONE) {
    return ERR_PARAM;
  }

  return ERR_NONE;
}


/*****************************************************************************/
/*
 * WLC Poll Information
 */
/*****************************************************************************/


/*****************************************************************************/
static uint32_t ndefRtdWlcPollInfoGetPayloadLength(const ndefType *type)
{
  if ((type == NULL) || ((type)->id != NDEF_TYPE_ID_RTD_WLCINFO)) {
    return 0;
  }

  return NDEF_TYPE_RTD_WLC_PAYLOAD_LENGTH;
}


/*****************************************************************************/
static const uint8_t *ndefRtdWlcPollInfoGetPayloadItem(const ndefType *type, ndefConstBuffer *bufItem, bool begin)
{
  static uint32_t item = 0;
  const ndefTypeRtdWlcPollInfo *ndefData;

  if ((type    == NULL) || (type->id != NDEF_TYPE_ID_RTD_WLCINFO) ||
      (bufItem == NULL)) {
    return NULL;
  }

  ndefData = &type->data.wlcPollInfo;

  if (begin == true) {
    item = 0;
  }

  /* Prepare bufItem->length: Each field is 1 byte, except when complete */
  bufItem->length = sizeof(uint8_t);

  switch (item) {
    case 0: /* P Tx */
      bufItem->buffer = &ndefData->pTx;
      break;
    case 1: { /* WLC-P Capability | Power class */
        static uint8_t temp;
        temp =
          ((ndefData->wlcPCap   << NDEF_WLC_POLL_INFO_WLC_P_CAPABILITY_SHIFT)
           | ((ndefData->powerClass & NDEF_WLC_POLL_INFO_WLC_P_CAPABILITY_MASK)));

        bufItem->buffer = &temp;
        break;
      }
    case 2: /* Total Power Steps */
      bufItem->buffer = &ndefData->totPowerSteps;
      break;
    case 3: /* Current Power Steps */
      bufItem->buffer = &ndefData->curPowerStep;
      break;
    case 4: /* Next min step increment */
      bufItem->buffer = &ndefData->nextMinStepInc;
      break;
    case 5: /* Next min step decrement */
      bufItem->buffer = &ndefData->nextMinStepDec;
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
ReturnCode ndefRtdWlcPollInfoInit(ndefType *type, const ndefTypeRtdWlcPollInfo *param)
{
  ndefTypeRtdWlcPollInfo *ndefData;

  if ((type == NULL) || (param == NULL)) {
    return ERR_PARAM;
  }

  type->id               = NDEF_TYPE_ID_RTD_WLCINFO;
  type->getPayloadLength = ndefRtdWlcPollInfoGetPayloadLength;
  type->getPayloadItem   = ndefRtdWlcPollInfoGetPayloadItem;
  type->typeToRecord     = ndefRtdWlcPollInfoToRecord;
  ndefData               = &type->data.wlcPollInfo;

  (void)ST_MEMCPY(ndefData, param, sizeof(ndefTypeRtdWlcPollInfo));

  return ERR_NONE;
}


/*****************************************************************************/
ReturnCode ndefGetRtdWlcPollInfo(const ndefType *type, ndefTypeRtdWlcPollInfo *param)
{
  const ndefTypeRtdWlcPollInfo *ndefData;

  if ((type  == NULL) || (type->id != NDEF_TYPE_ID_RTD_WLCINFO) ||
      (param == NULL)) {
    return ERR_PARAM;
  }

  ndefData = &type->data.wlcPollInfo;

  (void)ST_MEMCPY(param, ndefData, sizeof(ndefTypeRtdWlcPollInfo));

  return ERR_NONE;
}


/*****************************************************************************/
static ReturnCode ndefPayloadToRtdWlcPollInfo(const ndefConstBuffer *bufPayload, ndefType *type)
{
  ndefTypeRtdWlcPollInfo *ndefData;

  if ((bufPayload == NULL) || (bufPayload->buffer == NULL) ||
      (type       == NULL)) {
    return ERR_PARAM;
  }

  if (bufPayload->length != NDEF_TYPE_RTD_WLC_PAYLOAD_LENGTH) {
    return ERR_PARAM;
  }

  type->id               = NDEF_TYPE_ID_RTD_WLCINFO;
  type->getPayloadLength = ndefRtdWlcPollInfoGetPayloadLength;
  type->getPayloadItem   = ndefRtdWlcPollInfoGetPayloadItem;
  type->typeToRecord     = ndefRtdWlcPollInfoToRecord;
  ndefData               = &type->data.wlcPollInfo;

  ndefData->pTx            = bufPayload->buffer[NDEF_WLC_POLL_INFO_PTX_OFFSET];
  ndefData->wlcPCap        = bufPayload->buffer[NDEF_WLC_POLL_INFO_WLC_P_CAPABILITY_OFFSET] >> NDEF_WLC_POLL_INFO_WLC_P_CAPABILITY_SHIFT;
  ndefData->powerClass     = bufPayload->buffer[NDEF_WLC_POLL_INFO_POWER_CLASS_OFFSET]       & NDEF_WLC_POLL_INFO_WLC_P_CAPABILITY_MASK;
  ndefData->totPowerSteps  = bufPayload->buffer[NDEF_WLC_POLL_INFO_TOT_POWER_STEPS_OFFSET];
  ndefData->curPowerStep   = bufPayload->buffer[NDEF_WLC_POLL_INFO_CUR_POWER_STEP_OFFSET];
  ndefData->nextMinStepInc = bufPayload->buffer[NDEF_WLC_POLL_INFO_NEXT_MIN_STEP_INC_OFFSET];
  ndefData->nextMinStepDec = bufPayload->buffer[NDEF_WLC_POLL_INFO_NEXT_MIN_STEP_DEC_OFFSET];

  return ERR_NONE;
}


/*****************************************************************************/
ReturnCode ndefRecordToRtdWlcPollInfo(const ndefRecord *record, ndefType *type)
{
  const ndefType *ndefData;

  if ((record == NULL) || (type == NULL)) {
    return ERR_PARAM;
  }

  /* NDEF TNF and String type */
  if (! ndefRecordTypeMatch(record, NDEF_TNF_RTD_WELL_KNOWN_TYPE, &bufTypeRtdWlcPollInfo)) {
    return ERR_PROTO;
  }

  ndefData = ndefRecordGetNdefType(record);
  if ((ndefData != NULL) && (ndefData->id == NDEF_TYPE_ID_RTD_WLCINFO)) {
    (void)ST_MEMCPY(type, ndefData, sizeof(ndefType));
    return ERR_NONE;
  }

  return ndefPayloadToRtdWlcPollInfo(&record->bufPayload, type);
}


/*****************************************************************************/
ReturnCode ndefRtdWlcPollInfoToRecord(const ndefType *type, ndefRecord *record)
{
  if ((type   == NULL) || ((type)->id != NDEF_TYPE_ID_RTD_WLCINFO) ||
      (record == NULL)) {
    return ERR_PARAM;
  }

  (void)ndefRecordReset(record);

  /* String type */
  (void)ndefRecordSetType(record, NDEF_TNF_RTD_WELL_KNOWN_TYPE, &bufTypeRtdWlcPollInfo);

  if (ndefRecordSetNdefType(record, type) != ERR_NONE) {
    return ERR_PARAM;
  }

  return ERR_NONE;
}


/*****************************************************************************/
/*
 * WLC Listen Control
 */
/*****************************************************************************/


/*****************************************************************************/
static uint32_t ndefRtdWlcListenCtlGetPayloadLength(const ndefType *type)
{
  const ndefTypeRtdWlcListenCtl *ndefData;
  uint32_t payloadLength;

  if ((type == NULL) || ((type)->id != NDEF_TYPE_ID_RTD_WLCCTL)) {
    return 0;
  }

  ndefData = &type->data.wlcListenCtl;

  payloadLength  = NDEF_TYPE_RTD_WLC_PAYLOAD_LENGTH;
  /* Check for optional last ERROR_INFO byte */
  payloadLength += ((ndefData->statusInfoErrorFlag != 0U) ? 1U : 0U);

  return payloadLength;
}


/*****************************************************************************/
static const uint8_t *ndefRtdWlcListenCtlGetPayloadItem(const ndefType *type, ndefConstBuffer *bufItem, bool begin)
{
  static uint32_t item = 0;
  static uint8_t temp = 0;
  const ndefTypeRtdWlcListenCtl *ndefData;

  if ((type    == NULL) || ((type)->id != NDEF_TYPE_ID_RTD_WLCCTL) ||
      (bufItem == NULL)) {
    return NULL;
  }

  ndefData = &type->data.wlcListenCtl;

  if (begin == true) {
    item = 0;
  }

  /* Prepare bufItem->length: Each field is 1 byte, except when complete */
  bufItem->length = sizeof(uint8_t);

  switch (item) {
    case 0: { /* Status Info */
        temp =
          ((ndefData->statusInfoErrorFlag     & NDEF_WLC_LISTEN_CTL_STATUS_INFO_ERROR_MASK)   << NDEF_WLC_LISTEN_CTL_STATUS_INFO_ERROR_SHIFT)
          | ((ndefData->statusInfoBatteryStatus & NDEF_WLC_LISTEN_CTL_STATUS_INFO_BATTERY_MASK) << NDEF_WLC_LISTEN_CTL_STATUS_INFO_BATTERY_SHIFT)
          | ((ndefData->statusInfoCnt           & NDEF_WLC_LISTEN_CTL_STATUS_INFO_COUNTER_MASK) << NDEF_WLC_LISTEN_CTL_STATUS_INFO_COUNTER_SHIFT);

        bufItem->buffer = &temp;
        break;
      }
    case 1: /* WPT Config */
      temp =
        ((ndefData->wptConfigWptReq      & NDEF_WLC_LISTEN_CTL_WPT_CONFIG_REQ_MASK)      << NDEF_WLC_LISTEN_CTL_WPT_CONFIG_REQ_SHIFT)
        | ((ndefData->wptConfigWptDuration & NDEF_WLC_LISTEN_CTL_WPT_CONFIG_DURATION_MASK) << NDEF_WLC_LISTEN_CTL_WPT_CONFIG_DURATION_SHIFT)
        | ((ndefData->wptConfigInfoReq     & NDEF_WLC_LISTEN_CTL_WPT_CONFIG_INFO_REQ_MASK) << NDEF_WLC_LISTEN_CTL_WPT_CONFIG_INFO_REQ_SHIFT);

      bufItem->buffer = &temp;
      break;
    case 2: /* Power Adjust Request */
      bufItem->buffer = &ndefData->powerAdjReq;
      break;
    case 3: /* Battery Level */
      bufItem->buffer = &ndefData->batteryLevel;
      break;
    case 4: { /* Drv Info */
        temp =
          ((ndefData->drvInfoFlag & NDEF_WLC_LISTEN_CTL_DRV_INFO_FLAG_MASK) << NDEF_WLC_LISTEN_CTL_DRV_INFO_FLAG_SHIFT)
          | ((ndefData->drvInfoInt  & NDEF_WLC_LISTEN_CTL_DRV_INFO_INT_MASK)  << NDEF_WLC_LISTEN_CTL_DRV_INFO_INT_SHIFT);

        bufItem->buffer = &temp;
        break;
      }
    case 5: /* Hold Off Wt Int */
      bufItem->buffer = &ndefData->holdOffWtInt;
      break;
    case 6: /* Optional last ERROR_INFO byte */
      if (ndefData->statusInfoErrorFlag != 0U) {
        /* Send the ERROR_INFO byte */
        temp =
          ((ndefData->errorInfoError       & NDEF_WLC_LISTEN_CTL_ERROR_INFO_PROTOCOL_MASK)    << NDEF_WLC_LISTEN_CTL_ERROR_INFO_PROTOCOL_SHIFT)
          | ((ndefData->errorInfoTemperature & NDEF_WLC_LISTEN_CTL_ERROR_INFO_TEMPERATURE_MASK) << NDEF_WLC_LISTEN_CTL_ERROR_INFO_TEMPERATURE_SHIFT);

        bufItem->buffer = &temp;
        break;
      }
    /* fall through if no ERR_INFO byte to send */
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
ReturnCode ndefRtdWlcListenCtlInit(ndefType *type, const ndefTypeRtdWlcListenCtl *param)
{
  ndefTypeRtdWlcListenCtl *ndefData;

  if ((type == NULL) || (param == NULL)) {
    return ERR_PARAM;
  }

  type->id               = NDEF_TYPE_ID_RTD_WLCCTL;
  type->getPayloadLength = ndefRtdWlcListenCtlGetPayloadLength;
  type->getPayloadItem   = ndefRtdWlcListenCtlGetPayloadItem;
  type->typeToRecord     = ndefRtdWlcListenCtlToRecord;
  ndefData               = &type->data.wlcListenCtl;

  (void)ST_MEMCPY(ndefData, param, sizeof(ndefTypeRtdWlcListenCtl));

  return ERR_NONE;
}


/*****************************************************************************/
ReturnCode ndefGetRtdWlcListenCtl(const ndefType *type, ndefTypeRtdWlcListenCtl *param)
{
  const ndefTypeRtdWlcListenCtl *ndefData;

  if ((type  == NULL) || (type->id != NDEF_TYPE_ID_RTD_WLCCTL) ||
      (param == NULL)) {
    return ERR_PARAM;
  }

  ndefData = &type->data.wlcListenCtl;

  (void)ST_MEMCPY(param, ndefData, sizeof(ndefTypeRtdWlcListenCtl));

  return ERR_NONE;
}


/*****************************************************************************/
static ReturnCode ndefPayloadToRtdWlcListenCtl(const ndefConstBuffer *bufPayload, ndefType *type)
{
  ndefTypeRtdWlcListenCtl *ndefData;

  if ((bufPayload == NULL) || (bufPayload->buffer == NULL) ||
      (type       == NULL)) {
    return ERR_PARAM;
  }

  if ((bufPayload->length !=  NDEF_TYPE_RTD_WLC_PAYLOAD_LENGTH) &&
      (bufPayload->length != (NDEF_TYPE_RTD_WLC_PAYLOAD_LENGTH + 1U))) {
    return ERR_PARAM;
  }

  type->id               = NDEF_TYPE_ID_RTD_WLCCTL;
  type->getPayloadLength = ndefRtdWlcListenCtlGetPayloadLength;
  type->getPayloadItem   = ndefRtdWlcListenCtlGetPayloadItem;
  type->typeToRecord     = ndefRtdWlcListenCtlToRecord;
  ndefData               = &type->data.wlcListenCtl;

  uint8_t status = bufPayload->buffer[NDEF_WLC_LISTEN_CTL_STATUS_INFO_OFFSET];
  ndefData->statusInfoErrorFlag     = (status >> NDEF_WLC_LISTEN_CTL_STATUS_INFO_ERROR_SHIFT)   & NDEF_WLC_LISTEN_CTL_STATUS_INFO_ERROR_MASK;
  ndefData->statusInfoBatteryStatus = (status >> NDEF_WLC_LISTEN_CTL_STATUS_INFO_BATTERY_SHIFT) & NDEF_WLC_LISTEN_CTL_STATUS_INFO_BATTERY_MASK;
  ndefData->statusInfoCnt           = (status >> NDEF_WLC_LISTEN_CTL_STATUS_INFO_COUNTER_SHIFT) & NDEF_WLC_LISTEN_CTL_STATUS_INFO_COUNTER_MASK;

  uint8_t config = bufPayload->buffer[NDEF_WLC_LISTEN_CTL_WPT_CONFIG_OFFSET];
  ndefData->wptConfigWptReq         = (config >> NDEF_WLC_LISTEN_CTL_WPT_CONFIG_REQ_SHIFT)      & NDEF_WLC_LISTEN_CTL_WPT_CONFIG_REQ_MASK;
  ndefData->wptConfigWptDuration    = (config >> NDEF_WLC_LISTEN_CTL_WPT_CONFIG_DURATION_SHIFT) & NDEF_WLC_LISTEN_CTL_WPT_CONFIG_DURATION_MASK;
  ndefData->wptConfigInfoReq        = (config >> NDEF_WLC_LISTEN_CTL_WPT_CONFIG_INFO_REQ_SHIFT) & NDEF_WLC_LISTEN_CTL_WPT_CONFIG_INFO_REQ_MASK;

  ndefData->powerAdjReq             = bufPayload->buffer[NDEF_WLC_LISTEN_CTL_POWER_ADJ_REQ_OFFSET];
  ndefData->batteryLevel            = bufPayload->buffer[NDEF_WLC_LISTEN_CTL_BATTERY_LEVEL_OFFSET];

  uint8_t drvInfo = bufPayload->buffer[NDEF_WLC_LISTEN_CTL_DRV_INFO_OFFSET];
  ndefData->drvInfoFlag             = (drvInfo >> NDEF_WLC_LISTEN_CTL_DRV_INFO_FLAG_SHIFT) & NDEF_WLC_LISTEN_CTL_DRV_INFO_FLAG_MASK;
  ndefData->drvInfoInt              = (drvInfo >> NDEF_WLC_LISTEN_CTL_DRV_INFO_INT_SHIFT)  & NDEF_WLC_LISTEN_CTL_DRV_INFO_INT_MASK;

  ndefData->holdOffWtInt            = bufPayload->buffer[NDEF_WLC_LISTEN_CTL_HOLD_OFF_WT_INT_OFFSET];

  uint8_t error = 0;
  if (bufPayload->length == (NDEF_TYPE_RTD_WLC_PAYLOAD_LENGTH + 1U)) {
    error = bufPayload->buffer[NDEF_WLC_LISTEN_CTL_ERROR_INFO_OFFSET];
  }
  ndefData->errorInfoError          = (error >> NDEF_WLC_LISTEN_CTL_ERROR_INFO_PROTOCOL_SHIFT)    & NDEF_WLC_LISTEN_CTL_ERROR_INFO_PROTOCOL_MASK;
  ndefData->errorInfoTemperature    = (error >> NDEF_WLC_LISTEN_CTL_ERROR_INFO_TEMPERATURE_SHIFT) & NDEF_WLC_LISTEN_CTL_ERROR_INFO_TEMPERATURE_MASK;

  return ERR_NONE;
}


/*****************************************************************************/
ReturnCode ndefRecordToRtdWlcListenCtl(const ndefRecord *record, ndefType *type)
{
  const ndefType *ndefData;

  if ((record == NULL) || (type == NULL)) {
    return ERR_PARAM;
  }

  /* NDEF TNF and String type */
  if (! ndefRecordTypeMatch(record, NDEF_TNF_RTD_WELL_KNOWN_TYPE, &bufTypeRtdWlcListenCtl)) {
    return ERR_PROTO;
  }

  ndefData = ndefRecordGetNdefType(record);
  if ((ndefData != NULL) && (ndefData->id == NDEF_TYPE_ID_RTD_WLCCTL)) {
    (void)ST_MEMCPY(type, ndefData, sizeof(ndefType));
    return ERR_NONE;
  }

  return ndefPayloadToRtdWlcListenCtl(&record->bufPayload, type);
}


/*****************************************************************************/
ReturnCode ndefRtdWlcListenCtlToRecord(const ndefType *type, ndefRecord *record)
{
  if ((type   == NULL) || ((type)->id != NDEF_TYPE_ID_RTD_WLCCTL) ||
      (record == NULL)) {
    return ERR_PARAM;
  }

  (void)ndefRecordReset(record);

  /* String type */
  (void)ndefRecordSetType(record, NDEF_TNF_RTD_WELL_KNOWN_TYPE, &bufTypeRtdWlcListenCtl);

  if (ndefRecordSetNdefType(record, type) != ERR_NONE) {
    return ERR_PARAM;
  }

  return ERR_NONE;
}

#endif
