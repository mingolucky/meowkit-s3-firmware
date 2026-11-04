
/**
  ******************************************************************************
  * @file           : ndef_type_uri.h
  * @brief          : NDEF RTD (well-known and external) types header file
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


#ifndef NDEF_TYPE_URI_H
#define NDEF_TYPE_URI_H



/*
 ******************************************************************************
 * INCLUDES
 ******************************************************************************
 */

#include "ndef_record.h"
#include "ndef_buffer.h"


/*
 ******************************************************************************
 * GLOBAL DEFINES
 ******************************************************************************
 */


/*! RTD Type URI Protocols */
#define NDEF_URI_PREFIX_NONE          0x00U    /*!< No URI Protocol               */
#define NDEF_URI_PREFIX_HTTP_WWW      0x01U    /*!< URI Protocol http://www.      */
#define NDEF_URI_PREFIX_HTTPS_WWW     0x02U    /*!< URI Protocol https://www.     */
#define NDEF_URI_PREFIX_HTTP          0x03U    /*!< URI Protocol http://          */
#define NDEF_URI_PREFIX_HTTPS         0x04U    /*!< URI Protocol https://         */
#define NDEF_URI_PREFIX_TEL           0x05U    /*!< URI Protocol tel:             */
#define NDEF_URI_PREFIX_MAILTO        0x06U    /*!< URI Protocol mailto:          */
#define NDEF_URI_PREFIX_FTP_ANONYMOUS 0x07U    /*!< URI Protocol ftp://anonymous@ */
#define NDEF_URI_PREFIX_FTP_FTP       0x08U    /*!< URI Protocol ftp://ftp.       */
#define NDEF_URI_PREFIX_FTPS          0x09U    /*!< URI Protocol ftps://          */
#define NDEF_URI_PREFIX_SFTP          0x0AU    /*!< URI Protocol sftp://          */
#define NDEF_URI_PREFIX_SMB           0x0BU    /*!< URI Protocol smb://           */
#define NDEF_URI_PREFIX_NFS           0x0CU    /*!< URI Protocol nfs://           */
#define NDEF_URI_PREFIX_FTP           0x0DU    /*!< URI Protocol ftp://           */
#define NDEF_URI_PREFIX_DAV           0x0EU    /*!< URI Protocol dav://           */
#define NDEF_URI_PREFIX_NEWS          0x0FU    /*!< URI Protocol news:            */
#define NDEF_URI_PREFIX_TELNET        0x10U    /*!< URI Protocol telnet://        */
#define NDEF_URI_PREFIX_IMAP          0x11U    /*!< URI Protocol imap:            */
#define NDEF_URI_PREFIX_RTSP          0x12U    /*!< URI Protocol rtsp://          */
#define NDEF_URI_PREFIX_URN           0x13U    /*!< URI Protocol urn:             */
#define NDEF_URI_PREFIX_POP           0x14U    /*!< URI Protocol pop:             */
#define NDEF_URI_PREFIX_SIP           0x15U    /*!< URI Protocol sip:             */
#define NDEF_URI_PREFIX_SIPS          0x16U    /*!< URI Protocol sips:            */
#define NDEF_URI_PREFIX_TFTP          0x17U    /*!< URI Protocol tftp:            */
#define NDEF_URI_PREFIX_BTSPP         0x18U    /*!< URI Protocol btspp://         */
#define NDEF_URI_PREFIX_BTL2CAP       0x19U    /*!< URI Protocol btl2cap://       */
#define NDEF_URI_PREFIX_BTGOEP        0x1AU    /*!< URI Protocol btgoep://        */
#define NDEF_URI_PREFIX_TCPOBEX       0x1BU    /*!< URI Protocol tcpobex://       */
#define NDEF_URI_PREFIX_IRDAOBEX      0x1CU    /*!< URI Protocol irdaobex://      */
#define NDEF_URI_PREFIX_FILE          0x1DU    /*!< URI Protocol file://          */
#define NDEF_URI_PREFIX_URN_EPC_ID    0x1EU    /*!< URI Protocol urn:epc:id:      */
#define NDEF_URI_PREFIX_URN_EPC_TAG   0x1FU    /*!< URI Protocol urn:epc:tag      */
#define NDEF_URI_PREFIX_URN_EPC_PAT   0x20U    /*!< URI Protocol urn:epc:pat:     */
#define NDEF_URI_PREFIX_URN_EPC_RAW   0x21U    /*!< URI Protocol urn:epc:raw:     */
#define NDEF_URI_PREFIX_URN_EPC       0x22U    /*!< URI Protocol urn:epc:         */
#define NDEF_URI_PREFIX_URN_NFC       0x23U    /*!< URI Protocol urn:nfc:         */
#define NDEF_URI_PREFIX_AUTODETECT    0x24U    /*!< ST Protocol Autodetect        */
#define NDEF_URI_PREFIX_COUNT         0x25U    /*!< Number of URI protocols       */


/*
 ******************************************************************************
 * GLOBAL TYPES
 ******************************************************************************
 */

/*! RTD URI Record Type buffer */
extern const ndefConstBuffer8 bufRtdTypeUri;        /*! URI Record Type buffer                              */

/*! RTD Type URI */
typedef struct {
  uint8_t         protocol;     /*!< Protocol Identifier */
  ndefConstBuffer bufUriString; /*!< URI string buffer   */
} ndefTypeRtdUri;

/*
 ******************************************************************************
 * GLOBAL FUNCTION PROTOTYPES
 ******************************************************************************
 */


/***************
 * URI
 ***************
 */

/*!
 *****************************************************************************
 * Initialize a URI RTD type
 *
 * \param[out] uri:          Type to initialize
 * \param[in]  protocol:     URI protocol
 * \param[in]  bufUriString: URI string buffer
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRtdUriInit(ndefType *uri, uint8_t protocol, const ndefConstBuffer *bufUriString);


/*!
 *****************************************************************************
 * Get URI RTD type content
 *
 * \param[in]  uri:          Type to get information from
 * \param[out] bufProtocol:  URI protocol buffer
 * \param[out] bufUriString: URI string buffer
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefGetRtdUri(const ndefType *uri, ndefConstBuffer *bufProtocol, ndefConstBuffer *bufUriString);


/*!
 *****************************************************************************
 * Convert an NDEF record to a URI RTD type
 *
 * \param[in]  record: Record to convert
 * \param[out] uri:    The converted type
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRecordToRtdUri(const ndefRecord *record, ndefType *uri);


/*!
 *****************************************************************************
 * Convert a URI RTD type to an NDEF record
 *
 * \param[in]  uri:    Type to convert
 * \param[out] record: The converted type
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRtdUriToRecord(const ndefType *uri, ndefRecord *record);



#endif /* NDEF_TYPE_URI_H */

/**
  * @}
  *
  */
