
/**
  ******************************************************************************
  * @file           : ndef_type_wifi.cpp
  * @brief          : NDEF Wifi type
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
#include "ndef_type_wifi.h"
#include "nfc_utils.h"


#if NDEF_TYPE_WIFI_SUPPORT


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


/*! Wifi Type strings */
static const uint8_t ndefMediaTypeWifi[]     = "application/vnd.wfa.wsc";   /*!< Wi-Fi Simple Configuration Type */

const ndefConstBuffer8 bufMediaTypeWifi      = { ndefMediaTypeWifi,      sizeof(ndefMediaTypeWifi) - 1U      };  /*!< Wifi Type buffer     */


/*! Wifi OBB (WPS) */

#define NDEF_WIFI_DEFAULT_NETWORK_KEY      "00000000"      /*! Network key to be used when the Authentication is set to None.
                                                               Although WPS defines a 0-length network key in such case,
                                                               a 8 digit value is required with tested phones. */

#define NDEF_WIFI_NETWORK_SSID_LENGTH           32U    /*!< Network SSID length        */
#define NDEF_WIFI_NETWORK_KEY_LENGTH            32U    /*!< Network Key length         */

#define NDEF_WIFI_ENCRYPTION_TYPE_LENGTH         2U    /*!< Encryption type length     */
#define NDEF_WIFI_AUTHENTICATION_TYPE_LENGTH     2U    /*!< Authentication type length */
#define WIFI_SSID_TYPE_LENGTH                    2U    /*!< SSID type length           */
#define WIFI_SSID_KEY_TYPE_LENGTH                2U    /*!< SSID key type length       */

#define NDEF_WIFI_ATTRIBUTE_ID_SSID_LSB       0x10U    /*!< SSID Attribute ID LSB      */
#define NDEF_WIFI_ATTRIBUTE_ID_SSID_MSB       0x45U    /*!< SSID Attribute ID MSB      */

#define NDEF_WIFI_ATTRIBUTE_ID_NETWORK_LSB    0x10U    /*!< Network Attribute ID LSB   */
#define NDEF_WIFI_ATTRIBUTE_ID_NETWORK_MSB    0x27U    /*!< Network Attribute ID MSB   */

#define NDEF_WIFI_ATTRIBUTE_ENCRYPTION        0x0FU    /*!< Encryption attribute       */
#define NDEF_WIFI_ATTRIBUTE_AUTHENTICATION    0x03U    /*!< Authentication attribute   */

#define NDEF_WIFI_ATTRIBUTE_ID_OFFSET                 0x01U    /*!< Attribute Id offset */
#define NDEF_WIFI_ATTRIBUTE_LENGTH_MSB_OFFSET         0x02U    /*!< Attribute length MSB offset     */
#define NDEF_WIFI_ATTRIBUTE_LENGTH_LSB_OFFSET         0x03U    /*!< Attribute length LSB offset     */
#define NDEF_WIFI_ATTRIBUTE_DATA_OFFSET               0x04U    /*!< Attribute data offset           */
#define NDEF_WIFI_ATTRIBUTE_ENCRYPTION_LSB_OFFSET     0x05U    /*!< Attribute encryption offset     */
#define NDEF_WIFI_ATTRIBUTE_AUTHENTICATION_LSB_OFFSET 0x05U    /*!< Attribute authentication offset */


static uint8_t wifiConfigToken1[] = {
  0x10, 0x4A, /* Attribute ID: Version       */
  0x00, 0x01, /* Attribute ID Length         */
  0x10,       /* Version 1.0                 */
  0x10, 0x0E, /* Attribute ID Credential     */
  0x00, 0x48, /* Attribute ID Length         */
  0x10, 0x26, /* Attribute ID: Network Index */
  0x00, 0x01, /* Attribute Length            */
  0x01,       /* Index                       */
  0x10, 0x45  /* Attribute ID: SSID          */
};

static uint8_t wifiConfigToken3[] = {
  0x10, 0x03, /* Attribute ID:Authentication Type */
  0x00, 0x02, /* Attribute Length                 */
  0x00, 0x01, /* Attribute Type: Open             */
  0x10, 0x0F, /* Attribute ID: Encryption Type    */
  0x00, 0x02, /* Attribute Length                 */
  0x00, 0x01, /* Encryption Type: None            */
  0x10, 0x27  /* Attribute ID: Network Key        */
};

static uint8_t wifiConfigToken5[] = {
  0x10, 0x20,       /* Attribute ID: MAC Address           */
  0x00, 0x06,       /* Attribute Length                    */
  0,                /* MAC-ADDRESS                         */
  0,                /* MAC-ADDRESS                         */
  0,                /* MAC-ADDRESS                         */
  0,                /* MAC-ADDRESS                         */
  0,                /* MAC-ADDRESS                         */
  0,                /* MAC-ADDRESS                         */
  0x10, 0x49,       /* Attribute ID: Vendor Extension      */
  0x00, 0x06,       /* Attribute Length                    */
  0x00, 0x37, 0x2A, /* Vendor ID: WFA                      */
  0x02,             /* Subelement ID:Network Key Shareable */
  0x01,             /* Subelement Length                   */
  0x01,             /* Network Key Shareable: TRUE         */
  0x10, 0x49,       /* Attribute ID: Vendor Extension      */
  0x00, 0x06,       /* Attribute Length                    */
  0x00, 0x37, 0x2A, /* Vendor ID: WFA                      */
  0x00,             /* Subelement ID: Version2             */
  0x01,             /* Subelement Length: 1                */
  0x20              /* Version 2                           */
};


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
/*! Manage a Wifi Out-Of-Band NDEF message, to start a communication based on Wifi.
 *  The Wifi OOB format is described by the Wifi Protected Setup specification.
 *  It consists in a list of data elements formatted as type-length-value.

    The Wifi OOB in a NDEF record has the following structure:
        - Version
        - Credential
            - Network index
            - SSID
            - Authentication Type
            - Encryption Type
            - Network Key
            - MAC Address
            - Vendor Extension
                - Network Key Shareable
            - Vendor Extension
                - Version2

    Note: If the `Network key` is set to an empty buffer, the library sets it to "0x00000000"
          Even if 0-length Network Key is supposed to be supported, smartphones dont necessarily accept it.
  */


/*****************************************************************************/
static uint32_t ndefWifiPayloadGetLength(const ndefType *wifi)
{
  const ndefTypeWifi *wifiData;
  uint32_t payloadLength;

  if ((wifi == NULL) || (wifi->id != NDEF_TYPE_ID_MEDIA_WIFI)) {
    return 0;
  }

  wifiData = &wifi->data.wifi;

  payloadLength = sizeof(wifiConfigToken1)
                  + WIFI_SSID_TYPE_LENGTH    + wifiData->bufNetworkSSID.length
                  + sizeof(wifiConfigToken3)
                  + WIFI_SSID_KEY_TYPE_LENGTH + wifiData->bufNetworkKey.length
                  + sizeof(wifiConfigToken5);

  return payloadLength;
}


/*****************************************************************************/
static const uint8_t *ndefWifiToPayloadItem(const ndefType *wifi, ndefConstBuffer *bufItem, bool begin)
{
  static uint32_t item = 0;
  const ndefTypeWifi *wifiData;
  uint16_t credentialLength;

  uint8_t defaultKey[4] = {0, 0, 0, 0};
  ndefConstBuffer8 bufDefaultNetworkKey = { defaultKey, sizeof(defaultKey) };

  const uint8_t CONFIG_TOKEN_1_CREDENTIAL_LENGTH_INDEX   =  7U;
  const uint8_t CONFIG_TOKEN_3_AUTHENTICATION_TYPE_INDEX =  5U;
  const uint8_t CONFIG_TOKEN_3_ENCRYPTION_TYPE_INDEX     = 11U;

  static uint8_t zero[] = { 0 };
  static ndefConstBuffer8 bufZero = { zero, sizeof(zero) };

  if ((wifi    == NULL) || (wifi->id != NDEF_TYPE_ID_MEDIA_WIFI) ||
      (bufItem == NULL)) {
    return NULL;
  }

  wifiData = &wifi->data.wifi;

  if (begin == true) {
    item = 0;
  }

  bufItem->buffer = NULL;
  bufItem->length = 0;

  switch (item) {
    case 0:
      /* Config Token1 */

      /* Update Token1 with credential length */
      credentialLength = (uint16_t)(5U +                   /* Network index      */
                                    2U +                              /* SSID type          */
                                    2U +                              /* SSID key length    */
                                    wifiData->bufNetworkSSID.length + /* SSID key           */
                                    sizeof(wifiConfigToken3) +        /* Token3 length      */
                                    2U +                              /* Network key length */
                                    wifiData->bufNetworkKey.length +  /* Network key        */
                                    sizeof(wifiConfigToken5));        /* Token5 length      */

      wifiConfigToken1[CONFIG_TOKEN_1_CREDENTIAL_LENGTH_INDEX]      = (uint8_t)(credentialLength >>    8U);
      wifiConfigToken1[CONFIG_TOKEN_1_CREDENTIAL_LENGTH_INDEX + 1U] = (uint8_t)(credentialLength  & 0xFFU);

      bufItem->buffer = wifiConfigToken1;
      bufItem->length = sizeof(wifiConfigToken1);
      break;

    case 1:
      /* SSID Length (1st byte) */
      bufItem->buffer = bufZero.buffer;
      bufItem->length = bufZero.length;
      break;

    case 2:
      /* SSID Length (2nd byte) */
      bufItem->buffer = (const uint8_t *)&wifiData->bufNetworkSSID.length;
      bufItem->length = 1U;
      break;

    case 3:
      /* SSID Value */
      bufItem->buffer = wifiData->bufNetworkSSID.buffer;
      bufItem->length = wifiData->bufNetworkSSID.length;
      break;

    case 4:
      /* Config Token3 */

      /* Update Token3 with Authentication and Encryption Types */
      wifiConfigToken3[CONFIG_TOKEN_3_AUTHENTICATION_TYPE_INDEX] = wifiData->authentication;
      wifiConfigToken3[CONFIG_TOKEN_3_ENCRYPTION_TYPE_INDEX]     = wifiData->encryption;

      bufItem->buffer = wifiConfigToken3;
      bufItem->length = sizeof(wifiConfigToken3);
      break;

    case 5:
      /* SSID Key Length (1st byte) */
      bufItem->buffer = bufZero.buffer;
      bufItem->length = bufZero.length;
      break;

    case 6:
      /* SSID Key Length (2 bytes) */
      bufItem->buffer = (const uint8_t *)&wifiData->bufNetworkKey.length;
      bufItem->length = 1U;
      break;

    case 7:
      /* SSID Key Value */
      if (wifiData->bufNetworkKey.length == 0U) {
        /* Empty network key is not supported by Phones */
        bufItem->buffer = bufDefaultNetworkKey.buffer;
        bufItem->length = bufDefaultNetworkKey.length;
      } else {
        bufItem->buffer = wifiData->bufNetworkKey.buffer;
        bufItem->length = wifiData->bufNetworkKey.length;
      }
      break;

    case 8:
      /* Config Token5 */
      bufItem->buffer = wifiConfigToken5;
      bufItem->length = sizeof(wifiConfigToken5);
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
ReturnCode ndefWifiInit(ndefType *wifi, const ndefTypeWifi *wifiConfig)
{
  ndefTypeWifi *wifiData;

  if ((wifi == NULL) || (wifiConfig == NULL)) {
    return ERR_PARAM;
  }

  wifi->id               = NDEF_TYPE_ID_MEDIA_WIFI;
  wifi->getPayloadLength = ndefWifiPayloadGetLength;
  wifi->getPayloadItem   = ndefWifiToPayloadItem;
  wifi->typeToRecord     = ndefWifiToRecord;
  wifiData               = &wifi->data.wifi;

  wifiData->bufNetworkSSID = wifiConfig->bufNetworkSSID;
  wifiData->bufNetworkKey  = wifiConfig->bufNetworkKey;
  wifiData->authentication = wifiConfig->authentication;
  wifiData->encryption     = wifiConfig->encryption;

  return ERR_NONE;
}


/*****************************************************************************/
ReturnCode ndefGetWifi(const ndefType *wifi, ndefTypeWifi *wifiConfig)
{
  const ndefTypeWifi *wifiData;

  if ((wifi       == NULL) || (wifi->id != NDEF_TYPE_ID_MEDIA_WIFI) ||
      (wifiConfig == NULL)) {
    return ERR_PARAM;
  }

  wifiData = &wifi->data.wifi;

  wifiConfig->bufNetworkSSID.buffer = wifiData->bufNetworkSSID.buffer;
  wifiConfig->bufNetworkSSID.length = wifiData->bufNetworkSSID.length;
  wifiConfig->bufNetworkKey.buffer  = wifiData->bufNetworkKey.buffer;
  wifiConfig->bufNetworkKey.length  = wifiData->bufNetworkKey.length;
  wifiConfig->authentication = wifiData->authentication;
  wifiConfig->encryption     = wifiData->encryption;

  return ERR_NONE;
}


/*****************************************************************************/
static ReturnCode ndefPayloadToWifi(const ndefConstBuffer *bufPayload, ndefType *wifi)
{
  ndefTypeWifi wifiConfig;
  uint32_t offset;

  if ((bufPayload == NULL) || (bufPayload->buffer == NULL) ||
      (wifi       == NULL)) {
    return ERR_PARAM;
  }

  wifiConfig.bufNetworkSSID.buffer = NULL;
  wifiConfig.bufNetworkSSID.length = 0;
  wifiConfig.bufNetworkKey.buffer  = NULL;
  wifiConfig.bufNetworkKey.length  = 0;
  wifiConfig.authentication        = 0;
  wifiConfig.encryption            = 0;

  offset = 0;
  while (offset < bufPayload->length) {
    uint8_t attribute = bufPayload->buffer[offset];
    if (attribute == NDEF_WIFI_ATTRIBUTE_ID_SSID_LSB) {
      uint8_t data1   = bufPayload->buffer[offset + NDEF_WIFI_ATTRIBUTE_LENGTH_MSB_OFFSET];
      uint8_t data2   = bufPayload->buffer[offset + NDEF_WIFI_ATTRIBUTE_LENGTH_LSB_OFFSET];
      uint32_t length = ((uint32_t)data1 << 8U) | data2;

      switch (bufPayload->buffer[offset + NDEF_WIFI_ATTRIBUTE_ID_OFFSET]) {
        case NDEF_WIFI_ATTRIBUTE_ID_SSID_MSB:
          /* Network SSID */
          if (length > NDEF_WIFI_NETWORK_SSID_LENGTH) {
            return ERR_PROTO;
          }
          wifiConfig.bufNetworkSSID.buffer = &bufPayload->buffer[offset + NDEF_WIFI_ATTRIBUTE_DATA_OFFSET];
          wifiConfig.bufNetworkSSID.length = length;
          offset += (NDEF_WIFI_ATTRIBUTE_DATA_OFFSET + length);
          break;
        case NDEF_WIFI_ATTRIBUTE_ID_NETWORK_MSB:
          /* Network key */
          if (length > NDEF_WIFI_NETWORK_KEY_LENGTH) {
            return ERR_PROTO;
          }
          wifiConfig.bufNetworkKey.buffer = &bufPayload->buffer[offset + NDEF_WIFI_ATTRIBUTE_DATA_OFFSET];
          wifiConfig.bufNetworkKey.length = length;
          offset += (NDEF_WIFI_ATTRIBUTE_DATA_OFFSET + length);
          break;
        case NDEF_WIFI_ATTRIBUTE_AUTHENTICATION:
          /* Authentication */
          if (length != NDEF_WIFI_AUTHENTICATION_TYPE_LENGTH) {
            return ERR_PROTO;
          }
          wifiConfig.authentication = bufPayload->buffer[offset + NDEF_WIFI_ATTRIBUTE_AUTHENTICATION_LSB_OFFSET];
          offset += (NDEF_WIFI_ATTRIBUTE_DATA_OFFSET + length);
          break;
        case NDEF_WIFI_ATTRIBUTE_ENCRYPTION:
          /* Encryption */
          if (length != NDEF_WIFI_ENCRYPTION_TYPE_LENGTH) {
            return ERR_PROTO;
          }
          wifiConfig.encryption = bufPayload->buffer[offset + NDEF_WIFI_ATTRIBUTE_ENCRYPTION_LSB_OFFSET];
          offset += (NDEF_WIFI_ATTRIBUTE_DATA_OFFSET + length);
          break;
        default:
          offset++;
          break;
      }
    } else {
      offset++;
    }
  }

  return ndefWifiInit(wifi, &wifiConfig);
}


/*****************************************************************************/
ReturnCode ndefRecordToWifi(const ndefRecord *record, ndefType *wifi)
{
  const ndefType *type;

  if ((record == NULL) || (wifi == NULL)) {
    return ERR_PARAM;
  }

  if (! ndefRecordTypeMatch(record, NDEF_TNF_MEDIA_TYPE, &bufMediaTypeWifi)) { /* "application/vnd.wfa.wsc" */
    return ERR_PROTO;
  }

  type = ndefRecordGetNdefType(record);
  if ((type != NULL) && (type->id == NDEF_TYPE_ID_MEDIA_WIFI)) {
    (void)ST_MEMCPY(wifi, type, sizeof(ndefType));
    return ERR_NONE;
  }

  return ndefPayloadToWifi(&record->bufPayload, wifi);
}


/*****************************************************************************/
ReturnCode ndefWifiToRecord(const ndefType *wifi, ndefRecord *record)
{
  if ((wifi   == NULL) || (wifi->id != NDEF_TYPE_ID_MEDIA_WIFI) ||
      (record == NULL)) {
    return ERR_PARAM;
  }

  (void)ndefRecordReset(record);

  (void)ndefRecordSetType(record, NDEF_TNF_MEDIA_TYPE, &bufMediaTypeWifi);

  if (ndefRecordSetNdefType(record, wifi) != ERR_NONE) {
    return ERR_PARAM;
  }

  return ERR_NONE;
}

#endif
