
/**
  ******************************************************************************
  * @file           : ndef_t2t.cpp
  * @brief          : Provides NDEF methods and definitions to access NFC Forum T2T
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
#include "ndef_t2t.h"
#include "nfc_utils.h"
#include "ndef_class.h"

/*
 ******************************************************************************
 * ENABLE SWITCH
 ******************************************************************************
 */

#ifndef NDEF_FEATURE_T2T
  #error " NDEF: Module configuration missing. Please enable/disable T2T module by setting: NDEF_FEATURE_T2T"
#endif

#if NDEF_FEATURE_T2T

#ifndef NDEF_FEATURE_FULL_API
  #error " NDEF: Module configuration missing. Please enable/disable Full API by setting: NDEF_FEATURE_FULL_API"
#endif

/*
 ******************************************************************************
 * GLOBAL DEFINES
 ******************************************************************************
 */

#define NDEF_T2T_BLOCK_SIZE            4U         /*!< block size                                        */
#define NDEF_T2T_MAX_SECTOR          255U         /*!< Max Number of Sector in Sector Select Command     */ /* 00h -- FEh: 255 sectors */
#define NDEF_T2T_BLOCKS_PER_SECTOR   256U         /*!< Number of Block per Sector                        */
#define NDEF_T2T_BYTES_PER_SECTOR (NDEF_T2T_BLOCKS_PER_SECTOR * NDEF_T2T_BLOCK_SIZE) /*!< Number of Bytes per Sector                        */
#define NDEF_T2T_MAX_OFFSET       (NDEF_T2T_BYTES_PER_SECTOR  * NDEF_T2T_MAX_SECTOR) /*!< Maximum offset allowed                            */
#define NDEF_T2T_3_BYTES_TLV_LEN    0xFFU         /* FFh indicates the use of 3 bytes got the L field    */
#define NDEF_T2T_STATIC_MEM_SIZE      48U         /* Static memory size                                  */

#define NDEF_T2T_CC_OFFSET            12U         /*!< CC offset                                         */
#define NDEF_T2T_CC_LEN                4U         /*!< CC length                                         */
#define NDEF_T2T_AREA_OFFSET          16U         /*!< T2T Area starts at block #4                       */

#define NDEF_T2T_MAGIC              0xE1U         /*!< CC Magic Number                                   */
#define NDEF_T2T_CC_0                  0U         /*!< CC_0: Magic Number                                */
#define NDEF_T2T_CC_1                  1U         /*!< CC_1: Version                                     */
#define NDEF_T2T_CC_2                  2U         /*!< CC_2: Size                                        */
#define NDEF_T2T_CC_3                  3U         /*!< CC_3: Access conditions                           */

#define NDEF_T2T_VERSION_1_0        0x10U         /*!< Version 1.0                                       */

#define NDEF_T2T_SIZE_DIVIDER          8U         /*!<  T2T_area size is measured in bytes is equal to 8 * Size */

#define NDEF_T2T_TLV_NULL           0x00U         /*!< Null TLV                                          */
#define NDEF_T2T_TLV_LOCK_CTRL      0x01U         /*!< Lock Control TLV                                  */
#define NDEF_T2T_TLV_MEMORY_CTRL    0x02U         /*!< Memory Control TLV                                */
#define NDEF_T2T_TLV_NDEF_MESSAGE   0x03U         /*!< NDEF Message TLV                                  */
#define NDEF_T2T_TLV_PROPRIETRARY   0xFDU         /*!< Proprietary TLV                                   */
#define NDEF_T2T_TLV_TERMINATOR     0xFEU         /*!< Terminator TLV                                    */

#define NDEF_T2T_TLV_L_3_BYTES_LEN     3U         /*!< TLV L Length: 3 bytes                             */
#define NDEF_T2T_TLV_L_1_BYTES_LEN     1U         /*!< TLV L Length: 1 byte                              */
#define NDEF_T2T_TLV_T_LEN             1U         /*!< TLV T Length: 1 byte                              */

#define NDEF_T2T_LOCK_CTRL_LEN         3U         /*!< Dyn Lock Control Length: 3 bytes                  */
#define NDEF_T2T_MEM_CTRL_LEN          3U         /*!< Memory Control Length: 3 bytes                    */

#define NDEF_T2T_DEF_BYTES_LCK_PER_BIT 8U         /*!< Default nbr of bytes locked per bit for DynLock   */

#define NDEF_T2T_WR_ACCESS_GRANTED   0x0U         /*!< Write Access 0h: Access granted w/o any security  */
#define NDEF_T2T_WR_ACCESS_NONE      0xFU         /*!< Write Access Fh: No access granted                */

#define NDEF_T2T_STATLOCK_OFFSET      10U         /*!< Static Lock offset                                */

#ifndef NDEF_T2T_N_RETRY_ERROR
  #define NDEF_T2T_N_RETRY_ERROR         1U         /*!< nT2T,RETRY,ERROR DP 2.2  �B.7                     */
#endif /* NDEF_T2T_N_RETRY_ERROR */

#define NDEF_T2T_DYN_LOCK_BYTES_MAX   32U         /*!< Max number of Dyn Lock Bytes                      */

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

#define ndefT2TisT2TDevice(device) ((((device)->type == RFAL_NFC_LISTEN_TYPE_NFCA) && ((device)->dev.nfca.type == RFAL_NFCA_T2T)))
#define ndefT2TInvalidateCache(ctx) { (ctx)->subCtx.t2t.cacheAddr = 0xFFFFFFFFU; }

#define ndefT2TIsReadOnlyAccessGranted(ctx)  (((ctx)->cc.t2t.readAccess == 0x0U) && ((ctx)->cc.t2t.writeAccess == NDEF_T2T_WR_ACCESS_NONE))
#define ndefT2TIsReadWriteAccessGranted(ctx) (((ctx)->cc.t2t.readAccess == 0x0U) && ((ctx)->cc.t2t.writeAccess == NDEF_T2T_WR_ACCESS_GRANTED))

#define ndefT2TIsTransmissionError(err)      ( ((err) == ERR_FRAMING) || ((err) == ERR_CRC) || ((err) == ERR_PAR) )

#define ndefT2TLogD(...)                                                                                  /*!< Macro for the debug log method                  */
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
static ReturnCode ndefT2TPollerReadBlock(ndefContext *ctx, uint16_t blockAddr, uint8_t *buf);

#if NDEF_FEATURE_FULL_API
  static ReturnCode ndefT2TPollerWriteBlock(ndefContext *ctx, uint16_t blockAddr, const uint8_t *buf);
#endif /* NDEF_FEATURE_FULL_API */

/*
 ******************************************************************************
 * GLOBAL FUNCTIONS
 ******************************************************************************
 */

/*******************************************************************************/
static ReturnCode ndefT2TPollerReadBlock(ndefContext *ctx, uint16_t blockAddr, uint8_t *buf)
{
  ReturnCode           ret;
  uint8_t              secNo;
  uint8_t              blNo;
  uint16_t             rcvdLen;
  uint32_t             retry;

  //ndefT2TLogD("ndefT2TPollerReadBlock 0x%2.2x\r\n", blockAddr);

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T2T) || (buf == NULL)) {
    return ERR_PARAM;
  }

  RfalNfcClass *rfal_nfc = ((NdefClass *)(ctx->ndef_class_instance))->rfal_nfc;

  secNo = (uint8_t)(blockAddr >> 8U);
  blNo  = (uint8_t)blockAddr;

  if (secNo != ctx->subCtx.t2t.currentSecNo) {
    ret = rfal_nfc->rfalT2TPollerSectorSelect(secNo);
    if (ret != ERR_NONE) {
      return ret;
    }
    ctx->subCtx.t2t.currentSecNo = secNo;
  }

  retry = NDEF_T2T_N_RETRY_ERROR;
  do {
    ret = rfal_nfc->rfalT2TPollerRead(blNo, buf, NDEF_T2T_READ_RESP_SIZE, &rcvdLen);
  } while ((retry-- != 0U) && ndefT2TIsTransmissionError(ret));

  if ((ret == ERR_NONE) && (rcvdLen != NDEF_T2T_READ_RESP_SIZE)) {
    return ERR_PROTO;
  }

  return ret;
}

/*******************************************************************************/
ReturnCode ndefT2TPollerReadBytes(ndefContext *ctx, uint32_t offset, uint32_t len, uint8_t *buf, uint32_t *rcvdLen)
{
  ReturnCode           ret;
  uint8_t              le;
  uint32_t             lvOffset = offset;
  uint32_t             lvLen    = len;
  uint8_t             *lvBuf    = buf;
  uint16_t             blockAddr;
  uint8_t              byteNo;
  uint8_t              numOfValidBlocks;

  //ndefT2TLogD("ndefT2TPollerReadBytes offset: %d, len %d\r\n", offset, len);
  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T2T) || (lvLen == 0U) || (offset > NDEF_T2T_MAX_OFFSET)) {
    return ERR_PARAM;
  }

  if ((offset >= ctx->subCtx.t2t.cacheAddr) && (offset < (ctx->subCtx.t2t.cacheAddr + NDEF_T2T_READ_RESP_SIZE)) && ((offset + len) < (ctx->subCtx.t2t.cacheAddr + NDEF_T2T_READ_RESP_SIZE))) {
    /* data in cache buffer */
    (void)ST_MEMCPY(lvBuf, &ctx->subCtx.t2t.cacheBuf[offset - ctx->subCtx.t2t.cacheAddr], len);
  } else {
    do {
      blockAddr = (uint16_t)(lvOffset / NDEF_T2T_BLOCK_SIZE);
      byteNo    = (uint8_t)(lvOffset % NDEF_T2T_BLOCK_SIZE);
      le = (lvLen < NDEF_T2T_READ_RESP_SIZE) ? (uint8_t)lvLen : (uint8_t)NDEF_T2T_READ_RESP_SIZE;
      if (((uint32_t)(uint8_t)blockAddr + (NDEF_T2T_READ_RESP_SIZE / NDEF_T2T_BLOCK_SIZE)) > NDEF_T2T_BLOCKS_PER_SECTOR) {
        numOfValidBlocks = (uint8_t)(NDEF_T2T_BLOCKS_PER_SECTOR - (uint8_t)blockAddr);
        le = MIN(le, numOfValidBlocks * NDEF_T2T_BLOCK_SIZE);
        //ndefT2TLogD("ndefT2TPollerReadBytes blockAddr: 0x%4.4x numofValidBlock: %d le: %d \r\n", blockAddr, numOfValidBlocks, le);
      } else {
        numOfValidBlocks = NDEF_T2T_READ_RESP_SIZE / NDEF_T2T_BLOCK_SIZE;
      }

      if ((byteNo != 0U) || (lvLen < NDEF_T2T_READ_RESP_SIZE)) {
        ret = ndefT2TPollerReadBlock(ctx, blockAddr, ctx->subCtx.t2t.cacheBuf);
        if (ret != ERR_NONE) {
          ndefT2TInvalidateCache(ctx);
          return ret;
        }
        ctx->subCtx.t2t.cacheAddr = (uint32_t)blockAddr * NDEF_T2T_BLOCK_SIZE;
        if ((NDEF_T2T_READ_RESP_SIZE - byteNo) < le) {
          le = NDEF_T2T_READ_RESP_SIZE - byteNo;
        }
        if (le > 0U) {
          (void)ST_MEMCPY(lvBuf, &ctx->subCtx.t2t.cacheBuf[byteNo], le);
        }
      } else {
        ret = ndefT2TPollerReadBlock(ctx, blockAddr, lvBuf);
        if (ret != ERR_NONE) {
          return ret;
        }
        if ((lvLen == le) && (numOfValidBlocks == (NDEF_T2T_READ_RESP_SIZE / NDEF_T2T_BLOCK_SIZE))) {
          /* cache the last read block */
          (void)ST_MEMCPY(&ctx->subCtx.t2t.cacheBuf[0], lvBuf, NDEF_T2T_READ_RESP_SIZE);
          ctx->subCtx.t2t.cacheAddr = (uint32_t)blockAddr * NDEF_T2T_BLOCK_SIZE;
        }
      }
      lvBuf     = &lvBuf[le];
      lvOffset += le;
      lvLen    -= le;

    } while (lvLen != 0U);
  }

  if (rcvdLen != NULL) {
    *rcvdLen = len;
  }
  return ERR_NONE;
}

/*******************************************************************************/
static ReturnCode ndefT2TPollerSplitIntoAvailableAreas(ndefContext *ctx, uint32_t offset, uint32_t len, uint32_t *physOffset, uint32_t *maxLen)
{
  uint32_t updatedOffset;
  uint32_t updatedLen;
  uint32_t i;

  //ndefT2TLogD("ndefT2TPollerSplitIntoAvailableAreas offset %d, len %d\r\n", offset, len);
  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T2T) || (physOffset == NULL) || (maxLen == NULL)) {
    return ERR_PARAM;
  }

  updatedOffset = offset;
  updatedLen    = len;

  for (i = 0; i < ctx->subCtx.t2t.nbrRsvdAreas; i++) {
    //ndefT2TLogD("ndefT2TPollerSplitIntoAvailableAreas area #%d, start: %d Len: %d\r\n", i, ctx->subCtx.t2t.rsvdAreaFirstByteAddr[i], ctx->subCtx.t2t.rsvdAreaSize[i]);
    if (updatedOffset >= ctx->subCtx.t2t.rsvdAreaFirstByteAddr[i]) {
      updatedOffset += ctx->subCtx.t2t.rsvdAreaSize[i];
    } else {
      if ((updatedOffset + len) > ctx->subCtx.t2t.rsvdAreaFirstByteAddr[i]) {
        updatedLen = ctx->subCtx.t2t.rsvdAreaFirstByteAddr[i] - updatedOffset;
      }
      break;
    }
  }
  *physOffset = updatedOffset;
  *maxLen = updatedLen;
  return ERR_NONE;
}

/*******************************************************************************/
static ReturnCode ndefT2TPollerReadBytesFromAvailableAreas(ndefContext *ctx, uint32_t offset, uint32_t len, uint8_t *buf, uint32_t *rcvdLen)
{
  ReturnCode ret;
  uint32_t curOffset;
  uint32_t curPhyOffset;
  uint32_t remainingLen;
  uint32_t curRcvdLen;
  uint32_t maxLen;

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T2T)) {
    return ERR_PARAM;
  }

  curOffset = offset;
  remainingLen = len;
  if (rcvdLen != NULL) {
    *rcvdLen = 0U;
  }
  while (remainingLen > 0U) {
    (void)ndefT2TPollerSplitIntoAvailableAreas(ctx, curOffset, remainingLen, &curPhyOffset, &maxLen);
    //ndefT2TLogD("ndefT2TPollerReadBytesFromAvailableAreas CurOffset: %d, curPhyOffset:%d Len :%d , buf offset: %d\r\n", curOffset,  curPhyOffset, maxLen, len - remainingLen);
    ret = ndefT2TPollerReadBytes(ctx, curPhyOffset, maxLen, &buf[len - remainingLen], &curRcvdLen);
    if (ret != ERR_NONE) {
      return ret;
    }
    if (rcvdLen != NULL) {
      *rcvdLen += curRcvdLen;
    }
    remainingLen -= maxLen;
    curOffset += maxLen;
  }
  return ERR_NONE;
}

/*******************************************************************************/
static ReturnCode ndefT2TReadLField(ndefContext *ctx)
{
  ReturnCode           ret;
  uint32_t             offset;
  uint8_t              data[3];
  uint16_t             lenTLV;

  ctx->state = NDEF_STATE_INVALID;
  offset = ctx->subCtx.t2t.offsetNdefTLV;
  offset++;
  ret = ndefT2TPollerReadBytesFromAvailableAreas(ctx, offset, 1, data, NULL);
  if (ret != ERR_NONE) {
    /* Conclude procedure */
    return ret;
  }
  offset++;
  lenTLV = data[0];
  if (lenTLV == NDEF_T2T_3_BYTES_TLV_LEN) {
    ret = ndefT2TPollerReadBytesFromAvailableAreas(ctx, offset, 2, data, NULL);
    if (ret != ERR_NONE) {
      /* Conclude procedure */
      return ret;
    }
    offset += 2U;
    lenTLV = GETU16(&data[0]);
  }
  ctx->messageLen    = lenTLV;
  ctx->messageOffset = offset;

  if (ctx->messageLen == 0U) {
    if (!(ndefT2TIsReadWriteAccessGranted(ctx))) {
      /* Conclude procedure  */
      return ERR_REQUEST;
    }
    /* Empty message found TS T2T v1.0 7.5.1.6 & TS T2T v1.0 7.4.2.1 */
    ctx->state = NDEF_STATE_INITIALIZED;
  } else {
    if ((ndefT2TIsReadWriteAccessGranted(ctx))) {
      /* Empty message found TS T2T v1.0 7.5.1.7 & TS T2T v1.0 7.4.3.1 */
      ctx->state = NDEF_STATE_READWRITE;
    } else {
      if (!(ndefT2TIsReadOnlyAccessGranted(ctx))) {
        /* Conclude procedure  */
        return ERR_REQUEST;
      }
      /* Empty message found TS T2T v1.0 7.5.1.7 & TS T2T v1.0 7.4.4.1 */
      ctx->state = NDEF_STATE_READONLY;
    }
  }
  return ERR_NONE;
}

/*******************************************************************************/
ReturnCode ndefT2TPollerContextInitialization(ndefContext *ctx, const ndefDevice *dev)
{
  if ((ctx == NULL) || (dev == NULL) || !ndefT2TisT2TDevice(dev)) {
    return ERR_PARAM;
  }

  (void)ST_MEMCPY(&ctx->device, dev, sizeof(ctx->device));

  ctx->type                    = NDEF_DEV_T2T;
  ctx->state                   = NDEF_STATE_INVALID;
  ctx->subCtx.t2t.currentSecNo = 0U;
  ndefT2TInvalidateCache(ctx);

  return ERR_NONE;
}

/*******************************************************************************/
ReturnCode ndefT2TPollerNdefDetect(ndefContext *ctx, ndefInfo *info)
{
  ReturnCode           ret;
  uint8_t              data[3];
  uint32_t             offset;
  uint16_t             lenTLV;
  uint8_t              typeTLV;
  uint8_t              nbrMajorOffsets;
  uint8_t              nbrMinorOffsets;
  uint8_t              majorOffsetSize;
  uint8_t              blplb;
  uint32_t             rsvdAreaFirstByteAddr;
  uint32_t             i, j;
  uint32_t             maxAddr;
  uint32_t             rsvdAreasLen;

  if (info != NULL) {
    info->state                = NDEF_STATE_INVALID;
    info->majorVersion         = 0U;
    info->minorVersion         = 0U;
    info->areaLen              = 0U;
    info->areaAvalableSpaceLen = 0U;
    info->messageLen           = 0U;
  }

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T2T)) {
    return ERR_PARAM;
  }

  ctx->state = NDEF_STATE_INVALID;

  /* Read CC TS T2T v1.0 7.5.1.1 */
  ret = ndefT2TPollerReadBytes(ctx, NDEF_T2T_CC_OFFSET, NDEF_T2T_CC_LEN, ctx->ccBuf, NULL);
  if (ret != ERR_NONE) {
    /* Conclude procedure */
    return ret;
  }
  ctx->cc.t2t.magicNumber  = ctx->ccBuf[NDEF_T2T_CC_0];
  ctx->cc.t2t.majorVersion = ndefMajorVersion(ctx->ccBuf[NDEF_T2T_CC_1]);
  ctx->cc.t2t.minorVersion = ndefMinorVersion(ctx->ccBuf[NDEF_T2T_CC_1]);
  ctx->cc.t2t.size         = ctx->ccBuf[NDEF_T2T_CC_2];
  ctx->cc.t2t.readAccess   = (uint8_t)(ctx->ccBuf[NDEF_T2T_CC_3] >> 4U);
  ctx->cc.t2t.writeAccess  = (uint8_t)(ctx->ccBuf[NDEF_T2T_CC_3] & 0xFU);
  ctx->areaLen = (uint32_t)ctx->cc.t2t.size * NDEF_T2T_SIZE_DIVIDER;
  maxAddr = ctx->areaLen + NDEF_T2T_AREA_OFFSET;
  rsvdAreasLen = 0U;
  /* Default Dyn Lock settings TS T2T v1.0 �4.7.1 */
  ctx->subCtx.t2t.dynLockFirstByteAddr     = ctx->areaLen + NDEF_T2T_AREA_OFFSET;
  ctx->subCtx.t2t.dynLockBytesLockedPerBit = NDEF_T2T_DEF_BYTES_LCK_PER_BIT;
  ctx->subCtx.t2t.dynLockNbrLockBits       = (uint16_t)(ctx->areaLen - NDEF_T2T_STATIC_MEM_SIZE + NDEF_T2T_DEF_BYTES_LCK_PER_BIT - 1U) / NDEF_T2T_DEF_BYTES_LCK_PER_BIT;
  ctx->subCtx.t2t.dynLockNbrBytes          = (ctx->subCtx.t2t.dynLockNbrLockBits + 7U) / 8U;
  ctx->subCtx.t2t.nbrRsvdAreas             = 0U;
  /* Check version number TS T2T v1.0 7.5.1.2 */
  if ((ctx->cc.t2t.magicNumber != NDEF_T2T_MAGIC) || (ctx->cc.t2t.majorVersion > ndefMajorVersion(NDEF_T2T_VERSION_1_0))) {
    /* Conclude procedure TS T2T v1.0 7.5.1.2 */
    return ERR_REQUEST;
  }
  /* Search for NDEF message TLV TS T2T v1.0 7.5.1.3 */
  offset = NDEF_T2T_AREA_OFFSET;
  while ((offset < (NDEF_T2T_AREA_OFFSET + ctx->areaLen))) {
    ret = ndefT2TPollerReadBytesFromAvailableAreas(ctx, offset, 1, data, NULL);
    if (ret != ERR_NONE) {
      /* Conclude procedure */
      return ret;
    }
    typeTLV = data[0];
    if (typeTLV == NDEF_T2T_TLV_NDEF_MESSAGE) {
      ctx->subCtx.t2t.offsetNdefTLV = offset;
    }
    offset++;
    if (typeTLV == NDEF_T2T_TLV_TERMINATOR) {
      break;
    }
    if (typeTLV == NDEF_T2T_TLV_NULL) {
      continue;
    }
    /* read TLV Len */
    ret = ndefT2TPollerReadBytesFromAvailableAreas(ctx, offset, 1, data, NULL);
    if (ret != ERR_NONE) {
      /* Conclude procedure */
      return ret;
    }
    offset++;
    lenTLV = data[0];
    if (lenTLV == NDEF_T2T_3_BYTES_TLV_LEN) {
      ret = ndefT2TPollerReadBytesFromAvailableAreas(ctx, offset, 2, data, NULL);
      if (ret != ERR_NONE) {
        /* Conclude procedure */
        return ret;
      }
      offset += 2U;
      lenTLV = GETU16(&data[0]);
    }
    if (typeTLV == NDEF_T2T_TLV_LOCK_CTRL) {
      if (lenTLV != NDEF_T2T_LOCK_CTRL_LEN) {
        return ERR_REQUEST;
      }
      if (ctx->subCtx.t2t.nbrRsvdAreas >= NDEF_T2T_MAX_RSVD_AREAS) {
        return ERR_REQUEST;
      }
      ret = ndefT2TPollerReadBytesFromAvailableAreas(ctx, offset, NDEF_T2T_LOCK_CTRL_LEN, data, NULL);
      if (ret != ERR_NONE) {
        /* Conclude procedure */
        return ret;
      }
      nbrMajorOffsets = (uint8_t)(data[0] >> 4U);
      nbrMinorOffsets = (uint8_t)(data[0] & 0x0FU);
      ctx->subCtx.t2t.dynLockNbrLockBits = (data[1] == 0U) ? 256U : (uint16_t)data[1];
      blplb           = (uint8_t)(data[2] >> 4U);
      majorOffsetSize = (uint8_t)(data[2] & 0x0FU);
      if ((blplb == 0U) || (majorOffsetSize == 0U)) {
        /* values 0h are RFU */
        return ERR_REQUEST;
      }
      ctx->subCtx.t2t.dynLockBytesLockedPerBit = (uint16_t)1U << blplb;
      ctx->subCtx.t2t.dynLockFirstByteAddr     = (nbrMajorOffsets * ((uint32_t)1U << majorOffsetSize)) + nbrMinorOffsets;
      ctx->subCtx.t2t.dynLockNbrBytes          = (ctx->subCtx.t2t.dynLockNbrLockBits + 7U) / 8U; /* TS T2T v1.0 �4.9.5 */
      rsvdAreaFirstByteAddr                    = ctx->subCtx.t2t.dynLockFirstByteAddr;
      if (rsvdAreaFirstByteAddr < maxAddr) {
        for (i = 0; i < ctx->subCtx.t2t.nbrRsvdAreas; i++) {
          if (rsvdAreaFirstByteAddr < ctx->subCtx.t2t.rsvdAreaFirstByteAddr[i]) {
            for (j = i; j < ctx->subCtx.t2t.nbrRsvdAreas; j++) {
              ctx->subCtx.t2t.rsvdAreaFirstByteAddr[j + 1U] = ctx->subCtx.t2t.rsvdAreaFirstByteAddr[j];
              ctx->subCtx.t2t.rsvdAreaSize[j + 1U]          = ctx->subCtx.t2t.rsvdAreaSize[j];
            }
            break;
          }
        }
        ctx->subCtx.t2t.rsvdAreaFirstByteAddr[i] = rsvdAreaFirstByteAddr;
        ctx->subCtx.t2t.rsvdAreaSize[i]          = ((ctx->subCtx.t2t.dynLockNbrBytes  + 3U) / 4U) * 4U;
        if ((rsvdAreaFirstByteAddr + ctx->subCtx.t2t.rsvdAreaSize[i]) > maxAddr) {
          ctx->subCtx.t2t.rsvdAreaSize[i] = (uint16_t)(maxAddr - ctx->subCtx.t2t.rsvdAreaSize[i]);
        }
        rsvdAreasLen += ctx->subCtx.t2t.rsvdAreaSize[i];
        ctx->subCtx.t2t.nbrRsvdAreas++;
      }
    }
    if (typeTLV == NDEF_T2T_TLV_MEMORY_CTRL) {
      if (lenTLV != NDEF_T2T_MEM_CTRL_LEN) {
        offset += lenTLV;
        continue;
      }
      if (ctx->subCtx.t2t.nbrRsvdAreas >= NDEF_T2T_MAX_RSVD_AREAS) {
        return ERR_REQUEST;
      }
      ret = ndefT2TPollerReadBytesFromAvailableAreas(ctx, offset, NDEF_T2T_MEM_CTRL_LEN, data, NULL);
      if (ret != ERR_NONE) {
        /* Conclude procedure */
        return ret;
      }
      nbrMajorOffsets = (uint8_t)(data[0] >> 4U);
      nbrMinorOffsets = (uint8_t)(data[0] & 0x0FU);
      majorOffsetSize = (uint8_t)(data[2] & 0x0FU);
      if (majorOffsetSize == 0U) {
        /* value 0h is RFU */
        return ERR_REQUEST;
      }
      rsvdAreaFirstByteAddr = (nbrMajorOffsets * ((uint32_t)1U << majorOffsetSize)) + nbrMinorOffsets;
      if (rsvdAreaFirstByteAddr < maxAddr) {
        for (i = 0; i < ctx->subCtx.t2t.nbrRsvdAreas; i++) {
          if (rsvdAreaFirstByteAddr < ctx->subCtx.t2t.rsvdAreaFirstByteAddr[i]) {
            for (j = i; j < ctx->subCtx.t2t.nbrRsvdAreas; j++) {
              ctx->subCtx.t2t.rsvdAreaFirstByteAddr[j + 1U] = ctx->subCtx.t2t.rsvdAreaFirstByteAddr[j];
              ctx->subCtx.t2t.rsvdAreaSize[j + 1U]          = ctx->subCtx.t2t.rsvdAreaSize[j];
            }
            break;
          }
        }
        ctx->subCtx.t2t.rsvdAreaFirstByteAddr[i] = rsvdAreaFirstByteAddr;
        ctx->subCtx.t2t.rsvdAreaSize[i] = (data[1] == 0U) ? 256U : (uint16_t)data[1];
        if ((rsvdAreaFirstByteAddr + ctx->subCtx.t2t.rsvdAreaSize[i]) > maxAddr) {
          ctx->subCtx.t2t.rsvdAreaSize[i] = (uint16_t)(maxAddr - ctx->subCtx.t2t.rsvdAreaSize[i]);
        }
        rsvdAreasLen += ctx->subCtx.t2t.rsvdAreaSize[i];
        ctx->subCtx.t2t.nbrRsvdAreas++;
      }
    }
    /* NDEF message present TLV TS T2T v1.0 7.5.1.4 */
    if (typeTLV == NDEF_T2T_TLV_NDEF_MESSAGE) {
      /* Read length TS T2T v1.0 7.5.1.5 */
      ctx->messageLen    = lenTLV;
      ctx->messageOffset = offset;
      if (ctx->messageLen == 0U) {
        if (!(ndefT2TIsReadWriteAccessGranted(ctx))) {
          /* Conclude procedure  */
          return ERR_REQUEST;
        }
        /* Empty message found TS T2T v1.0 7.5.1.6 & TS T2T v1.0 7.4.2.1 */
        ctx->state = NDEF_STATE_INITIALIZED;
      } else {
        if ((ndefT2TIsReadWriteAccessGranted(ctx))) {
          /* Empty message found TS T2T v1.0 7.5.1.7 & TS T2T v1.0 7.4.3.1 */
          ctx->state = NDEF_STATE_READWRITE;
        } else {
          if (!(ndefT2TIsReadOnlyAccessGranted(ctx))) {
            /* Conclude procedure  */
            return ERR_REQUEST;
          }
          /* Empty message found TS T2T v1.0 7.5.1.7 & TS T2T v1.0 7.4.4.1 */
          ctx->state = NDEF_STATE_READONLY;
        }
      }
      ctx->areaLen -= rsvdAreasLen;
      if (info != NULL) {
        info->state                = ctx->state;
        info->majorVersion         = ctx->cc.t2t.majorVersion;
        info->minorVersion         = ctx->cc.t2t.minorVersion;
        info->areaLen              = ctx->areaLen;
        info->areaAvalableSpaceLen = ctx->areaLen - ctx->messageOffset;
        info->messageLen           = ctx->messageLen;
      }
      return ERR_NONE;
    }
    offset += lenTLV;
  }
  return ERR_REQUEST;
}

/*******************************************************************************/
ReturnCode ndefT2TPollerReadRawMessage(ndefContext *ctx, uint8_t *buf, uint32_t bufLen, uint32_t *rcvdLen, bool single)
{
  ReturnCode ret;

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T2T) || (buf == NULL)) {
    return ERR_PARAM;
  }

  /* TS T2T v1.0 7.5.2.1: T2T NDEF Detect should have been called at least once before NDEF read procedure */

  if (! single) {
    ndefT2TInvalidateCache(ctx);
    ret = ndefT2TReadLField(ctx);
    if (ret != ERR_NONE) {
      /* Conclude procedure */
      return ret;
    }
  }

  /* TS T2T v1.0 7.5.2.3: check presence of NDEF message */
  if (ctx->state <= NDEF_STATE_INITIALIZED) {
    /* Conclude procedure TS T4T v1.0 7.2.2.2 */
    return ERR_WRONG_STATE;
  }

  if (ctx->messageLen > bufLen) {
    return ERR_NOMEM;
  }

  ret = ndefT2TPollerReadBytesFromAvailableAreas(ctx, ctx->messageOffset, ctx->messageLen, buf, rcvdLen);
  if (ret != ERR_NONE) {
    ctx->state = NDEF_STATE_INVALID;
  }
  return ret;
}

#if NDEF_FEATURE_FULL_API

/*******************************************************************************/
static ReturnCode ndefT2TPollerWriteBlock(ndefContext *ctx, uint16_t blockAddr, const uint8_t *buf)
{
  ReturnCode ret;
  uint8_t    secNo;
  uint8_t    blNo;
  uint32_t   retry;

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T2T) || (buf == NULL)) {
    return ERR_PARAM;
  }

  RfalNfcClass *rfal_nfc = ((NdefClass *)(ctx->ndef_class_instance))->rfal_nfc;

  secNo = (uint8_t)(blockAddr >> 8U);
  blNo  = (uint8_t)blockAddr;

  if (secNo != ctx->subCtx.t2t.currentSecNo) {
    ret = rfal_nfc->rfalT2TPollerSectorSelect(secNo);
    if (ret != ERR_NONE) {
      return ret;
    }
    ctx->subCtx.t2t.currentSecNo = secNo;
  }

  retry = NDEF_T2T_N_RETRY_ERROR;
  do {
    ret = rfal_nfc->rfalT2TPollerWrite(blNo, buf);
  } while ((retry-- != 0U) && ndefT2TIsTransmissionError(ret));

  return ret;
}

/*******************************************************************************/
ReturnCode ndefT2TPollerWriteBytes(ndefContext *ctx, uint32_t offset, const uint8_t *buf, uint32_t len, bool pad, bool writeTerminator)
{
  ReturnCode           ret;
  uint32_t             lvOffset = offset;
  uint32_t             lvLen    = len;
  const uint8_t       *lvBuf    = buf;
  uint16_t             blockAddr;
  uint8_t              byteNo;
  uint8_t              le;
  uint8_t              tempBuf[NDEF_T2T_READ_RESP_SIZE];
  bool                 lvWriteTerminator = writeTerminator;

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T2T) || (lvLen == 0U)) {
    return ERR_PARAM;
  }

  ndefT2TInvalidateCache(ctx);

  do {
    blockAddr = (uint16_t)(lvOffset / NDEF_T2T_BLOCK_SIZE);
    byteNo    = (uint8_t)(lvOffset % NDEF_T2T_BLOCK_SIZE);
    le = (lvLen < NDEF_T2T_BLOCK_SIZE) ? (uint8_t)lvLen : (uint8_t)NDEF_T2T_BLOCK_SIZE;
    if ((byteNo != 0U) || (lvLen < NDEF_T2T_BLOCK_SIZE)) {
      if ((byteNo != 0U) || !pad) {
        ret = ndefT2TPollerReadBlock(ctx, blockAddr, tempBuf);
        if (ret != ERR_NONE) {
          return ret;
        }
      }
      if ((byteNo + lvLen) < NDEF_T2T_BLOCK_SIZE) {
        if (pad) {
          (void)ST_MEMSET(&tempBuf[byteNo + lvLen], 0x00, NDEF_T2T_BLOCK_SIZE - (byteNo + lvLen));
        }
        if (lvWriteTerminator) {
          tempBuf[byteNo + lvLen] = NDEF_T2T_TLV_TERMINATOR;
          lvWriteTerminator = false;
        }
      }
      if ((NDEF_T2T_BLOCK_SIZE - byteNo) < le) {
        le = NDEF_T2T_BLOCK_SIZE - byteNo;
      }
      if (le > 0U) {
        (void)ST_MEMCPY(&tempBuf[byteNo], lvBuf, le);
      }
      ret = ndefT2TPollerWriteBlock(ctx, blockAddr, tempBuf);
      if (ret != ERR_NONE) {
        return ret;
      }
    } else {
      ret = ndefT2TPollerWriteBlock(ctx, blockAddr, lvBuf);
      if (ret != ERR_NONE) {
        return ret;
      }
    }
    lvBuf     = &lvBuf[le];
    lvOffset += le;
    lvLen    -= le;

  } while (lvLen != 0U);
  if (lvWriteTerminator) {
    blockAddr++;
    (void)ST_MEMSET(tempBuf, 0x00, NDEF_T2T_BLOCK_SIZE);
    tempBuf[0] = NDEF_T2T_TLV_TERMINATOR;
    (void)ndefT2TPollerWriteBlock(ctx, blockAddr, tempBuf);
  }

  return ERR_NONE;
}

/*******************************************************************************/
static ReturnCode ndefT2TPollerWriteBytesToAvailableAreas(ndefContext *ctx, uint32_t offset, const uint8_t *buf, uint32_t len, bool pad, bool writeTerminator)
{
  ReturnCode ret;
  uint32_t curOffset;
  uint32_t curPhyOffset;
  uint32_t remainingLen;
  uint32_t maxLen;
  uint32_t maxLenTerm;
  bool     term = false;
  bool     lvWriteTerminator = writeTerminator;
  uint8_t  termBuf[1];

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T2T)) {
    return ERR_PARAM;
  }
  curOffset = offset;
  remainingLen = len;

  while (remainingLen > 0U) {
    (void)ndefT2TPollerSplitIntoAvailableAreas(ctx, curOffset, remainingLen, &curPhyOffset, &maxLen);
    if ((remainingLen == maxLen) &&  writeTerminator) { /* last part */
      (void)ndefT2TPollerSplitIntoAvailableAreas(ctx, curOffset, remainingLen + 1U, &curPhyOffset, &maxLenTerm);
      if ((remainingLen + 1U) == maxLenTerm) { /* check enough room for terminator in that area */
        term = true;
        lvWriteTerminator = false;
      }
    }
    ret = ndefT2TPollerWriteBytes(ctx, curPhyOffset, &buf[len - remainingLen], maxLen, (remainingLen == maxLen) && pad, term);
    //ndefT2TLogD("ndefT2TPollerWriteBytesToAvailableAreas CurOffset: %d, curPhyOffset: %d, Len :%d , buf offset: %d\r\n", curOffset, curPhyOffset, maxLen, len - remainingLen);
    if (ret != ERR_NONE) {
      return ret;
    }
    remainingLen -= maxLen;
    curOffset += maxLen;
  }
  if (lvWriteTerminator) {
    (void)ndefT2TPollerSplitIntoAvailableAreas(ctx, curOffset, 1U, &curPhyOffset, &maxLen);
    termBuf[0] = NDEF_T2T_TLV_TERMINATOR;
    (void)ndefT2TPollerWriteBytes(ctx, curPhyOffset, termBuf, 1U, pad, false);
  }
  return ERR_NONE;
}

/*******************************************************************************/
ReturnCode ndefT2TPollerWriteRawMessageLen(ndefContext *ctx, uint32_t rawMessageLen, bool writeTerminator)
{
  ReturnCode           ret;
  uint8_t              buf[NDEF_T2T_BLOCK_SIZE];
  uint8_t              dataIt;

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T2T)) {
    return ERR_PARAM;
  }

  if ((ctx->state != NDEF_STATE_INITIALIZED) && (ctx->state != NDEF_STATE_READWRITE)) {
    return ERR_WRONG_STATE;
  }

  if (writeTerminator && (rawMessageLen != 0U) && ((ctx->messageOffset + rawMessageLen) < (ctx->areaLen + NDEF_T2T_AREA_OFFSET))) {
    /* Write Terminator TLV */
    dataIt = 0U;
    buf[dataIt] = NDEF_T2T_TLV_TERMINATOR;
    dataIt++;
    ret = ndefT2TPollerWriteBytesToAvailableAreas(ctx, ctx->messageOffset + rawMessageLen, buf, dataIt, true, false);
    if (ret != ERR_NONE) {
      return ret;
    }
  }

  dataIt = 0U;
  buf[dataIt] = NDEF_T2T_TLV_NDEF_MESSAGE;
  dataIt++;
  if (rawMessageLen <= NDEF_SHORT_VFIELD_MAX_LEN) {
    buf[dataIt] = (uint8_t) rawMessageLen;
    dataIt++;
    if ((rawMessageLen == 0U) && writeTerminator) {
      buf[dataIt] = NDEF_T2T_TLV_TERMINATOR;
      dataIt++;
    }
  } else {
    buf[dataIt] = (uint8_t)(NDEF_SHORT_VFIELD_MAX_LEN + 1U);
    dataIt++;
    buf[dataIt] = (uint8_t)(rawMessageLen >> 8U);
    dataIt++;
    buf[dataIt] = (uint8_t) rawMessageLen;
    dataIt++;
  }

  ret = ndefT2TPollerWriteBytesToAvailableAreas(ctx, ctx->subCtx.t2t.offsetNdefTLV, buf, dataIt, writeTerminator && (rawMessageLen == 0U), false);
  return ret;
}

/*******************************************************************************/
ReturnCode ndefT2TPollerWriteRawMessage(ndefContext *ctx, const uint8_t *buf, uint32_t bufLen)
{
  ReturnCode ret;

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T2T) || ((buf == NULL) && (bufLen != 0U))) {
    return ERR_PARAM;
  }

  /* TS T2T v1.0 7.5.3.1/2: T4T NDEF Detect should have been called before NDEF write procedure */
  /* Warning: current tag content must not be changed between NDEF Detect procedure and NDEF Write procedure*/

  /* TS T2T v1.0 7.5.3.3: check write access condition */
  if ((ctx->state != NDEF_STATE_INITIALIZED) && (ctx->state != NDEF_STATE_READWRITE)) {
    /* Conclude procedure */
    return ERR_WRONG_STATE;
  }

  /* TS T2T v1.0 7.5.3.3: verify available space */
  ret = ndefT2TPollerCheckAvailableSpace(ctx, bufLen);
  if (ret != ERR_NONE) {
    /* Conclude procedures */
    return ERR_PARAM;
  }

  /* TS T2T v1.0 7.5.3.4: reset L_Field to 0                */
  /* and update ctx->messageOffset according to L-field len */
  ret = ndefT2TPollerBeginWriteMessage(ctx, bufLen);
  if (ret != ERR_NONE) {
    ctx->state = NDEF_STATE_INVALID;
    /* Conclude procedure */
    return ret;
  }

  if (bufLen != 0U) {
    /* TS T2T v1.0 7.5.3.5: write new NDEF message */
    ret = ndefT2TPollerWriteBytesToAvailableAreas(ctx, ctx->messageOffset, buf, bufLen, true, ndefT2TPollerCheckAvailableSpace(ctx, bufLen + 1U) == ERR_NONE);
    if (ret != ERR_NONE) {
      /* Conclude procedure */
      ctx->state = NDEF_STATE_INVALID;
      return ret;
    }

    /* TS T2T v1.0 7.5.3.6 & 7.5.3.7: update L_Field and write Terminator TLV */
    ret = ndefT2TPollerEndWriteMessage(ctx, bufLen, false);
    if (ret != ERR_NONE) {
      /* Conclude procedure */
      ctx->state = NDEF_STATE_INVALID;
      return ret;
    }
  }

  return ret;
}

/*******************************************************************************/
ReturnCode ndefT2TPollerTagFormat(ndefContext *ctx, const ndefCapabilityContainer *cc, uint32_t options)
{
  ReturnCode           ret;
  uint8_t              dataIt;
  static const uint8_t emptyNdef[] = {NDEF_T2T_TLV_NDEF_MESSAGE, 0x00U, NDEF_T2T_TLV_TERMINATOR, 0x00U};

  NO_WARNING(options);

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T2T)) {
    return ERR_PARAM;
  }

  /*
   * Read CC area
   */
  ret = ndefT2TPollerReadBytes(ctx, NDEF_T2T_CC_OFFSET, NDEF_T2T_CC_LEN, ctx->ccBuf, NULL);
  if (ret != ERR_NONE) {
    return ret;
  }

  ndefT2TInvalidateCache(ctx);

  /*
   * Write CC only in case of virgin CC area
   */
  if ((ctx->ccBuf[NDEF_T2T_CC_0] == 0U) && (ctx->ccBuf[NDEF_T2T_CC_1] == 0U) && (ctx->ccBuf[NDEF_T2T_CC_2] == 0U) && (ctx->ccBuf[NDEF_T2T_CC_3] == 0U)) {
    dataIt = 0U;
    if (cc == NULL) {
      /* Use default values if no cc provided */
      ctx->ccBuf[dataIt] = NDEF_T2T_MAGIC;
      dataIt++;
      ctx->ccBuf[dataIt] = NDEF_T2T_VERSION_1_0;
      dataIt++;
      ctx->ccBuf[dataIt] = NDEF_T2T_STATIC_MEM_SIZE / NDEF_T2T_SIZE_DIVIDER;
      dataIt++;
      ctx->ccBuf[dataIt] = 0x00U;
      dataIt++;
    } else {
      ctx->ccBuf[dataIt] = cc->t2t.magicNumber;
      dataIt++;
      ctx->ccBuf[dataIt] = (uint8_t)(cc->t2t.majorVersion << 4U) | cc->t2t.minorVersion;
      dataIt++;
      ctx->ccBuf[dataIt] = cc->t2t.size;
      dataIt++;
      ctx->ccBuf[dataIt] = (uint8_t)(cc->t2t.readAccess << 4U) | cc->t2t.writeAccess;
      dataIt++;
    }
    ret = ndefT2TPollerWriteBlock(ctx, NDEF_T2T_CC_OFFSET / NDEF_T2T_BLOCK_SIZE, ctx->ccBuf);
    if (ret != ERR_NONE) {
      return ret;
    }
  }

  /*
   * Write NDEF place holder
   */
  ret = ndefT2TPollerWriteBlock(ctx, NDEF_T2T_AREA_OFFSET / NDEF_T2T_BLOCK_SIZE, emptyNdef);

  return ret;
}

/*******************************************************************************/
ReturnCode ndefT2TPollerCheckPresence(ndefContext *ctx)
{
  ReturnCode ret;
  uint16_t   blockAddr;

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T2T)) {
    return ERR_PARAM;
  }

  blockAddr = 0U;
  ret = ndefT2TPollerReadBlock(ctx, blockAddr, ctx->subCtx.t2t.cacheBuf);
  if (ret != ERR_NONE) {
    ndefT2TInvalidateCache(ctx);
    return ret;
  }
  ctx->subCtx.t2t.cacheAddr = (uint32_t)blockAddr * NDEF_T2T_BLOCK_SIZE;
  return ERR_NONE;
}

/*******************************************************************************/
ReturnCode ndefT2TPollerCheckAvailableSpace(const ndefContext *ctx, uint32_t messageLen)
{
  uint32_t lLen;

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T2T)) {
    return ERR_PARAM;
  }

  if (ctx->state == NDEF_STATE_INVALID) {
    return ERR_WRONG_STATE;
  }

  lLen = (messageLen > NDEF_SHORT_VFIELD_MAX_LEN) ? NDEF_T2T_TLV_L_3_BYTES_LEN : NDEF_T2T_TLV_L_1_BYTES_LEN;

  if ((messageLen + ctx->subCtx.t2t.offsetNdefTLV + NDEF_T2T_TLV_T_LEN + lLen) > (ctx->areaLen + NDEF_T2T_AREA_OFFSET)) {
    return ERR_NOMEM;
  }
  return ERR_NONE;
}

/*******************************************************************************/
ReturnCode ndefT2TPollerBeginWriteMessage(ndefContext *ctx, uint32_t messageLen)
{
  ReturnCode ret;
  uint32_t   lLen;

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T2T)) {
    return ERR_PARAM;
  }

  if ((ctx->state != NDEF_STATE_INITIALIZED) && (ctx->state != NDEF_STATE_READWRITE)) {
    return ERR_WRONG_STATE;
  }

  /* TS T2T v1.0 7.5.3.4: reset L_Field to 0 */
  ret = ndefT2TPollerWriteRawMessageLen(ctx, 0U, true);
  if (ret != ERR_NONE) {
    /* Conclude procedure */
    ctx->state = NDEF_STATE_INVALID;
    return ret;
  }

  lLen = (messageLen > NDEF_SHORT_VFIELD_MAX_LEN) ? NDEF_T2T_TLV_L_3_BYTES_LEN : NDEF_T2T_TLV_L_1_BYTES_LEN;
  ctx->messageOffset  = ctx->subCtx.t2t.offsetNdefTLV;
  ctx->messageOffset += NDEF_T2T_TLV_T_LEN; /* T Len */
  ctx->messageOffset += lLen;               /* L Len */

  ctx->state = NDEF_STATE_INITIALIZED;

  return ERR_NONE;
}

/*******************************************************************************/
ReturnCode ndefT2TPollerEndWriteMessage(ndefContext *ctx, uint32_t messageLen, bool writeTerminator)
{
  ReturnCode ret;

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T2T)) {
    return ERR_PARAM;
  }

  if (ctx->state != NDEF_STATE_INITIALIZED) {
    return ERR_WRONG_STATE;
  }

  /* TS T2T v1.0 7.5.3.6 & 7.5.3.7: update L_Field and write Terminator TLV */
  ret = ndefT2TPollerWriteRawMessageLen(ctx, messageLen, writeTerminator);
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
ReturnCode ndefT2TPollerSetReadOnly(ndefContext *ctx)
{
  ReturnCode ret;
  uint32_t   nbrDynLockBits;
  uint32_t   nbrDynLockBitsInLastByte;
  uint8_t    dynLockBits[NDEF_T2T_DYN_LOCK_BYTES_MAX];
  uint32_t   i, j;
  uint32_t   maxAddr;
  uint32_t   addr;
  uint32_t   bitPos;
  uint32_t   DynLockByteIndex;

  static const uint8_t staticBits[2] = {0xFF, 0xFF};

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T2T)) {
    return ERR_PARAM;
  }

  if (ctx->state != NDEF_STATE_READWRITE) {
    return ERR_WRONG_STATE;
  }

  /* TS T2T v1.0 7.6.5.1: set the Access Conditions for Write (in the lower nibble) of CC_3 to Fh */
  ctx->cc.t2t.writeAccess = NDEF_T2T_WR_ACCESS_NONE;
  ctx->ccBuf[NDEF_T2T_CC_3] |= ctx->cc.t2t.writeAccess;
  ret = ndefT2TPollerWriteBlock(ctx, NDEF_T2T_CC_OFFSET / NDEF_T2T_BLOCK_SIZE, ctx->ccBuf);
  if (ret != ERR_NONE) {
    return ret;
  }
  /* TS T2T v1.0 7.6.5.1: set all Static Lock bits to 1b */
  ret = ndefT2TPollerWriteBytes(ctx, NDEF_T2T_STATLOCK_OFFSET, staticBits, sizeof(staticBits), false, false);
  if (ret != ERR_NONE) {
    return ret;
  }

  /*  TS T2T v1.0 7.6.5.1: set all Dynamic Lock bits that are associated with the TLVs_Area to 1b */
  if (ctx->areaLen > NDEF_T2T_STATIC_MEM_SIZE) {
    nbrDynLockBits = (((uint32_t)ctx->cc.t2t.size * NDEF_T2T_SIZE_DIVIDER) - NDEF_T2T_STATIC_MEM_SIZE + ctx->subCtx.t2t.dynLockBytesLockedPerBit - 1U) / ctx->subCtx.t2t.dynLockBytesLockedPerBit;
    for (i = 0; i < (nbrDynLockBits / 8U); i++) {
      dynLockBits[i] = 0xFFU;
    }
    nbrDynLockBitsInLastByte = nbrDynLockBits % 8U;
    if (nbrDynLockBitsInLastByte != 0U) {
      dynLockBits[i] = (1U << nbrDynLockBitsInLastByte) - 1U;
      i++;
    }

    maxAddr = ((uint32_t)ctx->cc.t2t.size * NDEF_T2T_SIZE_DIVIDER) + NDEF_T2T_AREA_OFFSET;
    /* clear dynLock bits for rsvd Areas */
    for (j = 0; (j < ctx->subCtx.t2t.nbrRsvdAreas) && (ctx->subCtx.t2t.rsvdAreaFirstByteAddr[j] <  maxAddr); j++) {
      for (addr = ((ctx->subCtx.t2t.rsvdAreaFirstByteAddr[j] + ctx->subCtx.t2t.dynLockBytesLockedPerBit - 1U) / ctx->subCtx.t2t.dynLockBytesLockedPerBit) * ctx->subCtx.t2t.dynLockBytesLockedPerBit;
           (addr < (ctx->subCtx.t2t.rsvdAreaFirstByteAddr[j] + ctx->subCtx.t2t.rsvdAreaSize[j])) && ((addr + ctx->subCtx.t2t.dynLockBytesLockedPerBit) < (ctx->subCtx.t2t.rsvdAreaFirstByteAddr[j] + ctx->subCtx.t2t.rsvdAreaSize[j]));
           addr += ctx->subCtx.t2t.dynLockBytesLockedPerBit) {
        bitPos  = (addr - (NDEF_T2T_STATIC_MEM_SIZE + NDEF_T2T_AREA_OFFSET)) / ctx->subCtx.t2t.dynLockBytesLockedPerBit;
        DynLockByteIndex = bitPos / 8U;
        if (DynLockByteIndex < NDEF_T2T_DYN_LOCK_BYTES_MAX) {
          dynLockBits[DynLockByteIndex] &= ~(1U << (bitPos % 8U));
        }
      }
    }

    ret = ndefT2TPollerWriteBytes(ctx, ctx->subCtx.t2t.dynLockFirstByteAddr, dynLockBits, i, false, false);
    if (ret != ERR_NONE) {
      return ret;
    }
  }

  return ERR_NONE;
}

#endif /* NDEF_FEATURE_FULL_API */

#endif /* NDEF_FEATURE_T2T */
