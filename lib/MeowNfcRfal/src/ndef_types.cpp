
/**
  ******************************************************************************
  * @file           : ndef_types.cpp
  * @brief          :NDEF RTD and MIME types
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
#include "nfc_utils.h"


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

/*! NDEF type table to associate a TNF, type and the recordToType function pointers */
typedef struct {
  uint8_t                 tnf;           /*!< TNF                */
  const ndefConstBuffer8 *bufTypeString; /*!< Type String buffer */
  ReturnCode(*recordToType)(const ndefRecord *record, ndefType *type);  /*!< Pointer to read function  */
} ndefTypeConverter;


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


/*****************************************************************************/
ReturnCode ndefRecordToType(const ndefRecord *record, ndefType *type)
{
#if NDEF_TYPE_EMPTY_SUPPORT
  /*! Empty string */
  static const uint8_t    ndefTypeEmpty[] = "";    /*!< Empty string */
  static ndefConstBuffer8 bufTypeEmpty    = { ndefTypeEmpty, sizeof(ndefTypeEmpty) - 1U };
#endif

  /*! Array to match RTD strings with Well-known types, and converting functions */
  static const ndefTypeConverter typeConverterTable[] = {
#if NDEF_TYPE_EMPTY_SUPPORT
    { NDEF_TNF_EMPTY,               &bufTypeEmpty,            ndefRecordToEmptyType        },
#endif
#if NDEF_TYPE_RTD_DEVICE_INFO_SUPPORT
    { NDEF_TNF_RTD_WELL_KNOWN_TYPE, &bufRtdTypeDeviceInfo,    ndefRecordToRtdDeviceInfo    },
#endif
#if NDEF_TYPE_RTD_TEXT_SUPPORT
    { NDEF_TNF_RTD_WELL_KNOWN_TYPE, &bufRtdTypeText,          ndefRecordToRtdText          },
#endif
#if NDEF_TYPE_RTD_URI_SUPPORT
    { NDEF_TNF_RTD_WELL_KNOWN_TYPE, &bufRtdTypeUri,           ndefRecordToRtdUri           },
#endif
#if NDEF_TYPE_RTD_AAR_SUPPORT
    { NDEF_TNF_RTD_EXTERNAL_TYPE,   &bufRtdTypeAar,           ndefRecordToRtdAar           },
#endif
#if NDEF_TYPE_RTD_WLC_SUPPORT
    { NDEF_TNF_RTD_WELL_KNOWN_TYPE, &bufTypeRtdWlcCapability, ndefRecordToRtdWlcCapability },
    { NDEF_TNF_RTD_WELL_KNOWN_TYPE, &bufTypeRtdWlcStatusInfo, ndefRecordToRtdWlcStatusInfo },
    { NDEF_TNF_RTD_WELL_KNOWN_TYPE, &bufTypeRtdWlcPollInfo,   ndefRecordToRtdWlcPollInfo   },
    { NDEF_TNF_RTD_WELL_KNOWN_TYPE, &bufTypeRtdWlcListenCtl,  ndefRecordToRtdWlcListenCtl  },
#endif
#if NDEF_TYPE_RTD_WPCWLC_SUPPORT
    { NDEF_TNF_RTD_EXTERNAL_TYPE,   &bufRtdTypeWpcWlc,        ndefRecordToRtdWpcWlc        },
#endif
#if NDEF_TYPE_RTD_TNEP_SUPPORT
    { NDEF_TNF_RTD_WELL_KNOWN_TYPE, &bufRtdTypeTnepServiceParameter, ndefRecordToRtdTnepServiceParameter },
    { NDEF_TNF_RTD_WELL_KNOWN_TYPE, &bufRtdTypeTnepServiceSelect,    ndefRecordToRtdTnepServiceSelect    },
    { NDEF_TNF_RTD_WELL_KNOWN_TYPE, &bufRtdTypeTnepStatus,           ndefRecordToRtdTnepStatus           },
#endif
#if NDEF_TYPE_BLUETOOTH_SUPPORT
    { NDEF_TNF_MEDIA_TYPE, &bufMediaTypeBluetoothBrEdr,       ndefRecordToBluetooth        },
    { NDEF_TNF_MEDIA_TYPE, &bufMediaTypeBluetoothLe,          ndefRecordToBluetooth        },
    { NDEF_TNF_MEDIA_TYPE, &bufMediaTypeBluetoothSecureBrEdr, ndefRecordToBluetooth        },
    { NDEF_TNF_MEDIA_TYPE, &bufMediaTypeBluetoothSecureLe,    ndefRecordToBluetooth        },
#endif
#if NDEF_TYPE_VCARD_SUPPORT
    { NDEF_TNF_MEDIA_TYPE,          &bufMediaTypeVCard,       ndefRecordToVCard            },
#endif
#if NDEF_TYPE_WIFI_SUPPORT
    { NDEF_TNF_MEDIA_TYPE,          &bufMediaTypeWifi,        ndefRecordToWifi             },
#endif
    /* Non-conditional field to avoid empty union when all types are disabled */
    { 0,                            NULL,                     NULL                         }
  };

  const ndefType *ndefData;

  if (type == NULL) {
    return ERR_PARAM;
  }

  ndefData = ndefRecordGetNdefType(record);
  if (ndefData != NULL) {
    /* Return the well-known type contained in the record */
    (void)ST_MEMCPY(type, ndefData, sizeof(ndefType));
    return ERR_NONE;
  }

  for (int32_t i = 0; i < (int32_t)SIZEOF_ARRAY(typeConverterTable); i++) {
    if (ndefRecordTypeMatch(record, typeConverterTable[i].tnf, typeConverterTable[i].bufTypeString)) {
      /* Call the appropriate function to the matching type */
      if (typeConverterTable[i].recordToType != NULL) {
        return typeConverterTable[i].recordToType(record, type);
      }
    }
  }

#if NDEF_TYPE_FLAT_SUPPORT
  return ndefRecordToFlatPayloadType(record, type);
#else
  return ERR_NOT_IMPLEMENTED;
#endif
}


/*****************************************************************************/
ReturnCode ndefTypeToRecord(const ndefType *type, ndefRecord *record)
{
  if (type == NULL) {
    return ERR_PARAM;
  }

  if (type->typeToRecord != NULL) {
    return type->typeToRecord(type, record);
  }

  return ERR_NOT_IMPLEMENTED;
}


/*****************************************************************************/
ReturnCode ndefRecordSetNdefType(ndefRecord *record, const ndefType *type)
{
  uint32_t payloadLength;

  if ((record == NULL) ||
      (type                   == NULL)               ||
      (type->id               == NDEF_TYPE_ID_NONE)  ||
      (type->id                > NDEF_TYPE_ID_COUNT) ||
      (type->getPayloadLength == NULL)               ||
      (type->getPayloadItem   == NULL)               ||
      (type->typeToRecord     == NULL)) {
    return ERR_PARAM;
  }

  record->ndeftype = type;

  /* Set Short Record bit accordingly */
  payloadLength = ndefRecordGetPayloadLength(record);
  ndefHeaderSetValueSR(record, (payloadLength <= NDEF_SHORT_RECORD_LENGTH_MAX) ? 1 : 0);

  return ERR_NONE;
}


/*****************************************************************************/
const ndefType *ndefRecordGetNdefType(const ndefRecord *record)
{
  if (record == NULL) {
    return NULL;
  }

  /* Check whether it is a valid NDEF type */
  if ((record->ndeftype != NULL) &&
      (record->ndeftype->id               != NDEF_TYPE_ID_NONE)  &&
      (record->ndeftype->id                < NDEF_TYPE_ID_COUNT) &&
      (record->ndeftype->getPayloadItem   != NULL)               &&
      (record->ndeftype->getPayloadLength != NULL)               &&
      (record->ndeftype->typeToRecord     != NULL)) {
    return record->ndeftype;
  }

  return NULL;
}
