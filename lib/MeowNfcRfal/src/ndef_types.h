
/**
  ******************************************************************************
  * @file           : ndef_types.h
  * @brief          : Common NDEF RTD (well-known and external) and Media types header file
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

#ifndef NDEF_TYPES_H
#define NDEF_TYPES_H



/*
 ******************************************************************************
 * INCLUDES
 ******************************************************************************
 */

#include "ndef_config.h"
#include "ndef_record.h"
#include "ndef_buffer.h"

/* RTD types */
#if NDEF_TYPE_EMPTY_SUPPORT
  #include "ndef_type_empty.h"
#endif
#if NDEF_TYPE_FLAT_SUPPORT
  #include "ndef_type_flat.h"
#endif
#if NDEF_TYPE_RTD_DEVICE_INFO_SUPPORT
  #include "ndef_type_deviceinfo.h"
#endif
#if NDEF_TYPE_RTD_TEXT_SUPPORT
  #include "ndef_type_text.h"
#endif
#if NDEF_TYPE_RTD_URI_SUPPORT
  #include "ndef_type_uri.h"
#endif
#if NDEF_TYPE_RTD_AAR_SUPPORT
  #include "ndef_type_aar.h"
#endif
#if NDEF_TYPE_RTD_WLC_SUPPORT
  #include "ndef_type_wlc.h"
#endif
#if NDEF_TYPE_RTD_WPCWLC_SUPPORT
  #include "ndef_type_wpcwlc.h"
#endif
#if NDEF_TYPE_RTD_TNEP_SUPPORT
  #include "ndef_type_tnep.h"
#endif

/* MIME types */
#if NDEF_TYPE_MEDIA_SUPPORT
  #include "ndef_type_media.h"
#endif
#if NDEF_TYPE_BLUETOOTH_SUPPORT
  #include "ndef_type_bluetooth.h"
#endif
#if NDEF_TYPE_VCARD_SUPPORT
  #include "ndef_type_vcard.h"
#endif
#if NDEF_TYPE_WIFI_SUPPORT
  #include "ndef_type_wifi.h"
#endif


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


/*****************************************************************************/

/*! NDEF Type Id enum */
typedef enum {
  NDEF_TYPE_ID_NONE = 0,
  NDEF_TYPE_ID_FLAT,
  NDEF_TYPE_ID_EMPTY,
  NDEF_TYPE_ID_RTD_DEVICE_INFO,
  NDEF_TYPE_ID_RTD_TEXT,
  NDEF_TYPE_ID_RTD_URI,
  NDEF_TYPE_ID_RTD_AAR,
  NDEF_TYPE_ID_RTD_WLCCAP,
  NDEF_TYPE_ID_RTD_WLCSTAI,
  NDEF_TYPE_ID_RTD_WLCINFO,
  NDEF_TYPE_ID_RTD_WLCCTL,
  NDEF_TYPE_ID_RTD_WPCWLC,
  NDEF_TYPE_ID_RTD_TNEP_SERVICE_PARAMETER,
  NDEF_TYPE_ID_RTD_TNEP_SERVICE_SELECT,
  NDEF_TYPE_ID_RTD_TNEP_STATUS,
  NDEF_TYPE_ID_MEDIA,
  NDEF_TYPE_ID_BLUETOOTH_BREDR,
  NDEF_TYPE_ID_BLUETOOTH_LE,
  NDEF_TYPE_ID_BLUETOOTH_SECURE_BREDR,
  NDEF_TYPE_ID_BLUETOOTH_SECURE_LE,
  NDEF_TYPE_ID_MEDIA_VCARD,
  NDEF_TYPE_ID_MEDIA_WIFI,
  NDEF_TYPE_ID_COUNT        /* Keep this one last */
} ndefTypeId;


/*! NDEF abstraction Struct */
struct ndefTypeStruct {
  ndefTypeId      id;                                       /*!< Type Id           */
  uint32_t (*getPayloadLength)(const ndefType *type);       /*!< Return payload length, specific to each type */
  const uint8_t *(*getPayloadItem)(const ndefType *type, ndefConstBuffer *item, bool begin); /*!< Payload Encoder, specific to each type */
  ReturnCode(*typeToRecord)(const ndefType *type, ndefRecord *record);      /*!< Type to Record convert function */
  union {
#if NDEF_TYPE_FLAT_SUPPORT
    ndefConstBuffer           bufPayload;       /*!< Flat/unknown type    */
#endif
#if NDEF_TYPE_RTD_DEVICE_INFO_SUPPORT
    ndefTypeRtdDeviceInfo     deviceInfo;       /*!< Device Information   */
#endif
#if NDEF_TYPE_RTD_TEXT_SUPPORT
    ndefTypeRtdText           text;             /*!< Text                 */
#endif
#if NDEF_TYPE_RTD_URI_SUPPORT
    ndefTypeRtdUri            uri;              /*!< URI                  */
#endif
#if NDEF_TYPE_RTD_AAR_SUPPORT
    ndefTypeRtdAar            aar;              /*!< AAR                  */
#endif
#if NDEF_TYPE_RTD_WLC_SUPPORT
    ndefTypeRtdWlcCapability  wlcCapability; /*!< WLC Capability          */
    ndefTypeRtdWlcStatusInfo  wlcStatusInfo; /*!< WLC Status and Info     */
    ndefTypeRtdWlcPollInfo    wlcPollInfo;   /*!< WLC Poll Information    */
    ndefTypeRtdWlcListenCtl   wlcListenCtl;  /*!< WLC Listen Control      */
#endif
#if NDEF_TYPE_RTD_WPCWLC_SUPPORT
    ndefTypeRtdWpcWlc         wpcWlc;           /*!< WPC WLC              */
#endif
#if NDEF_TYPE_RTD_TNEP_SUPPORT
    ndefTypeRtdTnepServiceParameter tnepServiceParameter; /*!< TNEP Service Parameter */
    ndefTypeRtdTnepServiceSelect    tnepServiceSelect;    /*!< TNEP Service Select    */
    ndefTypeRtdTnepStatus           tnepStatus;           /*!< TNEP Status            */
#endif
#if NDEF_TYPE_MEDIA_SUPPORT
    ndefTypeMedia             media;            /*!< Media                */
#endif
#if NDEF_TYPE_BLUETOOTH_SUPPORT
    ndefTypeBluetooth         bluetooth;        /*!< Bluetooth            */
#endif
#if NDEF_TYPE_VCARD_SUPPORT
    ndefTypeVCard             vCard;            /*!< vCard                */
#endif
#if NDEF_TYPE_WIFI_SUPPORT
    ndefTypeWifi              wifi;             /*!< Wifi                 */
#endif
    uint8_t                   reserved;         /*!< Non-conditional field to avoid empty union when all types are disabled */
  } data;                      /*!< Type data union                         */
};


/*
 ******************************************************************************
 * GLOBAL FUNCTION PROTOTYPES
 ******************************************************************************
 */



/*!
 *****************************************************************************
 * Convert a record to a supported type
 *
 * \param[in]  record: Record to read
 * \param[out] type:   Type to store the converted record
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRecordToType(const ndefRecord *record, ndefType *type);


/*!
 *****************************************************************************
 * Convert a supported type to a record
 *
 * \param[in]  type:    Pointer to the type to read data from
 * \param[out] record:  Record to fill
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefTypeToRecord(const ndefType *type, ndefRecord *record);


/*!
 *****************************************************************************
 * Set the NDEF specific structure to process NDEF types
 *
 * \param[in]  record: Record
 * \param[out] type:   NDEF type structure
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRecordSetNdefType(ndefRecord *record, const ndefType *type);


/*!
 *****************************************************************************
 * Get the NDEF type structure of this record
 *
 * \param[in] record: Record
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
const ndefType *ndefRecordGetNdefType(const ndefRecord *record);



#endif /* NDEF_TYPES_H */

/**
  * @}
  *
  */
