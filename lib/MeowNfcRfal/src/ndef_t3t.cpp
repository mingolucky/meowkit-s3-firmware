
/**
  ******************************************************************************
  * @file           : ndef_t3t.cpp
  * @brief          : Provides NDEF methods and definitions to access NFC Forum T3T
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
#include "ndef_t3t.h"
#include "nfc_utils.h"
#include "ndef_class.h"

/*
 ******************************************************************************
 * ENABLE SWITCH
 ******************************************************************************
 */

#ifndef NDEF_FEATURE_T3T
  #error " NDEF: Module configuration missing. Please enable/disable T3T module by setting: NDEF_FEATURE_T3T"
#endif

#if NDEF_FEATURE_T3T

#ifndef NDEF_FEATURE_FULL_API
  #error " NDEF: Module configuration missing. Please enable/disable Full API by setting: NDEF_FEATURE_FULL_API"
#endif

/*
 ******************************************************************************
 * GLOBAL DEFINES
 ******************************************************************************
 */
#define NDEF_T3T_MAX_DEVICE                  1U  /*!< T3T maximum number of device for detection         */
#define NDEF_T3T_SYSTEMCODE              0x12FCU /*!< SENSF_RES System Code for T3T TS T3T 1.0 7.1.1.1   */
#define NDEF_T3T_WRITEFLAG_ON               0xFU /*!< WriteFlag ON  value TS T3T 1.0 7.2.2.16            */
#define NDEF_T3T_WRITEFLAG_OFF              0x0U /*!< WriteFlag OFF value TS T3T 1.0 7.2.2.16            */
#define NDEF_T3T_AREA_OFFSET                 16U /*!< T3T Area starts at block #1                        */
#define NDEF_T3T_BLOCKLEN                    16U /*!< T3T block length is always 16                      */
#define NDEF_T3T_FLAG_RW                      1U /*!< T3T read/write flag value                          */
#define NDEF_T3T_FLAG_RO                      0U /*!< T3T read only flag value                           */
#define NDEF_T3T_SENSFRES_NFCID2              2U /*!< T3T offset of UID in SENSFRES struct               */
#define NDEF_T3T_ATTRIB_INFO_CHECKSUM_LEN   0xEU /*!< T3T checksum length for attribute info to compute  */
#define NDEF_T3T_ATTRIB_INFO_OFFSET_VERSION 0x0U /*!< T3T attribute info offset of version               */
#define NDEF_T3T_ATTRIB_INFO_OFFSET_NBR       1U /*!< T3T attribute info offset of number of read        */
#define NDEF_T3T_ATTRIB_INFO_OFFSET_NBW       2U /*!< T3T attribute info offset of number of write       */
#define NDEF_T3T_ATTRIB_INFO_OFFSET_MAXB      3U /*!< T3T attribute info offset of MAXB                  */
#define NDEF_T3T_ATTRIB_INFO_OFFSET_FLAG_W    9U /*!< T3T attribute info offset of Write flag            */
#define NDEF_T3T_ATTRIB_INFO_OFFSET_FLAG_RW  10U /*!< T3T attribute info offset of Read/Write flag       */
#define NDEF_T3T_ATTRIB_INFO_OFFSET_FLAG_LN  11U /*!< T3T attribute info offset of LN field              */
#define NDEF_T3T_ATTRIB_INFO_VERSION_1_0   0x10U /*!< T3T attribute info full version number             */
#define NDEF_T3T_ATTRIB_INFO_BLOCK_NB         0U /*!< T3T attribute info block number                    */
#define NDEF_T3T_BLOCKNB_CONF              0x80U /*!< T3T TxRx config value for Read/Write block         */
#define NDEF_T3T_CHECK_NB_BLOCKS_LEN          1U /*!< T3T Length of the Nb of blocks in the CHECK reply  */


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

#define ndefT3TisT3TDevice(device) ((device)->type == RFAL_NFC_LISTEN_TYPE_NFCF)
#define ndefT3TIsWriteFlagON(writeFlag) ((writeFlag) == NDEF_T3T_WRITEFLAG_ON)

#define ndefT3TLogD(...)                                                                                  /*!< Macro for the debug log method                  */

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
static ReturnCode ndefT3TPollerReadBlocks(ndefContext *ctx, uint16_t blockNum, uint8_t nbBlocks, uint8_t *rxBuf, uint16_t rxBufLen, uint16_t *rcvLen);
static ReturnCode ndefT3TPollerReadAttributeInformationBlock(ndefContext *ctx);

#if NDEF_FEATURE_FULL_API
  static ReturnCode ndefT3TPollerWriteBlocks(ndefContext *ctx, uint16_t blockNum, uint8_t nbBlocks, const uint8_t *dataBlocks);
  static ReturnCode ndefT3TPollerWriteAttributeInformationBlock(ndefContext *ctx);
#endif /* NDEF_FEATURE_FULL_API */

/*
 ******************************************************************************
 * GLOBAL FUNCTIONS
 ******************************************************************************
 */

/*******************************************************************************/
static ReturnCode ndefT3TPollerReadBlocks(ndefContext *ctx, uint16_t blockNum, uint8_t nbBlocks, uint8_t *rxBuf, uint16_t rxBufLen, uint16_t *rcvLen)
{
  ReturnCode                 ret;
  uint16_t                   requestedDataSize;
  rfalNfcfServBlockListParam servBlock;
  rfalNfcfBlockListElem     *listBlocks;
  uint8_t                    index;
  uint16_t                   rcvdLen = 0U;
  rfalNfcfServ               serviceCodeLst = 0x000BU; /* serviceCodeLst */

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T3T)) {
    return ERR_PARAM;
  }

  RfalNfcClass *rfal_nfc = ((NdefClass *)(ctx->ndef_class_instance))->rfal_nfc;

  requestedDataSize = (uint16_t)nbBlocks * NDEF_T3T_BLOCK_SIZE;
  if (rxBufLen < requestedDataSize) {
    return ERR_PARAM;
  }

  listBlocks = ctx->subCtx.t3t.listBlocks;

  for (index = 0U; index < nbBlocks; index++) {
    /* Write each block number (16 bits per block address) */
    listBlocks[index].blockNum = (uint16_t)(blockNum + (uint16_t) index);
    listBlocks[index].conf     = ((listBlocks[index].blockNum > 0xFFU) ? 0U : RFAL_NFCF_BLOCKLISTELEM_LEN_BIT);
  }

  servBlock.numServ   = 1U;
  servBlock.servList  = &serviceCodeLst;
  servBlock.numBlock  = nbBlocks;
  servBlock.blockList = listBlocks;

  ret = rfal_nfc->rfalNfcfPollerCheck(ctx->subCtx.t3t.NFCID2, &servBlock, ctx->subCtx.t3t.rxbuf, (uint16_t)sizeof(ctx->subCtx.t3t.rxbuf), &rcvdLen);
  if (ret != ERR_NONE) {
    return ret;
  }
  if (rcvdLen != (uint16_t)(NDEF_T3T_CHECK_NB_BLOCKS_LEN + requestedDataSize)) {
    return ERR_REQUEST;
  }
  if (requestedDataSize > 0U) {
    (void)ST_MEMCPY(rxBuf, &ctx->subCtx.t3t.rxbuf[NDEF_T3T_CHECK_NB_BLOCKS_LEN], requestedDataSize);
    if (rcvLen != NULL) {
      *rcvLen = requestedDataSize;
    }
  }
  return ERR_NONE;
}

/*******************************************************************************/
ReturnCode ndefT3TPollerReadBytes(ndefContext *ctx, uint32_t offset, uint32_t len, uint8_t *buf, uint32_t *rcvdLen)
{
  uint16_t        res;
  uint16_t        nbRead;
  ReturnCode      result     = ERR_NONE;
  uint32_t        currentLen = len;
  uint32_t        lvRcvLen   = 0U;
  const uint16_t  blockLen   = (uint16_t) NDEF_T3T_BLOCKLEN;
  uint16_t        startBlock = (uint16_t)(offset / blockLen);
  uint16_t        startAddr  = (uint16_t)(startBlock * blockLen);
  uint16_t        startOffset = (uint16_t)(offset - (uint32_t) startAddr);
  uint16_t        nbBlocks   = (uint16_t) NDEF_T3T_MAX_NB_BLOCKS;

  ndefT3TLogD("ndefT3TPollerReadBytes offset: 0x%8.8x, Len %d\r\n", offset, len);
  ndefT3TLogD("ndefT3TPollerReadBytes currentLen: %d, startBlock %d\r\n", currentLen, startBlock);

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T3T) || (len == 0U)) {
    return ERR_PARAM;
  }
  if (ctx->state != NDEF_STATE_INVALID) {
    nbBlocks = MIN(ctx->cc.t3t.nbR, NDEF_T3T_MAX_NB_BLOCKS);
  }

  if (startOffset != 0U) {
    /* Unaligned read, need to use a tmp buffer */
    res = ndefT3TPollerReadBlocks(ctx, startBlock, 1U /* One block */, ctx->subCtx.t3t.rxbuf, blockLen, &nbRead);
    if (res != ERR_NONE) {
      /* Check result */
      result = res;
    } else if (nbRead != NDEF_T3T_BLOCKLEN) {
      /* Check length */
      result = ERR_MEM_CORRUPT;
    } else {
      nbRead = (uint16_t)(nbRead - (uint16_t)startOffset);
      if ((uint32_t) nbRead > currentLen) {
        nbRead = (uint16_t) currentLen;
      }
      if (nbRead > 0U) {
        (void)ST_MEMCPY(buf, &ctx->subCtx.t3t.rxbuf[offset], (uint32_t)nbRead);
      }
      lvRcvLen   += (uint32_t) nbRead;
      currentLen -= (uint32_t) nbRead;
      startBlock++;
      ndefT3TLogD("ndefT3TPollerReadBytes currentLen: %d, startBlock %d\r\n", currentLen, startBlock);
    }
  }

  while ((currentLen >= (uint32_t)blockLen) && (result == ERR_NONE)) {
    if (currentLen < ((uint32_t)blockLen * nbBlocks)) {
      /* Reduce the nb of blocks to read */
      nbBlocks = (uint16_t)(currentLen / blockLen);
    }
    res = ndefT3TPollerReadBlocks(ctx, startBlock, (uint8_t)nbBlocks, ctx->subCtx.t3t.rxbuf, blockLen * nbBlocks, &nbRead);
    if (res != ERR_NONE) {
      /* Check result */
      return res;
    } else if (nbRead != (blockLen * nbBlocks)) {
      /* Check length */
      return ERR_MEM_CORRUPT;
    } else {
      (void)ST_MEMCPY(&buf[lvRcvLen], ctx->subCtx.t3t.rxbuf, (uint32_t)currentLen);
      lvRcvLen   += nbRead;
      currentLen -= nbRead;
      startBlock += nbBlocks;
      ndefT3TLogD("ndefT3TPollerReadBytes currentLen: %d, startBlock %d\r\n", currentLen, startBlock);
    }
  }
  if ((currentLen > 0U) && (result == ERR_NONE)) {
    /* Unaligned read, need to use a tmp buffer */
    res = ndefT3TPollerReadBlocks(ctx, startBlock, 1U /* One block */, ctx->subCtx.t3t.rxbuf, blockLen, &nbRead);
    if (res != ERR_NONE) {
      /* Check result */
      return res;
    } else if (nbRead != NDEF_T3T_BLOCKLEN) {
      /* Check length */
      return ERR_MEM_CORRUPT;
    } else {
      /* MISRA: PRQA requires to check the length to copy, IAR doesn't */
      if (currentLen > 0U) {
        (void)ST_MEMCPY(&buf[lvRcvLen], ctx->subCtx.t3t.rxbuf, (uint32_t)currentLen);
      }
      lvRcvLen   += (uint32_t) currentLen;
      currentLen -= (uint32_t) currentLen;
    }
  }
  if ((currentLen == 0U) && (result == ERR_NONE)) {
    result = ERR_NONE;
  }
  if (rcvdLen != NULL) {
    *rcvdLen = lvRcvLen;
  }
  return result;
}

/*******************************************************************************/
static ReturnCode ndefT3TPollerReadAttributeInformationBlock(ndefContext *ctx)
{
  /* Follow 7.4.1 NDEF Detection Procedure */
  ReturnCode   retcode;
  uint8_t     *rxbuf;
  uint16_t     checksum_received;
  uint16_t     checksum_computed = 0U;
  uint16_t     rcvLen            = 0U;
  uint8_t      i;

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T3T)) {
    return ERR_PARAM;
  }
  rxbuf   = ctx->ccBuf;
  retcode = ndefT3TPollerReadBlocks(ctx, NDEF_T3T_ATTRIB_INFO_BLOCK_NB, 1U /* One block */, rxbuf, NDEF_T3T_BLOCK_SIZE, &rcvLen);
  if ((retcode != ERR_NONE) && (rcvLen != NDEF_T3T_BLOCK_SIZE)) {
    return retcode;
  }
  /* Now compute checksum */
  for (i = 0U; i < NDEF_T3T_ATTRIB_INFO_CHECKSUM_LEN; i++) {
    checksum_computed += (uint16_t) rxbuf[i];
  }
  checksum_received = ((uint16_t)rxbuf[NDEF_T3T_ATTRIB_INFO_CHECKSUM_LEN] << 8U) + (uint16_t)rxbuf[NDEF_T3T_ATTRIB_INFO_CHECKSUM_LEN + 1U];
  if (checksum_received !=  checksum_computed) {
    return ERR_REQUEST;
  }

  /* Now copy the attribute struct */
  ctx->cc.t3t.majorVersion  = (rxbuf[NDEF_T3T_ATTRIB_INFO_OFFSET_VERSION] >> 4U);
  ctx->cc.t3t.minorVersion  = (rxbuf[NDEF_T3T_ATTRIB_INFO_OFFSET_VERSION] & 0xFU);
  ctx->cc.t3t.nbR           = rxbuf[NDEF_T3T_ATTRIB_INFO_OFFSET_NBR];
  ctx->cc.t3t.nbW           = rxbuf[NDEF_T3T_ATTRIB_INFO_OFFSET_NBW];
  ctx->cc.t3t.nMaxB         = ((uint16_t)rxbuf[NDEF_T3T_ATTRIB_INFO_OFFSET_MAXB] << 8U) + (uint16_t)rxbuf[NDEF_T3T_ATTRIB_INFO_OFFSET_MAXB + 1U];
  ctx->cc.t3t.writeFlag     = rxbuf[NDEF_T3T_ATTRIB_INFO_OFFSET_FLAG_W];
  ctx->cc.t3t.rwFlag        = rxbuf[NDEF_T3T_ATTRIB_INFO_OFFSET_FLAG_RW];
  ctx->cc.t3t.Ln            = ((uint32_t)rxbuf[NDEF_T3T_ATTRIB_INFO_OFFSET_FLAG_LN + 0U] << 0x10U)
                              | ((uint32_t)rxbuf[NDEF_T3T_ATTRIB_INFO_OFFSET_FLAG_LN + 1U] << 0x8U)
                              | (uint32_t)rxbuf[NDEF_T3T_ATTRIB_INFO_OFFSET_FLAG_LN + 2U];
  return ERR_NONE;
}

/*******************************************************************************/
ReturnCode ndefT3TPollerContextInitialization(ndefContext *ctx, const ndefDevice *dev)
{
  if ((ctx == NULL) || (dev == NULL) || !ndefT3TisT3TDevice(dev)) {
    return ERR_PARAM;
  }

  (void)ST_MEMCPY(&ctx->device, dev, sizeof(ctx->device));

  ST_MEMCPY(&ctx->subCtx.t3t.NFCID2, dev->dev.nfcf.sensfRes.NFCID2, sizeof(ctx->subCtx.t3t.NFCID2));

  ctx->type                    = NDEF_DEV_T3T;
  ctx->state                   = NDEF_STATE_INVALID;

  return ERR_NONE;
}

/*******************************************************************************/
ReturnCode ndefT3TPollerNdefDetect(ndefContext *ctx, ndefInfo *info)
{
  ReturnCode        retcode;
  rfalFeliCaPollRes pollRes[NDEF_T3T_MAX_DEVICE];
  uint8_t           devCnt     = NDEF_T3T_MAX_DEVICE;
  uint8_t           collisions = 0U;

  if (info != NULL) {
    info->state                = NDEF_STATE_INVALID;
    info->majorVersion         = 0U;
    info->minorVersion         = 0U;
    info->areaLen              = 0U;
    info->areaAvalableSpaceLen = 0U;
    info->messageLen           = 0U;
  }

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T3T)) {
    return ERR_PARAM;
  }

  RfalNfcClass *rfal_nfc = ((NdefClass *)(ctx->ndef_class_instance))->rfal_nfc;

  ctx->state = NDEF_STATE_INVALID;

  /* TS T3T v1.0 7.4.1.1 the Reader/Writer SHALL send a SENSF_REQ Command with System Code set to 12FCh. */
  retcode = rfal_nfc->rfalNfcfPollerPoll(RFAL_FELICA_1_SLOT, NDEF_T3T_SYSTEMCODE, (uint8_t)RFAL_FELICA_POLL_RC_NO_REQUEST, pollRes, &devCnt, &collisions);
  if (retcode != ERR_NONE) {
    /* TS T3T v1.0 7.4.1.2 Conclude procedure. */
    return retcode;
  }

  /* Check if UID of the first card is the same */
  if (ST_BYTECMP(&(pollRes[0U][NDEF_T3T_SENSFRES_NFCID2]), ctx->subCtx.t3t.NFCID2, RFAL_NFCF_NFCID2_LEN) != 0) {
    return ERR_REQUEST; /* Wrong UID */
  }

  /* TS T3T v1.0 7.4.1.3 The Reader/Writer SHALL read the Attribute Information Block using the CHECK Command. */
  /* TS T3T v1.0 7.4.1.4 The Reader/Writer SHALL verify the value of Checksum of the Attribute Information Block. */
  retcode = ndefT3TPollerReadAttributeInformationBlock(ctx);
  if (retcode != ERR_NONE) {
    return retcode;
  }

  /* TS T3T v1.0 7.4.1.6 The Reader/Writer SHALL check if it supports the NDEF mapping version number based on the rules given in Section 7.3. */
  if (ctx->cc.t3t.majorVersion != ndefMajorVersion(NDEF_T3T_ATTRIB_INFO_VERSION_1_0)) {
    return ERR_REQUEST;
  }

  ctx->messageLen     = ctx->cc.t3t.Ln;
  ctx->messageOffset  = NDEF_T3T_AREA_OFFSET;
  ctx->areaLen        = (uint32_t)ctx->cc.t3t.nMaxB * NDEF_T3T_BLOCK_SIZE;
  ctx->state          = NDEF_STATE_INITIALIZED;
  if (ctx->messageLen > 0U) {
    if (ctx->cc.t3t.rwFlag == NDEF_T3T_FLAG_RW) {
      ctx->state = NDEF_STATE_READWRITE;
    } else {
      if (ctx->cc.t3t.rwFlag == NDEF_T3T_FLAG_RO) {
        ctx->state = NDEF_STATE_READONLY;
      }
    }
  }

  if (info != NULL) {
    info->state                = ctx->state;
    info->majorVersion         = ctx->cc.t3t.majorVersion;
    info->minorVersion         = ctx->cc.t3t.minorVersion;
    info->areaLen              = ctx->areaLen;
    info->areaAvalableSpaceLen = ctx->areaLen;
    info->messageLen           = ctx->messageLen;
  }

  return ERR_NONE;
}

/*******************************************************************************/
ReturnCode ndefT3TPollerReadRawMessage(ndefContext *ctx, uint8_t *buf, uint32_t bufLen, uint32_t *rcvdLen, bool single)
{
  ReturnCode ret;

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T3T) || (buf == NULL)) {
    return ERR_PARAM;
  }
  ndefT3TLogD("ndefT3TPollerReadRawMessage messageLen: %d\r\n", ctx->messageLen);

  /* TS T3T v1.0 7.4.2: This procedure assumes that the Reader/Writer has successfully performed the NDEF detection procedure at least once. */

  if (!single) {
    ctx->state = NDEF_STATE_INVALID;
    ret = ndefT3TPollerReadAttributeInformationBlock(ctx);
    if (ret != ERR_NONE) {
      return ret;
    }
    ctx->messageLen = ctx->cc.t3t.Ln;
    if (ctx->messageLen == 0U) {
      if (ctx->cc.t3t.rwFlag != NDEF_T3T_FLAG_RW) {
        return ERR_WRONG_STATE;
      }
      ctx->state = NDEF_STATE_INITIALIZED;
    } else {
      if (ctx->cc.t3t.rwFlag == NDEF_T3T_FLAG_RW) {
        ctx->state = NDEF_STATE_READWRITE;
      } else {
        if (ctx->cc.t3t.rwFlag != NDEF_T3T_FLAG_RO) {
          return ERR_WRONG_STATE;
        }
        ctx->state = NDEF_STATE_READONLY;
      }
    }
  }

  if (ctx->state <= NDEF_STATE_INITIALIZED) {
    return ERR_WRONG_STATE;
  }
  /* TS T3T v1.0 7.4.2.1: If the WriteFlag remembered during the NDEF detection procedure is set to ON, the NDEF data may be inconsistent ...*/
  if (ndefT3TIsWriteFlagON(ctx->cc.t3t.writeFlag)) {
    /*  TS T3T v1.0 7.4.2.1: ... the Reader/Writer SHALL conclude the NDEF read procedure*/
    return ERR_WRONG_STATE;
  }

  if (ctx->messageLen > bufLen) {
    return ERR_NOMEM;
  }

  /*  TS T3T v1.0 7.4.2.2: Read NDEF data */
  ret = ndefT3TPollerReadBytes(ctx, ctx->messageOffset, ctx->messageLen, buf, rcvdLen);
  if (ret != ERR_NONE) {
    ctx->state = NDEF_STATE_INVALID;
  }
  return ret;
}

#if NDEF_FEATURE_FULL_API

/*******************************************************************************/
static ReturnCode ndefT3TPollerWriteBlocks(ndefContext *ctx, uint16_t blockNum, uint8_t nbBlocks, const uint8_t *dataBlocks)
{
  ReturnCode                 ret;
  rfalNfcfServBlockListParam servBlock;
  rfalNfcfBlockListElem     *listBlocks;
  uint8_t                    index;
  rfalNfcfServ               serviceCodeLst = 0x0009U;

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T3T)) {
    return ERR_PARAM;
  }

  RfalNfcClass *rfal_nfc = ((NdefClass *)(ctx->ndef_class_instance))->rfal_nfc;

  listBlocks = ctx->subCtx.t3t.listBlocks;

  for (index = 0U; index < nbBlocks; index++) {
    /* Write each block number (16 bits per block address) */
    listBlocks[index].blockNum = (uint16_t)(blockNum + (uint16_t) index);
    listBlocks[index].conf     = ((listBlocks[index].blockNum > 0xFFU) ? 0U : RFAL_NFCF_BLOCKLISTELEM_LEN_BIT);
  }
  servBlock.numServ   = 1U;
  servBlock.servList  = &serviceCodeLst;
  servBlock.numBlock  = nbBlocks;
  servBlock.blockList = listBlocks;

  ret = rfal_nfc->rfalNfcfPollerUpdate(ctx->subCtx.t3t.NFCID2, &servBlock, ctx->subCtx.t3t.txbuf, (uint16_t)sizeof(ctx->subCtx.t3t.txbuf), dataBlocks, ctx->subCtx.t3t.rxbuf, (uint16_t)sizeof(ctx->subCtx.t3t.rxbuf));

  return ret;
}

/*******************************************************************************/
ReturnCode ndefT3TPollerWriteBytes(ndefContext *ctx, uint32_t offset, const uint8_t *buf, uint32_t len, bool pad, bool writeTerminator)
{
  uint16_t        nbRead;
  uint16_t        nbWrite;
  uint16_t        res;
  uint32_t        currentLen = len;
  uint32_t        txtLen     = 0U;
  const uint16_t  blockLen   = (uint16_t) NDEF_T3T_BLOCKLEN;
  uint16_t        nbBlocks   = (uint16_t) NDEF_T3T_MAX_NB_BLOCKS;
  uint16_t        startBlock = (uint16_t)(offset / blockLen);
  uint16_t        startAddr  = (uint16_t)(startBlock * blockLen);
  uint16_t        startOffset = (uint16_t)(offset - (uint32_t) startAddr);
  uint8_t         tmpBuf[NDEF_T3T_BLOCKLEN];

  NO_WARNING(writeTerminator); /* Unused parameter */

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T3T) || (len == 0U)) {
    return ERR_PARAM;
  }
  if (ctx->state != NDEF_STATE_INVALID) {
    nbBlocks = MIN(ctx->cc.t3t.nbW, NDEF_T3T_MAX_NB_BLOCKS);
  }

  if (startOffset != 0U) {
    /* Unaligned write, need to use a tmp buffer */
    res = ndefT3TPollerReadBlocks(ctx, startBlock, 1, tmpBuf, blockLen, &nbRead);
    if (res != ERR_NONE) {
      /* Check result */
      return res;
    }
    if (nbRead != blockLen) {
      /* Check length */
      return ERR_PROTO;
    }
    /* Fill the rest of the buffer with user data */
    nbWrite =  NDEF_T3T_BLOCKLEN - startOffset;
    if (nbWrite >  len) {
      nbWrite = (uint16_t) len;
    }
    (void)ST_MEMCPY(&tmpBuf[startOffset], buf, nbWrite);
    if (pad) {
      startOffset += nbWrite;
      nbWrite =  NDEF_T3T_BLOCKLEN - startOffset;
      if (nbWrite > 0U) {
        (void)ST_MEMSET(&tmpBuf[startOffset], 0x00, nbWrite);
      }
    }
    res = ndefT3TPollerWriteBlocks(ctx, startBlock, 1U /* One block */, tmpBuf);
    if (res != ERR_NONE) {
      return res;
    }
    txtLen     += (uint32_t) nbWrite;
    currentLen -= (uint32_t) nbWrite;
    startBlock++;
  }
  while (currentLen >= (uint32_t)blockLen) {
    if (currentLen < ((uint32_t)blockLen * nbBlocks)) {
      /* Reduce the nb of blocks to read */
      nbBlocks = (uint16_t)(currentLen / blockLen);
    }
    nbWrite = blockLen * nbBlocks;
    res     = ndefT3TPollerWriteBlocks(ctx, startBlock, (uint8_t) nbBlocks, &buf[txtLen]);
    if (res != ERR_NONE) {
      return res;
    }
    txtLen     += nbWrite;
    currentLen -= nbWrite;
    startBlock += nbBlocks;
  }
  if (currentLen > 0U) {
    /* Unaligned write, need to use a tmp buffer */
    if (pad) {
      (void)ST_MEMSET(tmpBuf, 0x00, NDEF_T3T_BLOCKLEN);
    } else {
      res = ndefT3TPollerReadBlocks(ctx, startBlock, 1U /* One block */, tmpBuf, blockLen, &nbRead);
      if (res != ERR_NONE) {
        /* Check result */
        return res;
      }
      if (nbRead != blockLen) {
        /* Check length */
        return ERR_PROTO;
      }
    }
    /* Fill the beginning of the buffer with user data */
    (void)ST_MEMCPY(tmpBuf, &buf[txtLen], currentLen);
    res = ndefT3TPollerWriteBlocks(ctx, startBlock, 1U /* One block */, tmpBuf);
    if (res != ERR_NONE) {
      return res;
    }
  }

  return ERR_NONE;
}

/*******************************************************************************/
static ReturnCode ndefT3TPollerWriteAttributeInformationBlock(ndefContext *ctx)
{
  uint8_t    dataIt;
  uint16_t   checksum;
  uint8_t   *buf;
  ReturnCode ret;

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T3T)) {
    return ERR_PARAM;
  }
  if (ctx->state < NDEF_STATE_INITIALIZED) {
    return ERR_WRONG_STATE;
  }
  dataIt        = 0U;
  buf           = ctx->ccBuf;
  checksum      = 0U;
  buf[dataIt]   = ((uint8_t)(ctx->cc.t3t.majorVersion << 4U)) |  ctx->cc.t3t.minorVersion; /* Byte  0 Ver          */
  checksum     += buf[dataIt];
  dataIt++;
  buf[dataIt]   = ctx->cc.t3t.nbR;                                                         /* Byte  1 Nbr          */
  checksum     += buf[dataIt];
  dataIt++;
  buf[dataIt]   = ctx->cc.t3t.nbW;                                                         /* Byte  2 Nbw          */
  checksum     += buf[dataIt];
  dataIt++;
  buf[dataIt]   = (uint8_t)(ctx->cc.t3t.nMaxB >> 8U);                                      /* Byte  3 NmaxB  (MSB) */
  checksum     += buf[dataIt];
  dataIt++;
  buf[dataIt]   = (uint8_t)(ctx->cc.t3t.nMaxB >> 0U);                                      /* Byte  4 NmaxB  (LSB) */
  checksum     += buf[dataIt];
  dataIt++;
  buf[dataIt] = 0U;                                                                        /* Byte  5 RFU          */
  dataIt++;
  buf[dataIt] = 0U;                                                                        /* Byte  6 RFU          */
  dataIt++;
  buf[dataIt] = 0U;                                                                        /* Byte  7 RFU          */
  dataIt++;
  buf[dataIt] = 0U;                                                                        /* Byte  8 RFU          */
  dataIt++;
  buf[dataIt]   = ctx->cc.t3t.writeFlag;                                                   /* Byte  9 WriteFlag    */
  checksum     += buf[dataIt];
  dataIt++;
  buf[dataIt]   = ctx->cc.t3t.rwFlag;                                                      /* Byte 10 RWFlag       */
  checksum     += buf[dataIt];
  dataIt++;
  buf[dataIt]   = (uint8_t)(ctx->cc.t3t.Ln >> 16U);                                        /* Byte 11 Ln (MSB)     */
  checksum     += buf[dataIt];
  dataIt++;
  buf[dataIt]   = (uint8_t)(ctx->cc.t3t.Ln >>  8U);                                        /* Byte 12 Ln (middle)  */
  checksum     += buf[dataIt];
  dataIt++;
  buf[dataIt]   = (uint8_t)(ctx->cc.t3t.Ln >>  0U);                                        /* Byte 13 Ln (LSB)     */
  checksum     += buf[dataIt];
  dataIt++;
  buf[dataIt] = (uint8_t)(checksum >> 8U);                                                 /* Byte 14 checksum MSB */
  dataIt++;
  buf[dataIt] = (uint8_t)(checksum >> 0U);                                                 /* Byte 15 checksum LSB */
  dataIt++;

  ret = ndefT3TPollerWriteBlocks(ctx, NDEF_T3T_ATTRIB_INFO_BLOCK_NB, 1U /* One block */, buf);
  return ret;
}

/*******************************************************************************/
ReturnCode ndefT3TPollerWriteRawMessage(ndefContext *ctx, const uint8_t *buf, uint32_t bufLen)
{
  ReturnCode ret;

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T3T) || ((buf == NULL) && (bufLen != 0U))) {
    return ERR_PARAM;
  }
  /* TS T3T v1.0 7.4.3: This procedure assumes that the Reader/Writer has successfully performed the NDEF detection procedure... */
  /* Warning: current tag content must not be changed between NDEF Detect procedure and NDEF read procedure*/

  /* TS T3T v1.0 7.4.3: ... and that the RWFlag in the Attribute Information Block is set to 01h. */
  if ((ctx->state != NDEF_STATE_INITIALIZED) && (ctx->state != NDEF_STATE_READWRITE)) {
    /* Conclude procedure */
    return ERR_WRONG_STATE;
  }

  /* TS T3T v1.0 7.4.3.2: verify available space */
  ret = ndefT3TPollerCheckAvailableSpace(ctx, bufLen);
  if (ret != ERR_NONE) {
    /* Conclude procedure */
    return ERR_PARAM;
  }

  /* TS T3T v1.0 7.4.3.3: update WriteFlag */
  ret = ndefT3TPollerBeginWriteMessage(ctx, bufLen);
  if (ret != ERR_NONE) {
    ctx->state = NDEF_STATE_INVALID;
    /* Conclude procedure */
    return ret;
  }

  if (bufLen != 0U) {
    /* TS T3T v1.0 7.4.3.4: write new NDEF message */
    ret = ndefT3TPollerWriteBytes(ctx, ctx->messageOffset, buf, bufLen, true, false);
    if (ret != ERR_NONE) {
      /* Conclude procedure */
      ctx->state = NDEF_STATE_INVALID;
      return ret;
    }
  }
  /* TS T3T v1.0 7.4.3.5: update Ln value and set WriteFlag to OFF */
  ret = ndefT3TPollerEndWriteMessage(ctx, bufLen, false);
  if (ret != ERR_NONE) {
    /* Conclude procedure */
    ctx->state = NDEF_STATE_INVALID;
    return ret;
  }
  return ret;
}

/*******************************************************************************/
ReturnCode ndefT3TPollerTagFormat(ndefContext *ctx, const ndefCapabilityContainer *cc, uint32_t options)
{
  ReturnCode        res;
  rfalFeliCaPollRes buffOut[NDEF_T3T_MAX_DEVICE];
  uint8_t           devCnt     = NDEF_T3T_MAX_DEVICE;
  uint8_t           collisions = 0U;
  NO_WARNING(options); /* options not used in T3T */

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T3T)) {
    return ERR_PARAM;
  }

  RfalNfcClass *rfal_nfc = ((NdefClass *)(ctx->ndef_class_instance))->rfal_nfc;

  if (cc == NULL) {
    /* No default CC found so have to analyse the tag */
    res = ndefT3TPollerReadAttributeInformationBlock(ctx);  /* Read current cc */
    if (res != ERR_NONE) {
      return res;
    }
  } else {
    /* Nothing to do */
    (void)ST_MEMCPY(&ctx->cc, cc, sizeof(ndefCapabilityContainer));
  }

  /* 4.3.3 System Definition Information for SystemCode = 0x12FC (NDEF) */
  res = rfal_nfc->rfalNfcfPollerPoll(RFAL_FELICA_1_SLOT, NDEF_T3T_SYSTEMCODE, (uint8_t)RFAL_FELICA_POLL_RC_NO_REQUEST, buffOut, &devCnt, &collisions);
  if (res != ERR_NONE) {
    return res;
  }
  res = rfal_nfc->rfalNfcfPollerPoll(RFAL_FELICA_1_SLOT, NDEF_T3T_SYSTEMCODE, (uint8_t)RFAL_FELICA_POLL_RC_SYSTEM_CODE, buffOut, &devCnt, &collisions);
  if (res != ERR_NONE) {
    return res;
  }
  ctx->state            = NDEF_STATE_INITIALIZED; /* to be sure that the block will be written */
  ctx->cc.t3t.Ln        = 0U; /* Force actual stored NDEF size to 0 */
  ctx->cc.t3t.writeFlag = 0U; /* Force WriteFlag to 0 */
  res = ndefT3TPollerWriteAttributeInformationBlock(ctx);
  return res;
}

/*******************************************************************************/
ReturnCode ndefT3TPollerCheckPresence(ndefContext *ctx)
{
  ReturnCode        retcode;
  uint16_t          nbRead;

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T3T)) {
    return ERR_PARAM;
  }
  /* Perform a simple readblock */
  retcode = ndefT3TPollerReadBlocks(ctx, 0U /* First block */, 1U /* One Block */, ctx->subCtx.t3t.rxbuf, NDEF_T3T_BLOCKLEN, &nbRead);
  return retcode;
}

/*******************************************************************************/
ReturnCode ndefT3TPollerCheckAvailableSpace(const ndefContext *ctx, uint32_t messageLen)
{
  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T3T)) {
    return ERR_PARAM;
  }

  if (ctx->state == NDEF_STATE_INVALID) {
    return ERR_WRONG_STATE;
  }
  if (messageLen  > ctx->areaLen) {
    return ERR_NOMEM;
  }
  return ERR_NONE;
}

/*******************************************************************************/
ReturnCode ndefT3TPollerBeginWriteMessage(ndefContext *ctx, uint32_t messageLen)
{
  ReturnCode ret;
  NO_WARNING(messageLen);

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T3T)) {
    return ERR_PARAM;
  }

  if ((ctx->state != NDEF_STATE_INITIALIZED) && (ctx->state != NDEF_STATE_READWRITE)) {
    return ERR_WRONG_STATE;
  }
  /* Update WriteFlag */
  ctx->cc.t3t.writeFlag = NDEF_T3T_WRITEFLAG_ON;
  ret                   = ndefT3TPollerWriteAttributeInformationBlock(ctx);
  if (ret != ERR_NONE) {
    /* Conclude procedure */
    ctx->state = NDEF_STATE_INVALID;
    return ret;
  }
  ctx->state = NDEF_STATE_INITIALIZED;
  return ERR_NONE;
}

/*******************************************************************************/
ReturnCode ndefT3TPollerEndWriteMessage(ndefContext *ctx, uint32_t messageLen, bool writeTerminator)
{
  ReturnCode ret;

  NO_WARNING(writeTerminator); /* Unused parameter */

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T3T)) {
    return ERR_PARAM;
  }
  if (ctx->state != NDEF_STATE_INITIALIZED) {
    return ERR_WRONG_STATE;
  }
  /* TS T3T v1.0 7.4.3.5 Update Attribute Information Block */
  ctx->cc.t3t.writeFlag = NDEF_T3T_WRITEFLAG_OFF;
  ctx->cc.t3t.Ln        = messageLen;
  ret                   = ndefT3TPollerWriteAttributeInformationBlock(ctx);
  if (ret != ERR_NONE) {
    /* Conclude procedure */
    ctx->state = NDEF_STATE_INVALID;
    return ret;
  }
  ctx->messageLen = messageLen;
  ctx->state      = (ctx->messageLen == 0U) ? NDEF_STATE_INITIALIZED : NDEF_STATE_READWRITE;
  return ERR_NONE;
}

/*******************************************************************************/
ReturnCode ndefT3TPollerWriteRawMessageLen(ndefContext *ctx, uint32_t rawMessageLen, bool writeTerminator)
{
  NO_WARNING(writeTerminator); /* Unused parameter */

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T3T)) {
    return ERR_PARAM;
  }

  if ((ctx->state != NDEF_STATE_INITIALIZED) && (ctx->state != NDEF_STATE_READWRITE)) {
    return ERR_WRONG_STATE;
  }
  return ndefT3TPollerEndWriteMessage(ctx, rawMessageLen, false);
}

/*******************************************************************************/
ReturnCode ndefT3TPollerSetReadOnly(ndefContext *ctx)
{
  ReturnCode ret;

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T3T)) {
    return ERR_PARAM;
  }

  if (ctx->state != NDEF_STATE_READWRITE) {
    return ERR_WRONG_STATE;
  }

  /* TS T3T v1.0 7.5.2.4 */
  if (ctx->cc.t3t.writeFlag != NDEF_T3T_WRITEFLAG_OFF) {
    return ERR_WRONG_STATE;
  }

  /* TS T3T v1.0 7.5.2.4 */
  ctx->cc.t3t.rwFlag = NDEF_T3T_FLAG_RO;
  ret = ndefT3TPollerWriteAttributeInformationBlock(ctx);
  if (ret != ERR_NONE) {
    return ret;
  }

  ctx->state = NDEF_STATE_READONLY;
  return ERR_NONE;
}

#endif /* NDEF_FEATURE_FULL_API */

#endif /* NDEF_FEATURE_T3T */
