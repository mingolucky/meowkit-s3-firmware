
/**
  ******************************************************************************
  * @file           : ndef_type_uri.cpp
  * @brief          : NDEF RTD URI type
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
#include "ndef_type_uri.h"
#include "nfc_utils.h"


#if NDEF_TYPE_RTD_URI_SUPPORT


/*
 ******************************************************************************
 * GLOBAL DEFINES
 ******************************************************************************
 */


/*! URI defines */
#define NDEF_RTD_URI_PROTOCOL_LEN        1U                        /*!< URI protocol length */
#define NDEF_RTD_URI_PAYLOAD_LENGTH_MIN  (NDEF_RTD_URI_PROTOCOL_LEN + sizeof(uint8_t)) /*!< URI minimum payload length */

/*! URI defines */
#define NDEF_RTD_URI_ID_CODE_OFFSET      0U    /*!< URI Id code offset */
#define NDEF_RTD_URI_FIELD_OFFSET        1U    /*!< URI field offset */


/*
 ******************************************************************************
 * LOCAL VARIABLES
 ******************************************************************************
 */


/*! RTD URI Type string */
static const uint8_t ndefRtdTypeUri[]            = "U";               /*!< URI Record Type                {0x55}       */

const ndefConstBuffer8 bufRtdTypeUri             = { ndefRtdTypeUri,        sizeof(ndefRtdTypeUri) - 1U };        /*!< URI Record Type buffer                */


/*! URI Type strings */
static const uint8_t ndefUriPrefixNone[]         = "";
static const uint8_t ndefUriPrefixHttpWww[]      = "http://www.";
static const uint8_t ndefUriPrefixHttpsWww[]     = "https://www.";
static const uint8_t ndefUriPrefixHttp[]         = "http://";
static const uint8_t ndefUriPrefixHttps[]        = "https://";
static const uint8_t ndefUriPrefixTel[]          = "tel:";
static const uint8_t ndefUriPrefixMailto[]       = "mailto:";
static const uint8_t ndefUriPrefixFtpAnonymous[] = "ftp://anonymous:anonymous@";
static const uint8_t ndefUriPrefixFtpFtp[]       = "ftp://ftp.";
static const uint8_t ndefUriPrefixFtps[]         = "ftps://";
static const uint8_t ndefUriPrefixSftp[]         = "sftp://";
static const uint8_t ndefUriPrefixSmb[]          = "smb://";
static const uint8_t ndefUriPrefixNfs[]          = "nfs://";
static const uint8_t ndefUriPrefixFtp[]          = "ftp://";
static const uint8_t ndefUriPrefixDav[]          = "dav://";
static const uint8_t ndefUriPrefixNews[]         = "news:";
static const uint8_t ndefUriPrefixTelnet[]       = "telnet://";
static const uint8_t ndefUriPrefixImap[]         = "imap:";
static const uint8_t ndefUriPrefixRtsp[]         = "rtsp://";
static const uint8_t ndefUriPrefixUrn[]          = "urn:";
static const uint8_t ndefUriPrefixPop[]          = "pop:";
static const uint8_t ndefUriPrefixSip[]          = "sip:";
static const uint8_t ndefUriPrefixSips[]         = "sips:";
static const uint8_t ndefUriPrefixTftp[]         = "tftp:";
static const uint8_t ndefUriPrefixBtspp[]        = "btspp://";
static const uint8_t ndefUriPrefixBtl2cap[]      = "btl2cap://";
static const uint8_t ndefUriPrefixBtgoep[]       = "btgoep://";
static const uint8_t ndefUriPrefixTcpobex[]      = "tcpobex://";
static const uint8_t ndefUriPrefixIrdaobex[]     = "irdaobex://";
static const uint8_t ndefUriPrefixFile[]         = "file://";
static const uint8_t ndefUriPrefixUrnEpcId[]     = "urn:epc:id:";
static const uint8_t ndefUriPrefixUrnEpcTag[]    = "urn:epc:tag";
static const uint8_t ndefUriPrefixUrnEpcPat[]    = "urn:epc:pat:";
static const uint8_t ndefUriPrefixUrnEpcRaw[]    = "urn:epc:raw:";
static const uint8_t ndefUriPrefixUrnEpe[]       = "urn:epc:";
static const uint8_t ndefUriPrefixUrnNfc[]       = "urn:nfc:";
static const uint8_t ndefUriPrefixEmpty[]        = ""; /* Autodetect filler */

static const ndefConstBuffer ndefUriPrefix[NDEF_URI_PREFIX_COUNT] = {
  { ndefUriPrefixNone, sizeof(ndefUriPrefixNone) - 1U },
  { ndefUriPrefixHttpWww, sizeof(ndefUriPrefixHttpWww) - 1U },
  { ndefUriPrefixHttpsWww, sizeof(ndefUriPrefixHttpsWww) - 1U },
  { ndefUriPrefixHttp, sizeof(ndefUriPrefixHttp) - 1U },
  { ndefUriPrefixHttps, sizeof(ndefUriPrefixHttps) - 1U },
  { ndefUriPrefixTel, sizeof(ndefUriPrefixTel) - 1U },
  { ndefUriPrefixMailto, sizeof(ndefUriPrefixMailto) - 1U },
  { ndefUriPrefixFtpAnonymous, sizeof(ndefUriPrefixFtpAnonymous) - 1U },
  { ndefUriPrefixFtpFtp, sizeof(ndefUriPrefixFtpFtp) - 1U },
  { ndefUriPrefixFtps, sizeof(ndefUriPrefixFtps) - 1U },
  { ndefUriPrefixSftp, sizeof(ndefUriPrefixSftp) - 1U },
  { ndefUriPrefixSmb, sizeof(ndefUriPrefixSmb) - 1U },
  { ndefUriPrefixNfs, sizeof(ndefUriPrefixNfs) - 1U },
  { ndefUriPrefixFtp, sizeof(ndefUriPrefixFtp) - 1U },
  { ndefUriPrefixDav, sizeof(ndefUriPrefixDav) - 1U },
  { ndefUriPrefixNews, sizeof(ndefUriPrefixNews) - 1U },
  { ndefUriPrefixTelnet, sizeof(ndefUriPrefixTelnet) - 1U },
  { ndefUriPrefixImap, sizeof(ndefUriPrefixImap) - 1U },
  { ndefUriPrefixRtsp, sizeof(ndefUriPrefixRtsp) - 1U },
  { ndefUriPrefixUrn, sizeof(ndefUriPrefixUrn) - 1U },
  { ndefUriPrefixPop, sizeof(ndefUriPrefixPop) - 1U },
  { ndefUriPrefixSip, sizeof(ndefUriPrefixSip) - 1U },
  { ndefUriPrefixSips, sizeof(ndefUriPrefixSips) - 1U },
  { ndefUriPrefixTftp, sizeof(ndefUriPrefixTftp) - 1U },
  { ndefUriPrefixBtspp, sizeof(ndefUriPrefixBtspp) - 1U },
  { ndefUriPrefixBtl2cap, sizeof(ndefUriPrefixBtl2cap) - 1U },
  { ndefUriPrefixBtgoep, sizeof(ndefUriPrefixBtgoep) - 1U },
  { ndefUriPrefixTcpobex, sizeof(ndefUriPrefixTcpobex) - 1U },
  { ndefUriPrefixIrdaobex, sizeof(ndefUriPrefixIrdaobex) - 1U },
  { ndefUriPrefixFile, sizeof(ndefUriPrefixFile) - 1U },
  { ndefUriPrefixUrnEpcId, sizeof(ndefUriPrefixUrnEpcId) - 1U },
  { ndefUriPrefixUrnEpcTag, sizeof(ndefUriPrefixUrnEpcTag) - 1U },
  { ndefUriPrefixUrnEpcPat, sizeof(ndefUriPrefixUrnEpcPat) - 1U },
  { ndefUriPrefixUrnEpcRaw, sizeof(ndefUriPrefixUrnEpcRaw) - 1U },
  { ndefUriPrefixUrnEpe, sizeof(ndefUriPrefixUrnEpe) - 1U },
  { ndefUriPrefixUrnNfc, sizeof(ndefUriPrefixUrnNfc) - 1U },
  { ndefUriPrefixEmpty, sizeof(ndefUriPrefixEmpty) - 1U }
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


/*
 * URI
 */


/*****************************************************************************/
static uint32_t ndefRtdUriPayloadGetLength(const ndefType *uri)
{
  const ndefTypeRtdUri *rtdUri;

  if ((uri == NULL) || (uri->id != NDEF_TYPE_ID_RTD_URI)) {
    return 0;
  }

  rtdUri = &uri->data.uri;

  return sizeof(rtdUri->protocol) + rtdUri->bufUriString.length;
}


/*****************************************************************************/
static const uint8_t *ndefRtdUriToPayloadItem(const ndefType *uri, ndefConstBuffer *bufItem, bool begin)
{
  static uint32_t item = 0;
  const ndefTypeRtdUri *rtdUri;

  if ((uri     == NULL) || (uri->id != NDEF_TYPE_ID_RTD_URI) ||
      (bufItem == NULL)) {
    return NULL;
  }

  rtdUri = &uri->data.uri;

  if (begin == true) {
    item = 0;
  }

  switch (item) {
    case 0:
      /* Protocol byte */
      bufItem->buffer = &rtdUri->protocol;
      bufItem->length = sizeof(rtdUri->protocol);
      break;

    case 1:
      /* URI string */
      bufItem->buffer = rtdUri->bufUriString.buffer;
      bufItem->length = rtdUri->bufUriString.length;
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
static ReturnCode ndefRtdUriProtocolAutodetect(uint8_t *protocol, ndefConstBuffer *bufUriString)
{
  if ((protocol  == NULL)                       ||
      (*protocol != NDEF_URI_PREFIX_AUTODETECT) ||
      (bufUriString == NULL)) {
    return ERR_PARAM;
  }

  for (uint8_t i = 0; i < NDEF_URI_PREFIX_COUNT; i++) { /* Protocol fits in 1 byte => uint8_t */
    if (ndefUriPrefix[i].length > 0U) {
      if (ST_BYTECMP(bufUriString->buffer, ndefUriPrefix[i].buffer, ndefUriPrefix[i].length) == 0) {
        *protocol = i;
        /* Move after the protocol string */
        bufUriString->buffer  = &bufUriString->buffer[ndefUriPrefix[i].length];
        bufUriString->length -= ndefUriPrefix[i].length;
        return ERR_NONE;
      }
    }
  }

  *protocol = NDEF_URI_PREFIX_NONE;

  return ERR_NOTFOUND;
}


/*****************************************************************************/
ReturnCode ndefRtdUriInit(ndefType *uri, uint8_t protocol, const ndefConstBuffer *bufUriString)
{
  ndefTypeRtdUri *rtdUri;
  ndefConstBuffer bufUri;
  uint8_t protocolDetect;

  if ((uri == NULL) || (protocol >= NDEF_URI_PREFIX_COUNT) ||
      (bufUriString == NULL) || (bufUriString->buffer == NULL) || (bufUriString->length == 0U)) {
    return ERR_PARAM;
  }

  uri->id               = NDEF_TYPE_ID_RTD_URI;
  uri->getPayloadLength = ndefRtdUriPayloadGetLength;
  uri->getPayloadItem   = ndefRtdUriToPayloadItem;
  uri->typeToRecord     = ndefRtdUriToRecord;
  rtdUri                = &uri->data.uri;

  bufUri.buffer = bufUriString->buffer;
  bufUri.length = bufUriString->length;
  protocolDetect = protocol;
  if (protocol == NDEF_URI_PREFIX_AUTODETECT) {
    /* Update protocol and URI buffer */
    (void)ndefRtdUriProtocolAutodetect(&protocolDetect, &bufUri);
  }
  rtdUri->protocol = protocolDetect;

  rtdUri->bufUriString.buffer = bufUri.buffer;
  rtdUri->bufUriString.length = bufUri.length;

  return ERR_NONE;
}


/*****************************************************************************/
ReturnCode ndefGetRtdUri(const ndefType *uri, ndefConstBuffer *bufProtocol, ndefConstBuffer *bufUriString)
{
  const ndefTypeRtdUri *rtdUri;

  if ((uri         == NULL) || (uri->id != NDEF_TYPE_ID_RTD_URI) ||
      (bufProtocol == NULL) || (bufUriString == NULL)) {
    return ERR_PARAM;
  }

  rtdUri = &uri->data.uri;

  bufProtocol->buffer   = ndefUriPrefix[rtdUri->protocol].buffer;
  bufProtocol->length   = ndefUriPrefix[rtdUri->protocol].length;

  bufUriString->buffer = rtdUri->bufUriString.buffer;
  bufUriString->length = rtdUri->bufUriString.length;

  return ERR_NONE;
}

/*****************************************************************************/
static ReturnCode ndefPayloadToRtdUri(const ndefConstBuffer *bufUri, ndefType *uri)
{
  uint8_t protocol;

  if ((bufUri == NULL) || (bufUri->buffer == NULL) ||
      (uri    == NULL)) {
    return ERR_PARAM;
  }

  if (bufUri->length < NDEF_RTD_URI_PAYLOAD_LENGTH_MIN) {
    return ERR_PROTO;
  }

  /* Extract info from the payload */
  protocol = bufUri->buffer[NDEF_RTD_URI_ID_CODE_OFFSET];

  ndefConstBuffer bufStringUri;
  bufStringUri.buffer = &bufUri->buffer[NDEF_RTD_URI_FIELD_OFFSET];
  bufStringUri.length =  bufUri->length - sizeof(protocol);

  return ndefRtdUriInit(uri, protocol, &bufStringUri);
}


/*****************************************************************************/
ReturnCode ndefRecordToRtdUri(const ndefRecord *record, ndefType *uri)
{
  const ndefType *type;

  if ((record == NULL) || (uri == NULL)) {
    return ERR_PARAM;
  }

  if (! ndefRecordTypeMatch(record, NDEF_TNF_RTD_WELL_KNOWN_TYPE, &bufRtdTypeUri)) { /* "U" */
    return ERR_PROTO;
  }

  type = ndefRecordGetNdefType(record);
  if ((type != NULL) && (type->id == NDEF_TYPE_ID_RTD_URI)) {
    (void)ST_MEMCPY(uri, type, sizeof(ndefType));
    return ERR_NONE;
  }

  return ndefPayloadToRtdUri(&record->bufPayload, uri);
}


/*****************************************************************************/
ReturnCode ndefRtdUriToRecord(const ndefType *uri, ndefRecord *record)
{
  if ((uri    == NULL) || (uri->id != NDEF_TYPE_ID_RTD_URI) ||
      (record == NULL)) {
    return ERR_PARAM;
  }

  (void)ndefRecordReset(record);

  /* "U" */
  (void)ndefRecordSetType(record, NDEF_TNF_RTD_WELL_KNOWN_TYPE, &bufRtdTypeUri);

  if (ndefRecordSetNdefType(record, uri) != ERR_NONE) {
    return ERR_PARAM;
  }

  return ERR_NONE;
}

#endif
