
/**
  ******************************************************************************
  * @file           : ndef_t4t.cpp
  * @brief          : Provides NDEF methods and definitions to access NFC Forum T4T
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
#include "ndef_poller.h"
#include "ndef_t4t.h"
#include "nfc_utils.h"
#include "ndef_class.h"

/*
 ******************************************************************************
 * ENABLE SWITCH
 ******************************************************************************
 */

#ifndef NDEF_FEATURE_T4T
  #error " NDEF: Module configuration missing. Please enable/disable T4T module by setting: NDEF_FEATURE_T4T"
#endif

#if NDEF_FEATURE_T4T

#if RFAL_FEATURE_ISO_DEP_APDU_MAX_LEN < NDEF_T4T_MIN_APDU_LEN
  #error " RFAL: ISO-DEP APDU Max length must be greater than ISO7816 Command/Response-APDU length. Please increase RFAL_FEATURE_ISO_DEP_APDU_MAX_LEN"
#endif

#ifndef NDEF_FEATURE_FULL_API
  #error " NDEF: Module configuration missing. Please enable/disable Full API by setting: NDEF_FEATURE_FULL_API"
#endif

/*
 ******************************************************************************
 * GLOBAL DEFINES
 ******************************************************************************
 */

#define NDEF_T4T_FID_SIZE              2U        /*!< File Id size                                      */
#define NDEF_T4T_WRITE_ODO_PREFIX_SIZE 7U        /*!< Size of ODO for Write Binary: 54 03 xxyyzz 53 Ld  */

#define NDEF_T4T_DEFAULT_MLC      0x000DU        /*!< Defauit Max Lc value before reading CCFILE values */
#define NDEF_T4T_DEFAULT_MLE      0x000FU        /*!< Defauit Max Le value before reading CCFILE values */

#define NDEF_T4T_OFFSET_MAX       0x7FFFU        /*!< Maximum offset value for ReadBinary               */
#define NDEF_T4T_ODO_OFFSET_MAX 0xFFFFFEU        /*!< Maximum offset value for ReadBinary with ODO      */

#define NDEF_T4T_CCFILEV2_LEN         15U        /*!< CCFILE Length mapping version V2                  */
#define NDEF_T4T_CCFILEV3_LEN         17U        /*!< CCFILE Length mapping version V3                  */

#define NDEF_T4T_NDEF_CTLV_T        0x04U        /*!< NDEF-File_Ctrl_TLV T field value                  */
#define NDEF_T4T_ENDEF_CTLV_T       0x06U        /*!< ENDEF-File_Ctrl_TLV T field value                 */

#define NDEF_T4T_NDEF_CTLV_L        0x06U        /*!< NDEF-File_Ctrl_TLV T field value                  */
#define NDEF_T4T_ENDEF_CTLV_L       0x08U        /*!< ENDEF-File_Ctrl_TLV T field value                 */

#define NDEF_T4T_MIN_VALID_MLE    0x000FU        /*!< Min valid MLe. MLe valid range 000Fh-FFFFh        */
#define NDEF_T4T_MIN_VALID_MLC    0x000DU        /*!< Min valid MLc. MLc valid range 000Dh-FFFFh        */

#define NDEF_T4T_NLEN_LEN              2U        /*!< NLEN LEN (mapping version v2.0): 2 bytes          */
#define NDEF_T4T_ENLEN_LEN             4U        /*!< ENLEN LEN (mapping version v3.0): 4 bytes         */

#define NDEF_T4T_MIN_NLEN              3U        /*!< Minimum non null NLEN value. TS T4T v1.0 B        */

#define NDEF_T4T_MV2_MAX_OFSSET   0x7FFFU        /*!< ReadBinary maximum Offset (offset range 0000-7FFFh)*/

#if NDEF_T4T_MAX_RAPDU_BODY_LEN >= 255U
  #define NDEF_T4T_MAX_MLE             255U        /*!< Maximum MLe value supported in this implementation (short field coding). Le=0 (MLe=256) not supported by some tag. */
#else
  #define NDEF_T4T_MAX_MLE NDEF_T4T_MAX_RAPDU_BODY_LEN
#endif

#if NDEF_T4T_MAX_CAPDU_BODY_LEN >= 255U
  #define NDEF_T4T_MAX_MLC             255U        /*!< Maximum MLc value supported in this implementation (short field coding).                                           */
#else
  #define NDEF_T4T_MAX_MLC NDEF_T4T_MAX_CAPDU_BODY_LEN
#endif

/*
 ******************************************************************************
 * GLOBAL TYPES
 ******************************************************************************
 */

/*
 ******************************************************************************
 * GLOBAL MACROS
 ******************************************************************************
 */

#define ndefT4TisT4TDevice(device) ((((device)->type == RFAL_NFC_LISTEN_TYPE_NFCA) && ((device)->dev.nfca.type == RFAL_NFCA_T4T)) || ((device)->type == RFAL_NFC_LISTEN_TYPE_NFCB))

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
static void ndefT4TInitializeIsoDepTxRxParam(ndefContext *ctx, rfalIsoDepApduTxRxParam *isoDepAPDU);
static ReturnCode ndefT4TTransceiveTxRx(ndefContext *ctx, rfalIsoDepApduTxRxParam *isoDepAPDU);
static ReturnCode ndefT4TReadAndParseCCFile(ndefContext *ctx);

/*
 ******************************************************************************
 * GLOBAL FUNCTIONS
 ******************************************************************************
 */

/*******************************************************************************/
static void ndefT4TInitializeIsoDepTxRxParam(ndefContext *ctx, rfalIsoDepApduTxRxParam *isoDepAPDU)
{
  /* Initialize the ISO-DEP protocol transceive context */
  isoDepAPDU->txBuf        = &ctx->subCtx.t4t.cApduBuf;
  isoDepAPDU->DID          =  ctx->subCtx.t4t.DID;
  isoDepAPDU->FWT          =  ctx->subCtx.t4t.FWT;
  isoDepAPDU->dFWT         =  ctx->subCtx.t4t.dFWT;
  isoDepAPDU->FSx          =  ctx->subCtx.t4t.FSx;
  isoDepAPDU->ourFSx       = RFAL_ISODEP_FSX_KEEP;
  isoDepAPDU->rxBuf        = &ctx->subCtx.t4t.rApduBuf;
  isoDepAPDU->tmpBuf       = &ctx->subCtx.t4t.tmpBuf;
}

/*******************************************************************************/
static ReturnCode ndefT4TTransceiveTxRx(ndefContext *ctx, rfalIsoDepApduTxRxParam *isoDepAPDU)
{
  ReturnCode               ret;

  /* Initialize respAPDU */
  ctx->subCtx.t4t.respAPDU.rApduBuf = &ctx->subCtx.t4t.rApduBuf;
  isoDepAPDU->rxLen                 = &ctx->subCtx.t4t.respAPDU.rcvdLen;

  RfalNfcClass *rfal_nfc = ((NdefClass *)(ctx->ndef_class_instance))->rfal_nfc;

  ret = rfal_nfc->rfalIsoDepStartApduTransceive(*isoDepAPDU);
  if (ret == ERR_NONE) {
    do {
      /* Blocking implementation, T4T may define rather long timeouts */
      rfal_nfc->rfalNfcWorker();
      ret = rfal_nfc->rfalIsoDepGetApduTransceiveStatus();
    } while (ret == ERR_BUSY);
  }

  if (ret != ERR_NONE) {
    return ret;
  }

  ret = rfal_nfc->rfalT4TPollerParseRAPDU(&ctx->subCtx.t4t.respAPDU);
  ctx->subCtx.t4t.rApduBodyLen = ctx->subCtx.t4t.respAPDU.rApduBodyLen;

  return ret;
}

/*******************************************************************************/
static ReturnCode ndefT4TReadNlen(ndefContext *ctx)
{
  ReturnCode           ret;
  uint8_t              nlenLen;
  uint8_t             *nLen;

  ctx->state = NDEF_STATE_INVALID;
  nlenLen = (ndefMajorVersion(ctx->cc.t4t.vNo) == ndefMajorVersion(NDEF_T4T_MAPPING_VERSION_3_0)) ? NDEF_T4T_ENLEN_LEN : NDEF_T4T_NLEN_LEN;

  /* Read NLEN/ENLEN TS T4T v1.0 7.2.1.11 */
  ret = ndefT4TPollerReadBinary(ctx, 0U, nlenLen);
  if (ret != ERR_NONE) {
    /* Conclude procedure TS T4T v1.0 7.2.1.11 */
    return ret;
  }
  nLen = ctx->subCtx.t4t.rApduBuf.apdu;
  ctx->messageLen    = (nlenLen == NDEF_T4T_ENLEN_LEN) ?  GETU32(&nLen[0]) : (uint32_t)ndefBytes2Uint16(nLen[0], nLen[1]);

  if ((ctx->messageLen > (ctx->cc.t4t.fileSize - nlenLen)) || ((ctx->messageLen > 0U) && (ctx->messageLen < NDEF_T4T_MIN_NLEN))) {
    /* Conclude procedure TS T4T v1.0 7.2.1.11 */
    return ERR_REQUEST;
  }
  if (ctx->messageLen == 0U) {
    if (!(ndefT4TIsWriteAccessGranted(ctx->cc.t4t.writeAccess))) {
      /* Conclude procedure TS T4T v1.0 7.2.1.11 */
      return ERR_REQUEST;
    }
    ctx->state = NDEF_STATE_INITIALIZED;
  } else {
    ctx->state = (ndefT4TIsWriteAccessGranted(ctx->cc.t4t.writeAccess)) ? NDEF_STATE_READWRITE : NDEF_STATE_READONLY;
  }

  return ERR_NONE;
}

/*******************************************************************************/
static ReturnCode ndefT4TReadAndParseCCFile(ndefContext *ctx)
{
  static const uint8_t RFAL_T4T_FID_CC[]      = {0xE1, 0x03};                                /*!< FID_CC-File               T4T 1.0  4.2   */

  ReturnCode           ret;
  uint8_t              dataIt;

  /* Select CCFILE TS T4T v1.0 7.2.1.3 */
  ret =  ndefT4TPollerSelectFile(ctx, RFAL_T4T_FID_CC);
  if (ret != ERR_NONE) {
    /* Conclude procedure TS T4T v1.0 7.2.1.4 */
    return ret;
  }

  /* Read CCFILE TS T4T v1.0 7.2.1.5 */
  /* read CCFILE assuming at least 15 bytes are available. If V3 found will read the extra bytes in a second step */
  ret = ndefT4TPollerReadBinary(ctx, 0U, NDEF_T4T_CCFILEV2_LEN);
  if (ret != ERR_NONE) {
    /* Conclude procedure TS T4T v1.0 7.2.1.6 */
    return ret;
  }
  (void)ST_MEMCPY(ctx->ccBuf, ctx->subCtx.t4t.rApduBuf.apdu, NDEF_T4T_CCFILEV2_LEN);
  dataIt = 0;
  ctx->cc.t4t.ccLen = GETU16(&ctx->ccBuf[dataIt]);
  dataIt += (uint8_t)sizeof(uint16_t);
  ctx->cc.t4t.vNo   = ctx->ccBuf[dataIt];
  dataIt++;
  ctx->cc.t4t.mLe   = GETU16(&ctx->ccBuf[dataIt]);
  dataIt += (uint8_t)sizeof(uint16_t);
  ctx->cc.t4t.mLc   = GETU16(&ctx->ccBuf[dataIt]);
  dataIt += (uint8_t)sizeof(uint16_t);

  /* TS T4T v1.0 7.2.1.7 verify MLe and MLc are within the valid range */
  if ((ctx->cc.t4t.mLe < NDEF_T4T_MIN_VALID_MLE) || (ctx->cc.t4t.mLc < NDEF_T4T_MIN_VALID_MLC)) {
    /* Conclude procedure TS T4T v1.0 7.2.1.8 */
    return ERR_REQUEST;
  }

  ctx->subCtx.t4t.curMLe   = (uint8_t)MIN(ctx->cc.t4t.mLe, NDEF_T4T_MAX_MLE); /* Only short field codind supported */
  ctx->subCtx.t4t.curMLc   = (uint8_t)MIN(ctx->cc.t4t.mLc, NDEF_T4T_MAX_MLC); /* Only short field codind supported */

  /* TS T4T v1.0 7.2.1.7 and 4.3.2.4 verify support of mapping version */
  if (ndefMajorVersion(ctx->cc.t4t.vNo) > ndefMajorVersion(NDEF_T4T_MAPPING_VERSION_3_0)) {
    /* Conclude procedure TS T4T v1.0 7.2.1.8 */
    return ERR_REQUEST;
  }
  if (ndefMajorVersion(ctx->cc.t4t.vNo) == ndefMajorVersion(NDEF_T4T_MAPPING_VERSION_3_0)) {
    /* V3 found: read remainng bytes */
    ret = ndefT4TPollerReadBinary(ctx, NDEF_T4T_CCFILEV2_LEN, NDEF_T4T_CCFILEV3_LEN - NDEF_T4T_CCFILEV2_LEN);
    if (ret != ERR_NONE) {
      /* Conclude procedure TS T4T v1.0 7.2.1.6 */
      return ret;
    }
    (void)ST_MEMCPY(&ctx->ccBuf[NDEF_T4T_CCFILEV2_LEN], ctx->subCtx.t4t.rApduBuf.apdu, NDEF_T4T_CCFILEV3_LEN - NDEF_T4T_CCFILEV2_LEN);

    /* TS T4T v1.0 7.2.1.7 verify coding as in table 5 */
    if (ctx->ccBuf[dataIt] != NDEF_T4T_ENDEF_CTLV_T) {
      /* Conclude procedure TS T4T v1.0 7.2.1.8 */
      return ERR_REQUEST;
    }
    dataIt++;
    if (ctx->ccBuf[dataIt] < NDEF_T4T_ENDEF_CTLV_L) {
      /* Conclude procedure TS T4T v1.0 7.2.1.8 */
      return ERR_REQUEST;
    }
    dataIt++;
    ctx->cc.t4t.fileId[0U]   = ctx->ccBuf[dataIt];
    dataIt++;
    ctx->cc.t4t.fileId[1U]   = ctx->ccBuf[dataIt];
    dataIt++;
    ctx->cc.t4t.fileSize    = GETU32(&ctx->ccBuf[dataIt]);
    dataIt += (uint8_t)sizeof(uint32_t);
    ctx->cc.t4t.readAccess  = ctx->ccBuf[dataIt];
    dataIt++;
    ctx->cc.t4t.writeAccess = ctx->ccBuf[dataIt];
    dataIt++;
  } else {
    if (ctx->ccBuf[dataIt] != NDEF_T4T_NDEF_CTLV_T) {
      return ERR_REQUEST;
    }
    dataIt++;
    if (ctx->ccBuf[dataIt] < NDEF_T4T_NDEF_CTLV_L) {
      return ERR_REQUEST;
    }
    dataIt++;
    ctx->cc.t4t.fileId[0U]   = ctx->ccBuf[dataIt];
    dataIt++;
    ctx->cc.t4t.fileId[1U]   = ctx->ccBuf[dataIt];
    dataIt++;
    ctx->cc.t4t.fileSize    = ndefBytes2Uint16(ctx->ccBuf[dataIt], ctx->ccBuf[dataIt + 1U]);
    dataIt += (uint8_t)sizeof(uint16_t);
    ctx->cc.t4t.readAccess  = ctx->ccBuf[dataIt];
    dataIt++;
    ctx->cc.t4t.writeAccess = ctx->ccBuf[dataIt];
    dataIt++;
  }
  return ERR_NONE;
}

/*******************************************************************************/
ReturnCode ndefT4TPollerSelectNdefTagApplication(ndefContext *ctx)
{
  ReturnCode               ret;
  rfalIsoDepApduTxRxParam  isoDepAPDU;
  static const uint8_t NDEF_T4T_AID_NDEF[]    = {0xD2, 0x76, 0x00, 0x00, 0x85, 0x01, 0x01};  /*!< AID_NDEF v2.0 or higher   T4T 1.0  4.3.3 */
  static const uint8_t NDEF_T4T_AID_NDEF_V1[] = {0xD2, 0x76, 0x00, 0x00, 0x85, 0x01, 0x00};  /*!< AID_NDEF v1.0             T4T 1.0  4.3.3 */

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T4T)) {
    return ERR_PARAM;
  }

  RfalNfcClass *rfal_nfc = ((NdefClass *)(ctx->ndef_class_instance))->rfal_nfc;

  ndefT4TInitializeIsoDepTxRxParam(ctx, &isoDepAPDU);
  (void)rfal_nfc->rfalT4TPollerComposeSelectAppl(isoDepAPDU.txBuf, NDEF_T4T_AID_NDEF, (uint8_t)sizeof(NDEF_T4T_AID_NDEF), &isoDepAPDU.txBufLen);
  ret = ndefT4TTransceiveTxRx(ctx, &isoDepAPDU);

  if (ret == ERR_NONE) {
    /* application v2 or higher found */
    ctx->subCtx.t4t.mv1Flag = false;
    return ret;
  }

  if (ret != ERR_REQUEST) {
    return ret;
  }

  /* if v2 application not found, try v1 */
  (void)rfal_nfc->rfalT4TPollerComposeSelectAppl(isoDepAPDU.txBuf, NDEF_T4T_AID_NDEF_V1, (uint8_t)sizeof(NDEF_T4T_AID_NDEF_V1), &isoDepAPDU.txBufLen);
  ret = ndefT4TTransceiveTxRx(ctx, &isoDepAPDU);

  if (ret == ERR_NONE) {
    /* application v1 found */
    ctx->subCtx.t4t.mv1Flag = true;
  }
  return ret;
}

/*******************************************************************************/
ReturnCode ndefT4TPollerSelectFile(ndefContext *ctx, const uint8_t *fileId)
{
  ReturnCode               ret;
  rfalIsoDepApduTxRxParam  isoDepAPDU;

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T4T)) {
    return ERR_PARAM;
  }

  RfalNfcClass *rfal_nfc = ((NdefClass *)(ctx->ndef_class_instance))->rfal_nfc;

  ndefT4TInitializeIsoDepTxRxParam(ctx, &isoDepAPDU);

  if (ctx->subCtx.t4t.mv1Flag) {
    (void)rfal_nfc->rfalT4TPollerComposeSelectFileV1Mapping(isoDepAPDU.txBuf, fileId, (uint8_t)sizeof(fileId), &isoDepAPDU.txBufLen);
  } else {
    (void)rfal_nfc->rfalT4TPollerComposeSelectFile(isoDepAPDU.txBuf, fileId, NDEF_T4T_FID_SIZE, &isoDepAPDU.txBufLen);
  }

  ret = ndefT4TTransceiveTxRx(ctx, &isoDepAPDU);

  return ret;
}


/*******************************************************************************/
ReturnCode ndefT4TPollerReadBinary(ndefContext *ctx, uint16_t offset, uint8_t len)
{
  ReturnCode               ret;
  rfalIsoDepApduTxRxParam  isoDepAPDU;

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T4T) || (len >  ctx->subCtx.t4t.curMLe) || (offset > NDEF_T4T_OFFSET_MAX)) {
    return ERR_PARAM;
  }

  RfalNfcClass *rfal_nfc = ((NdefClass *)(ctx->ndef_class_instance))->rfal_nfc;

  ndefT4TInitializeIsoDepTxRxParam(ctx, &isoDepAPDU);
  (void)rfal_nfc->rfalT4TPollerComposeReadData(isoDepAPDU.txBuf, offset, len, &isoDepAPDU.txBufLen);
  ret = ndefT4TTransceiveTxRx(ctx, &isoDepAPDU);

  return ret;
}

/*******************************************************************************/
ReturnCode ndefT4TPollerReadBinaryODO(ndefContext *ctx, uint32_t offset, uint8_t len)
{
  ReturnCode               ret;
  rfalIsoDepApduTxRxParam  isoDepAPDU;

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T4T) || (len >  ctx->subCtx.t4t.curMLe)  || (offset > NDEF_T4T_ODO_OFFSET_MAX)) {
    return ERR_PARAM;
  }

  RfalNfcClass *rfal_nfc = ((NdefClass *)(ctx->ndef_class_instance))->rfal_nfc;

  ndefT4TInitializeIsoDepTxRxParam(ctx, &isoDepAPDU);
  (void)rfal_nfc->rfalT4TPollerComposeReadDataODO(isoDepAPDU.txBuf, offset, len, &isoDepAPDU.txBufLen);
  ret = ndefT4TTransceiveTxRx(ctx, &isoDepAPDU);

  return ret;
}

/*******************************************************************************/
ReturnCode ndefT4TPollerReadBytes(ndefContext *ctx, uint32_t offset, uint32_t len, uint8_t *buf, uint32_t *rcvdLen)
{
  ReturnCode           ret;
  uint8_t              le;
  uint32_t             lvOffset = offset;
  uint32_t             lvLen    = len;
  uint8_t             *lvBuf    = buf;

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T4T) || (lvLen == 0U)) {
    return ERR_PARAM;
  }
  if (rcvdLen != NULL) {
    *rcvdLen = 0U;
  }

  do {
    le = (lvLen > ctx->subCtx.t4t.curMLe) ? ctx->subCtx.t4t.curMLe : (uint8_t)lvLen;
    if (lvOffset > NDEF_T4T_MV2_MAX_OFSSET) {
      ret = ndefT4TPollerReadBinaryODO(ctx, lvOffset, le);
    } else {
      ret = ndefT4TPollerReadBinary(ctx, (uint16_t)lvOffset, le);
    }
    if (ret != ERR_NONE) {
      return ret;
    }
    if (ctx->subCtx.t4t.rApduBodyLen == 0U) {
      break; /* no more to read */
    }
    if (ctx->subCtx.t4t.rApduBodyLen >  lvLen) {
      return ERR_SYSTEM;
    }
    (void)ST_MEMCPY(lvBuf, ctx->subCtx.t4t.rApduBuf.apdu, ctx->subCtx.t4t.rApduBodyLen);
    lvBuf     = &lvBuf[ctx->subCtx.t4t.rApduBodyLen];
    lvOffset += ctx->subCtx.t4t.rApduBodyLen;
    lvLen    -= ctx->subCtx.t4t.rApduBodyLen;
    if (rcvdLen != NULL) {
      *rcvdLen += ctx->subCtx.t4t.rApduBodyLen;
    }
  } while (lvLen != 0U);

  return ERR_NONE;
}

/*******************************************************************************/
ReturnCode ndefT4TPollerContextInitialization(ndefContext *ctx, const ndefDevice *dev)
{
  if ((ctx == NULL) || (dev == NULL) || !ndefT4TisT4TDevice(dev)) {
    return ERR_PARAM;
  }

  (void)ST_MEMCPY(&ctx->device, dev, sizeof(ctx->device));

  ctx->type              = NDEF_DEV_T4T;
  ctx->state             = NDEF_STATE_INVALID;
  ctx->subCtx.t4t.curMLc = NDEF_T4T_DEFAULT_MLC;
  ctx->subCtx.t4t.curMLe = NDEF_T4T_DEFAULT_MLE;

  ctx->subCtx.t4t.DID    = dev->proto.isoDep.info.DID;
  ctx->subCtx.t4t.FWT    = dev->proto.isoDep.info.FWT;
  ctx->subCtx.t4t.dFWT   = dev->proto.isoDep.info.dFWT;
  ctx->subCtx.t4t.FSx    = dev->proto.isoDep.info.FSx;

  return ERR_NONE;
}

/*******************************************************************************/
ReturnCode ndefT4TPollerNdefDetect(ndefContext *ctx, ndefInfo *info)
{
  ReturnCode           ret;
  uint8_t              nlenLen;

  if (info != NULL) {
    info->state                = NDEF_STATE_INVALID;
    info->majorVersion         = 0U;
    info->minorVersion         = 0U;
    info->areaLen              = 0U;
    info->areaAvalableSpaceLen = 0U;
    info->messageLen           = 0U;
  }

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T4T)) {
    return ERR_PARAM;
  }

  ctx->state = NDEF_STATE_INVALID;

  /* Select NDEF Tag application TS T4T v1.0 7.2.1.1 */
  ret =  ndefT4TPollerSelectNdefTagApplication(ctx);
  if (ret != ERR_NONE) {
    /* Conclude procedure TS T4T v1.0 7.2.1.2 */
    return ret;
  }

  /* TS T4T v1.0 7.2.1.3 and following */
  ret = ndefT4TReadAndParseCCFile(ctx);
  if (ret != ERR_NONE) {
    return ret;
  }
  nlenLen = (ndefMajorVersion(ctx->cc.t4t.vNo) == ndefMajorVersion(NDEF_T4T_MAPPING_VERSION_3_0)) ? NDEF_T4T_ENLEN_LEN : NDEF_T4T_NLEN_LEN;

  /* TS T4T v1.0 7.2.1.7 verify file READ access */
  if (!(ndefT4TIsReadAccessGranted(ctx->cc.t4t.readAccess))) {
    /* Conclude procedure TS T4T v1.0 7.2.1.8 */
    return ERR_REQUEST;
  }
  /* File size need at least be enough to store NLEN or ENLEN */
  if (ctx->cc.t4t.fileSize < nlenLen) {
    return ERR_REQUEST;
  }

  /* Select NDEF File TS T4T v1.0 7.2.1.9 */
  ret =  ndefT4TPollerSelectFile(ctx, ctx->cc.t4t.fileId);
  if (ret != ERR_NONE) {
    /* Conclude procedure TS T4T v1.0 7.2.1.10 */
    return ret;
  }
  /* Read NLEN/ENLEN TS T4T v1.0 7.2.1.11 */
  ret = ndefT4TReadNlen(ctx);
  if (ret != ERR_NONE) {
    /* Conclude procedure TS T4T v1.0 7.2.1.11 */
    return ret;
  }
  ctx->messageOffset = nlenLen;
  ctx->areaLen       = ctx->cc.t4t.fileSize;

  if (info != NULL) {
    info->state                = ctx->state;
    info->majorVersion         = ndefMajorVersion(ctx->cc.t4t.vNo);
    info->minorVersion         = ndefMinorVersion(ctx->cc.t4t.vNo);
    info->areaLen              = ctx->areaLen;
    info->areaAvalableSpaceLen = ctx->areaLen - ctx->messageOffset;
    info->messageLen           = ctx->messageLen;
  }

  return ERR_NONE;
}

/*******************************************************************************/
ReturnCode ndefT4TPollerReadRawMessage(ndefContext *ctx, uint8_t *buf, uint32_t bufLen, uint32_t *rcvdLen, bool single)
{
  ReturnCode           ret;

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T4T) || (buf == NULL)) {
    return ERR_PARAM;
  }

  /* TS T4T v1.0 7.2.2.1: T4T NDEF Detect should have been called at least once before NDEF read procedure */

  if (!single) {
    ret = ndefT4TReadNlen(ctx);
    if (ret != ERR_NONE) {
      /* Conclude procedure */
      return ret;
    }
  }

  /* TS T4T v1.0 7.3.3.2: check presence of NDEF message */
  if (ctx->state <= NDEF_STATE_INITIALIZED) {
    /* Conclude procedure TS T4T v1.0 7.2.2.2 */
    return ERR_WRONG_STATE;
  }

  if (ctx->messageLen > bufLen) {
    return ERR_NOMEM;
  }

  /* TS T4T v1.0 7.3.3.3: read the NDEF message */
  ret = ndefT4TPollerReadBytes(ctx, ctx->messageOffset, ctx->messageLen, buf, rcvdLen);
  if (ret != ERR_NONE) {
    ctx->state = NDEF_STATE_INVALID;
  }

  return ret;
}

#if NDEF_FEATURE_FULL_API

/*******************************************************************************/
ReturnCode ndefT4TPollerWriteBinary(ndefContext *ctx, uint16_t offset, const uint8_t *data, uint8_t len)
{
  ReturnCode               ret;
  rfalIsoDepApduTxRxParam  isoDepAPDU;

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T4T) || (len >  ctx->subCtx.t4t.curMLc) || (offset > NDEF_T4T_OFFSET_MAX)) {
    return ERR_PARAM;
  }

  RfalNfcClass *rfal_nfc = ((NdefClass *)(ctx->ndef_class_instance))->rfal_nfc;

  ndefT4TInitializeIsoDepTxRxParam(ctx, &isoDepAPDU);
  (void)rfal_nfc->rfalT4TPollerComposeWriteData(isoDepAPDU.txBuf, offset, data, len, &isoDepAPDU.txBufLen);
  ret = ndefT4TTransceiveTxRx(ctx, &isoDepAPDU);

  return ret;
}

/*******************************************************************************/
ReturnCode ndefT4TPollerWriteBinaryODO(ndefContext *ctx, uint32_t offset, const uint8_t *data, uint8_t len)
{
  ReturnCode               ret;
  rfalIsoDepApduTxRxParam  isoDepAPDU;

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T4T) || (len >  ctx->subCtx.t4t.curMLc) || (offset > NDEF_T4T_ODO_OFFSET_MAX)) {
    return ERR_PARAM;
  }

  RfalNfcClass *rfal_nfc = ((NdefClass *)(ctx->ndef_class_instance))->rfal_nfc;

  ndefT4TInitializeIsoDepTxRxParam(ctx, &isoDepAPDU);
  (void)rfal_nfc->rfalT4TPollerComposeWriteDataODO(isoDepAPDU.txBuf, offset, data, len, &isoDepAPDU.txBufLen);
  ret = ndefT4TTransceiveTxRx(ctx, &isoDepAPDU);

  return ret;
}

/*******************************************************************************/
ReturnCode ndefT4TPollerWriteBytes(ndefContext *ctx, uint32_t offset, const uint8_t *buf, uint32_t len, bool pad, bool writeTerminator)
{
  ReturnCode           ret;
  uint8_t              lc;
  uint32_t             lvOffset = offset;
  uint32_t             lvLen    = len;
  const uint8_t       *lvBuf    = buf;

  NO_WARNING(pad);             /* Unused parameter */
  NO_WARNING(writeTerminator); /* Unused parameter */

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T4T) || (lvLen == 0U)) {
    return ERR_PARAM;
  }

  do {

    if (lvOffset > NDEF_T4T_MV2_MAX_OFSSET) {
      lc = (lvLen > ((uint32_t)ctx->subCtx.t4t.curMLc - NDEF_T4T_WRITE_ODO_PREFIX_SIZE)) ? (uint8_t)(ctx->subCtx.t4t.curMLc - NDEF_T4T_WRITE_ODO_PREFIX_SIZE) : (uint8_t)lvLen;
      ret = ndefT4TPollerWriteBinaryODO(ctx, lvOffset, lvBuf, lc);
    } else {
      lc = (lvLen > ctx->subCtx.t4t.curMLc) ? ctx->subCtx.t4t.curMLc : (uint8_t)lvLen;
      ret = ndefT4TPollerWriteBinary(ctx, (uint16_t)lvOffset, lvBuf, lc);
    }
    if (ret != ERR_NONE) {
      return ret;
    }
    lvBuf     = &lvBuf[lc];
    lvOffset += lc;
    lvLen    -= lc;
  } while (lvLen != 0U);

  return ERR_NONE;
}

/*******************************************************************************/
ReturnCode ndefT4TPollerWriteRawMessageLen(ndefContext *ctx, uint32_t rawMessageLen, bool writeTerminator)
{
  ReturnCode           ret;
  uint8_t              buf[NDEF_T4T_ENLEN_LEN];
  uint8_t              dataIt;

  NO_WARNING(writeTerminator); /* Unused parameter */

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T4T)) {
    return ERR_PARAM;
  }

  if ((ctx->state != NDEF_STATE_INITIALIZED) && (ctx->state != NDEF_STATE_READWRITE)) {
    return ERR_WRONG_STATE;
  }

  dataIt = 0U;
  if (ndefMajorVersion(ctx->cc.t4t.vNo) == ndefMajorVersion(NDEF_T4T_MAPPING_VERSION_3_0)) {
    buf[dataIt] = (uint8_t)(rawMessageLen >> 24U);
    dataIt++;
    buf[dataIt] = (uint8_t)(rawMessageLen >> 16U);
    dataIt++;
    buf[dataIt] = (uint8_t)(rawMessageLen >>  8U);
    dataIt++;
    buf[dataIt] = (uint8_t)(rawMessageLen);
    dataIt++;
  } else {
    buf[dataIt] = (uint8_t)(rawMessageLen >>  8U);
    dataIt++;
    buf[dataIt] = (uint8_t)(rawMessageLen);
    dataIt++;
  }

  ret = ndefT4TPollerWriteBytes(ctx, 0U, buf, dataIt, false, false);
  return ret;
}

/*******************************************************************************/
ReturnCode ndefT4TPollerWriteRawMessage(ndefContext *ctx, const uint8_t *buf, uint32_t bufLen)
{
  ReturnCode           ret;

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T4T) || ((buf == NULL) && (bufLen != 0U))) {
    return ERR_PARAM;
  }

  /* TS T4T v1.0 7.2.3.1: T4T NDEF Detect should have been called before NDEF write procedure */
  /* Warning: current selected file must not be changed between NDEF Detect procedure and NDEF Write procedure*/

  /* TS T4T v1.0 7.3.3.2: check write access condition */
  if ((ctx->state != NDEF_STATE_INITIALIZED) && (ctx->state != NDEF_STATE_READWRITE)) {
    /* Conclude procedure TS T4T v1.0 7.2.3.2 */
    return ERR_WRONG_STATE;
  }

  /* TS T4T v1.0 7.2.3.3: check Mapping Version    */
  /* Done automatically inside underlying functions */

  /* TS T4T v1.0 7.2.3.4/8 verify length of the NDEF message */
  ret = ndefT4TPollerCheckAvailableSpace(ctx, bufLen);
  if (ret != ERR_NONE) {
    /* Conclude procedure TS T4T v1.0 7.2.3.4/8 */
    return ERR_PARAM;
  }

  /* TS T4T v1.0 7.2.3.5/9 Write value 0000h in NLEN field (resp. 00000000h in ENLEN field) */
  ret = ndefT4TPollerBeginWriteMessage(ctx, bufLen);
  if (ret != ERR_NONE) {
    ctx->state = NDEF_STATE_INVALID;
    /* Conclude procedure TS T4T v1.0 7.2.3.5/9 */
    return ret;
  }

  if (bufLen != 0U) {
    /* TS T4T v1.0 7.2.3.6/10 Write NDEF message) */
    ret = ndefT4TPollerWriteBytes(ctx, ctx->messageOffset, buf, bufLen, false, false);
    if (ret != ERR_NONE) {
      /* Conclude procedure TS T4T v1.0 7.2.3.6/10 */
      ctx->state = NDEF_STATE_INVALID;
      return ret;
    }

    /* TS T4T v1.0 7.2.3.7/11 Write value length in NLEN field (resp. in ENLEN field) */
    ret = ndefT4TPollerEndWriteMessage(ctx, bufLen, false);
    if (ret != ERR_NONE) {
      /* Conclude procedure TS T4T v1.0 7.2.3.7/11 */
      ctx->state = NDEF_STATE_INVALID;
      return ret;
    }
  }

  return ret;
}

/*******************************************************************************/
ReturnCode ndefT4TPollerTagFormat(ndefContext *ctx, const ndefCapabilityContainer *cc, uint32_t options)
{
  ReturnCode           ret;

  uint8_t              buf[NDEF_T4T_ENLEN_LEN];

  NO_WARNING(cc);
  NO_WARNING(options);

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T4T)) {
    return ERR_PARAM;
  }

  ret =  ndefT4TPollerSelectNdefTagApplication(ctx);
  if (ret != ERR_NONE) {
    return ret;
  }

  ret =  ndefT4TReadAndParseCCFile(ctx);
  if (ret != ERR_NONE) {
    return ret;
  }

  ret =  ndefT4TPollerSelectFile(ctx, ctx->cc.t4t.fileId);
  if (ret != ERR_NONE) {
    return ret;
  }
  (void)ST_MEMSET(buf, 0x00, sizeof(buf));
  ret = ndefT4TPollerWriteBytes(ctx, 0U, buf, (ndefMajorVersion(ctx->cc.t4t.vNo) == ndefMajorVersion(NDEF_T4T_MAPPING_VERSION_3_0)) ? NDEF_T4T_ENLEN_LEN : NDEF_T4T_NLEN_LEN, false, false);
  return ret;
}

/*******************************************************************************/
ReturnCode ndefT4TPollerCheckPresence(ndefContext *ctx)
{
  rfalIsoDepApduTxRxParam  isoDepAPDU;
  ReturnCode               ret;

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T4T)) {
    return ERR_PARAM;
  }

  RfalNfcClass *rfal_nfc = ((NdefClass *)(ctx->ndef_class_instance))->rfal_nfc;

  ndefT4TInitializeIsoDepTxRxParam(ctx, &isoDepAPDU);
  (void)rfal_nfc->rfalT4TPollerComposeReadData(isoDepAPDU.txBuf, 0, 1, &isoDepAPDU.txBufLen);

  /* Initialize respAPDU */
  ctx->subCtx.t4t.respAPDU.rApduBuf = &ctx->subCtx.t4t.rApduBuf;
  isoDepAPDU.rxLen                  = &ctx->subCtx.t4t.respAPDU.rcvdLen;

  (void)rfal_nfc->rfalIsoDepStartApduTransceive(isoDepAPDU);
  do {
    /* Blocking implementation, T4T may define rather long timeouts */
    rfal_nfc->rfalNfcWorker();
    ret = rfal_nfc->rfalIsoDepGetApduTransceiveStatus();
  } while (ret == ERR_BUSY);

  return ret;
}

/*******************************************************************************/
ReturnCode ndefT4TPollerCheckAvailableSpace(const ndefContext *ctx, uint32_t messageLen)
{
  uint8_t              nlenLen;

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T4T)) {
    return ERR_PARAM;
  }

  if (ctx->state == NDEF_STATE_INVALID) {
    return ERR_WRONG_STATE;
  }

  nlenLen = (ndefMajorVersion(ctx->cc.t4t.vNo) == ndefMajorVersion(NDEF_T4T_MAPPING_VERSION_3_0)) ? NDEF_T4T_ENLEN_LEN : NDEF_T4T_NLEN_LEN;
  if ((messageLen + (uint32_t)nlenLen) > ctx->cc.t4t.fileSize) {
    return ERR_NOMEM;
  }
  return ERR_NONE;
}

/*******************************************************************************/
ReturnCode ndefT4TPollerBeginWriteMessage(ndefContext *ctx, uint32_t messageLen)
{
  ReturnCode           ret;
  NO_WARNING(messageLen);

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T4T)) {
    return ERR_PARAM;
  }

  if ((ctx->state != NDEF_STATE_INITIALIZED) && (ctx->state != NDEF_STATE_READWRITE)) {
    return ERR_WRONG_STATE;
  }

  /* TS T4T v1.0 7.2.3.5/9 Write value 0000h in NLEN field (resp. 00000000h in ENLEN field) */
  ret = ndefT4TPollerWriteRawMessageLen(ctx, 0U, false);
  if (ret != ERR_NONE) {
    /* Conclude procedure */
    ctx->state = NDEF_STATE_INVALID;
    return ret;
  }

  ctx->state = NDEF_STATE_INITIALIZED;

  return ERR_NONE;
}

/*******************************************************************************/
ReturnCode ndefT4TPollerEndWriteMessage(ndefContext *ctx, uint32_t messageLen, bool writeTerminator)
{
  ReturnCode           ret;

  NO_WARNING(writeTerminator); /* Unused parameter */

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T4T)) {
    return ERR_PARAM;
  }

  if (ctx->state != NDEF_STATE_INITIALIZED) {
    return ERR_WRONG_STATE;
  }

  /* TS T4T v1.0 7.2.3.7/11 Write value length in NLEN field (resp. in ENLEN field) */
  ret = ndefT4TPollerWriteRawMessageLen(ctx, messageLen, false);
  if (ret != ERR_NONE) {
    /* Conclude procedure */
    ctx->state = NDEF_STATE_INVALID;
    return ret;
  }
  ctx->messageLen = messageLen;
  ctx->state = (ctx->messageLen == 0U) ? NDEF_STATE_INITIALIZED : NDEF_STATE_READWRITE;
  return ERR_NONE;
}

/*******************************************************************************/
ReturnCode ndefT4TPollerSetReadOnly(ndefContext *ctx)
{
  NO_WARNING(ctx);
  return ERR_NOTSUPP;
}

#endif /* NDEF_FEATURE_FULL_API */

#endif /* NDEF_FEATURE_T4T */
