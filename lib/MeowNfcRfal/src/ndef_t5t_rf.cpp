
/**
  ******************************************************************************
  * @file           : ndef_t5t_rf.cpp
  * @brief          : Provides NDEF methods and definitions to access NFC-V Forum T5T
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
#include "ndef_t5t_hal.h"
#include "ndef_t5t.h"
#include "nfc_utils.h"
#include "rfal_st25xv.h"
#include "ndef_class.h"

/*
 ******************************************************************************
 * ENABLE SWITCH
 ******************************************************************************
 */

#ifndef NDEF_FEATURE_T5T
  #error " NDEF: Module configuration missing. Please enable/disable T5T support by setting: NDEF_FEATURE_T5T"
#endif

#if NDEF_FEATURE_T5T

#ifndef NDEF_FEATURE_FULL_API
  #error " NDEF: Module configuration missing. Please enable/disable Full API by setting: NDEF_FEATURE_FULL_API"
#endif

#ifdef TEST_NDEF
  #define NDEF_SKIP_T5T_SYS_INFO /* Must not call ndefT5TGetSystemInformation() in test mode */
#endif

/*
 ******************************************************************************
 * GLOBAL DEFINES
 ******************************************************************************
 */

#define NDEF_T5T_SYSINFO_MAX_LEN              22U    /*!< Max length for (Extended) Get System Info response */

#define NDEF_T5T_MAX_BLOCK_1_BYTE_ADDR       256U    /*!< Max number of blocks for 1 byte addressing        */


#ifndef NDEF_T5T_N_RETRY_ERROR
  #define NDEF_T5T_N_RETRY_ERROR                2U     /*!< nT5T,RETRY,ERROR DP 2.2  �B.12                    */
#endif /* NDEF_T5T_N_RETRY_ERROR */

#define NDEF_T5T_FLAG_LEN                     1U     /*!< Flag byte length                                  */


/*
 *****************************************************************************
 * GLOBAL TYPES
 ******************************************************************************
 */

#define NDEF_T5T_UID_MANUFACTURER_ID_POS       6U    /*!< Manufacturer ID Offset in UID buffer (reverse)    */
#define NDEF_T5T_MANUFACTURER_ID_ST         0x02U    /*!< Manufacturer ID for ST                            */


/*
 ******************************************************************************
 * GLOBAL MACROS
 ******************************************************************************
 */

#define ndefT5TIsTransmissionError(err)      ( ((err) == ERR_FRAMING) || ((err) == ERR_CRC) || ((err) == ERR_PAR) || ((err) == ERR_TIMEOUT) )


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
static ReturnCode ndefT5TPollerReadSingleBlock(ndefContext *ctx, uint16_t blockNum, uint8_t *rxBuf, uint16_t rxBufLen, uint16_t *rcvLen);
static ReturnCode ndefT5TPollerReadMultipleBlocks(ndefContext *ctx, uint16_t firstBlockNum, uint8_t numOfBlocks, uint8_t *rxBuf, uint16_t rxBufLen, uint16_t *rcvLen);

#if !defined NDEF_SKIP_T5T_SYS_INFO
  static ReturnCode ndefT5TGetSystemInformation(ndefContext *ctx, bool extended);
#endif /* NDEF_SKIP_T5T_SYS_INFO */

#if NDEF_FEATURE_FULL_API
  static ReturnCode ndefT5TPollerWriteSingleBlock(ndefContext *ctx, uint16_t blockNum, const uint8_t *wrData);
  static ReturnCode ndefT5TPollerLockSingleBlock(ndefContext *ctx, uint16_t blockNum);
#endif /* NDEF_FEATURE_FULL_API */

/*
 ******************************************************************************
 * GLOBAL FUNCTIONS
 ******************************************************************************
 */


/*******************************************************************************/
bool ndefT5TisSTDevice(const ndefDevice *dev)
{
  if (dev == NULL) {
    return false;
  }

  return (dev->dev.nfcv.InvRes.UID[NDEF_T5T_UID_MANUFACTURER_ID_POS] == NDEF_T5T_MANUFACTURER_ID_ST);
}


/*******************************************************************************/
bool ndefT5TisT5TDevice(const ndefDevice *dev)
{
  if (dev == NULL) {
    return false;
  }

  return dev->type == RFAL_NFC_LISTEN_TYPE_NFCV;
}


/*******************************************************************************/
ReturnCode ndefT5TPollerAccessMode(ndefContext *ctx, const ndefDevice *dev, ndefT5TAccessMode mode)
{
  ndefT5TAccessMode accessMode = mode;

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T5T) ||
      (dev == NULL)) {
    return ERR_PARAM;
  }

  RfalNfcClass *rfal_nfc = ((NdefClass *)(ctx->ndef_class_instance))->rfal_nfc;

  ctx->subCtx.t5t.flags = (uint8_t)RFAL_NFCV_REQ_FLAG_DEFAULT;

  if (accessMode == NDEF_T5T_ACCESS_MODE_SELECTED) {
    if (rfal_nfc->rfalNfcvPollerSelect(ctx->subCtx.t5t.flags, dev->dev.nfcv.InvRes.UID) == ERR_NONE) {
      /* Selected mode (AMS = 0, SMS = 1) */
      ctx->subCtx.t5t.uid    = NULL;
      ctx->subCtx.t5t.flags |= (uint8_t)RFAL_NFCV_REQ_FLAG_SELECT;
    } else {
      /* Set Addressed mode if Selected mode failed */
      accessMode = NDEF_T5T_ACCESS_MODE_ADDRESSED;
    }
  }
  if (accessMode == NDEF_T5T_ACCESS_MODE_ADDRESSED) {
    /* Addressed mode (AMS = 1, SMS = 0) */
    ctx->subCtx.t5t.uid    = dev->dev.nfcv.InvRes.UID;
    ctx->subCtx.t5t.flags |= (uint8_t)RFAL_NFCV_REQ_FLAG_ADDRESS;
  } else if (accessMode == NDEF_T5T_ACCESS_MODE_NON_ADDRESSED) {
    /* Non-addressed mode (AMS = 0, SMS = 0) */
    ctx->subCtx.t5t.uid = NULL;
  } else {
    /* MISRA 15.7 - Empty else */
  }

  return ERR_NONE;
}


/*******************************************************************************/
uint8_t ndefT5TGetBlockLength(ndefContext *ctx)
{
  ReturnCode    result;
  uint16_t      rcvLen;
  uint8_t       blockLen = 0;

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T5T)) {
    return 0;
  }

  /* GetBlockLength shall be called only once during context initialization */
  if (ctx->subCtx.t5t.blockLen == 0U) {
    /* T5T v1.1 4.1.1.3 Retrieve the Block Length */
    ctx->subCtx.t5t.legacySTHighDensity = false;
    result = ndefT5TPollerReadSingleBlock(ctx, 0U, ctx->subCtx.t5t.txrxBuf, (uint16_t)sizeof(ctx->subCtx.t5t.txrxBuf), &rcvLen);
    if ((result != ERR_NONE) && ctx->subCtx.t5t.stDevice) {
      /* Try High Density Legacy mode */
      ctx->subCtx.t5t.legacySTHighDensity = true;
      result = ndefT5TPollerReadSingleBlock(ctx, 0U, ctx->subCtx.t5t.txrxBuf, (uint16_t)sizeof(ctx->subCtx.t5t.txrxBuf), &rcvLen);
      if (result != ERR_NONE) {
        /* High Density Legacy mode not supported */
        ctx->subCtx.t5t.legacySTHighDensity = false;
        return 0;
      }
    }

    if ((rcvLen > 1U) && (ctx->subCtx.t5t.txrxBuf[0U] == (uint8_t)0U)) {
      blockLen = (uint8_t)(rcvLen - 1U);
    }
  }

  return blockLen;
}


#if !defined NDEF_SKIP_T5T_SYS_INFO
/*******************************************************************************/
ReturnCode ndefT5TGetMemoryConfig(ndefContext *ctx)
{
  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T5T)) {
    return ERR_PARAM;
  }

  if (!ctx->subCtx.t5t.legacySTHighDensity) {
    /* Extended Get System Info */
    if (ndefT5TGetSystemInformation(ctx, true) == ERR_NONE) {
      ctx->subCtx.t5t.sysInfoSupported = true;
    }
  }
  if (!ctx->subCtx.t5t.sysInfoSupported) {
    /* Get System Info */
    if (ndefT5TGetSystemInformation(ctx, false) == ERR_NONE) {
      ctx->subCtx.t5t.sysInfoSupported = true;
    }
  }

  return ERR_NONE;
}


/*******************************************************************************/
static ReturnCode ndefT5TGetSystemInformation(ndefContext *ctx, bool extended)
{
  ReturnCode                ret;
  uint8_t                   rxBuf[NDEF_T5T_SYSINFO_MAX_LEN];
  uint16_t                  rcvLen;
  uint8_t                  *resp;
  uint8_t                   flags;
  const uint8_t            *uid;

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T5T)) {
    return ERR_PARAM;
  }

  RfalNfcClass *rfal_nfc = ((NdefClass *)(ctx->ndef_class_instance))->rfal_nfc;

  uid   = ctx->subCtx.t5t.uid;
  flags = ctx->subCtx.t5t.flags;

  if (extended) {
    ret = rfal_nfc->rfalNfcvPollerExtendedGetSystemInformation(flags, uid, (uint8_t)RFAL_NFCV_SYSINFO_REQ_ALL, rxBuf, (uint16_t)sizeof(rxBuf), &rcvLen);
  } else {
    if (ctx->subCtx.t5t.legacySTHighDensity) {
      flags |= (uint8_t)RFAL_NFCV_REQ_FLAG_PROTOCOL_EXT;
    }
    ret = rfal_nfc->rfalNfcvPollerGetSystemInformation(flags, uid, rxBuf, (uint16_t)sizeof(rxBuf), &rcvLen);
  }

  if (ret != ERR_NONE) {
    return ret;
  }

  resp = &rxBuf[0U];
  /* skip Flags */
  resp++;
  /* get Info flags */
  ctx->subCtx.t5t.sysInfo.infoFlags = *resp;
  resp++;
  if (extended && (ndefT5TSysInfoLenValue(ctx->subCtx.t5t.sysInfo.infoFlags) != 0U)) {
    return ERR_PROTO;
  }
  /* get UID */
  (void)ST_MEMCPY(ctx->subCtx.t5t.sysInfo.UID, resp, RFAL_NFCV_UID_LEN);
  resp = &resp[RFAL_NFCV_UID_LEN];
  if (ndefT5TSysInfoDFSIDPresent(ctx->subCtx.t5t.sysInfo.infoFlags) != 0U) {
    ctx->subCtx.t5t.sysInfo.DFSID = *resp;
    resp++;
  }
  if (ndefT5TSysInfoAFIPresent(ctx->subCtx.t5t.sysInfo.infoFlags) != 0U) {
    ctx->subCtx.t5t.sysInfo.AFI = *resp;
    resp++;
  }
  if (ndefT5TSysInfoMemSizePresent(ctx->subCtx.t5t.sysInfo.infoFlags) != 0U) {
    if (ctx->subCtx.t5t.legacySTHighDensity || extended) {
      /* LRIS64K/M24LR16/M24LR64 */
      ctx->subCtx.t5t.sysInfo.numberOfBlock = *resp;
      resp++;
      ctx->subCtx.t5t.sysInfo.numberOfBlock |= (((uint16_t) * resp) << 8U);
      resp++;
    } else {
      ctx->subCtx.t5t.sysInfo.numberOfBlock = *resp;
      resp++;
    }
    ctx->subCtx.t5t.sysInfo.blockSize = *resp;
    resp++;
    /* Add 1 to get real values*/
    ctx->subCtx.t5t.sysInfo.numberOfBlock++;
    ctx->subCtx.t5t.sysInfo.blockSize++;
  }
  if (ndefT5TSysInfoICRefPresent(ctx->subCtx.t5t.sysInfo.infoFlags) != 0U) {
    ctx->subCtx.t5t.sysInfo.ICRef = *resp;
    resp++;
  }
  if (extended && (ndefT5TSysInfoCmdListPresent(ctx->subCtx.t5t.sysInfo.infoFlags) != 0U)) {
    ctx->subCtx.t5t.sysInfo.supportedCmd[0U] = *resp;
    resp++;
    ctx->subCtx.t5t.sysInfo.supportedCmd[1U] = *resp;
    resp++;
    ctx->subCtx.t5t.sysInfo.supportedCmd[2U] = *resp;
    resp++;
    ctx->subCtx.t5t.sysInfo.supportedCmd[3U] = *resp;
    resp++;
  }
  return ERR_NONE;
}
#endif /* NDEF_SKIP_T5T_SYS_INFO */


/*******************************************************************************/
bool ndefT5TIsMultipleBlockReadSupported(ndefContext *ctx)
{
  ReturnCode result;
  uint16_t   rcvdLen;

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T5T)) {
    return false;
  }

  /* Autodetect the Multiple Block Read feature (CC Byte 3 b0: MBREAD) */
  result = ndefT5TPollerReadMultipleBlocks(ctx, 0U, 0U, ctx->subCtx.t5t.txrxBuf, (uint16_t)sizeof(ctx->subCtx.t5t.txrxBuf), &rcvdLen);
  return (result == ERR_NONE);
}


/*******************************************************************************/
ReturnCode ndefT5TPollerReadBytes(ndefContext *ctx, uint32_t offset, uint32_t len, uint8_t *buf, uint32_t *rcvdLen)
{
  uint8_t         lastVal;
  uint16_t        res;
  uint16_t        nbRead;
  uint16_t        blockLen;
  uint16_t        startBlock;
  uint16_t        startAddr;
  uint32_t        currentLen = len;
  uint32_t        lvRcvLen   = 0U;

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T5T) || (buf == NULL)) {
    return ERR_PARAM;
  }

  if ((ctx->subCtx.t5t.blockLen > 0U) && (len > 0U)) {
    blockLen = (uint16_t)ctx->subCtx.t5t.blockLen;
    if (blockLen == 0U) {
      return ERR_SYSTEM;
    }
    startBlock = (uint16_t)(offset / blockLen);
    startAddr  = (uint16_t)(startBlock * blockLen);

    res = ((ctx->cc.t5t.multipleBlockRead == true) && (ctx->subCtx.t5t.useMultipleBlockRead == true)) ?
          /* Read a single block using the ReadMultipleBlock command... */
          ndefT5TPollerReadMultipleBlocks(ctx, startBlock, 0U, ctx->subCtx.t5t.txrxBuf, (uint16_t)sizeof(ctx->subCtx.t5t.txrxBuf), &nbRead) :
          ndefT5TPollerReadSingleBlock(ctx, startBlock, ctx->subCtx.t5t.txrxBuf, (uint16_t)sizeof(ctx->subCtx.t5t.txrxBuf), &nbRead);
    if (res != ERR_NONE) {
      return res;
    }

    nbRead = (uint16_t)(nbRead  + startAddr - (uint16_t)offset - 1U);
    if ((uint32_t) nbRead > currentLen) {
      nbRead = (uint16_t) currentLen;
    }
    if (nbRead > 0U) {
      /* Remove the Flag byte */
      (void)ST_MEMCPY(buf, &ctx->subCtx.t5t.txrxBuf[1U - startAddr + (uint16_t)offset], nbRead);
    }
    lvRcvLen   += (uint32_t)nbRead;
    currentLen -= (uint32_t)nbRead;
    /* Process all blocks but not the last one */
    /* Rationale: ndefT5TPollerReadSingleBlock() reads 2 extra CRC bytes and could write after buffer end */
    while (currentLen > (uint32_t)blockLen) {
      startBlock++;
      lastVal = buf[lvRcvLen - 1U]; /* Read previous value that is going to be overwritten by status byte (1st byte in response) */

      res = ((ctx->cc.t5t.multipleBlockRead == true) && (ctx->subCtx.t5t.useMultipleBlockRead == true)) ?
            /* Read a single block using the ReadMultipleBlock command... */
            ndefT5TPollerReadMultipleBlocks(ctx, startBlock, 0U, &buf[lvRcvLen - 1U], blockLen + NDEF_T5T_FLAG_LEN + RFAL_CRC_LEN, &nbRead) :
            ndefT5TPollerReadSingleBlock(ctx, startBlock, &buf[lvRcvLen - 1U], blockLen + NDEF_T5T_FLAG_LEN + RFAL_CRC_LEN, &nbRead);
      if (res != ERR_NONE) {
        return res;
      }

      buf[lvRcvLen - 1U] = lastVal; /* Restore previous value */

      lvRcvLen   += blockLen;
      currentLen -= blockLen;
    }
    if (currentLen > 0U) {
      /* Process the last block. Take care of removing status byte and 2 extra CRC bytes that could write after buffer end */
      startBlock++;

      res = ((ctx->cc.t5t.multipleBlockRead == true) && (ctx->subCtx.t5t.useMultipleBlockRead == true)) ?
            /* Read a single block using the ReadMultipleBlock command... */
            ndefT5TPollerReadMultipleBlocks(ctx, startBlock, 0U, ctx->subCtx.t5t.txrxBuf, (uint16_t)sizeof(ctx->subCtx.t5t.txrxBuf), &nbRead) :
            ndefT5TPollerReadSingleBlock(ctx, startBlock, ctx->subCtx.t5t.txrxBuf, (uint16_t)sizeof(ctx->subCtx.t5t.txrxBuf), &nbRead);
      if (res != ERR_NONE) {
        return res;
      }

      nbRead--; /* Remove Flag byte */
      if (nbRead > currentLen) {
        nbRead = (uint16_t)currentLen;
      }
      if (nbRead > 0U) {
        (void)ST_MEMCPY(&buf[lvRcvLen], & ctx->subCtx.t5t.txrxBuf[1U], nbRead);
      }
      lvRcvLen   += nbRead;
      currentLen -= nbRead;
    }
  }
  if (currentLen != 0U) {
    return ERR_SYSTEM;
  }
  if (rcvdLen != NULL) {
    *rcvdLen = lvRcvLen;
  }
  return ERR_NONE;
}


/*******************************************************************************/
static ReturnCode ndefT5TPollerReadSingleBlock(ndefContext *ctx, uint16_t blockNum, uint8_t *rxBuf, uint16_t rxBufLen, uint16_t *rcvLen)
{
  ReturnCode                ret;
  uint8_t                   flags;
  const uint8_t            *uid;
  uint32_t                  retry;

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T5T) || (rxBuf == NULL) || (rcvLen == NULL)) {
    return ERR_PARAM;
  }

  RfalNfcClass *rfal_nfc = ((NdefClass *)(ctx->ndef_class_instance))->rfal_nfc;

  if (ndefT5TIsValidCache(ctx, blockNum)) {
    /* Retrieve data from cache */
    (void)ST_MEMCPY(rxBuf, ctx->subCtx.t5t.cacheBuf, NDEF_T5T_TxRx_BUFF_HEADER_SIZE + (uint32_t)ctx->subCtx.t5t.blockLen);
    *rcvLen = (uint16_t)NDEF_T5T_TxRx_BUFF_HEADER_SIZE + ctx->subCtx.t5t.blockLen;

    return ERR_NONE;
  }

  uid   = ctx->subCtx.t5t.uid;
  flags = ctx->subCtx.t5t.flags;

  retry = NDEF_T5T_N_RETRY_ERROR;
  do {
    if (ctx->subCtx.t5t.legacySTHighDensity) {
#if RFAL_FEATURE_ST25xV
      ret = rfal_nfc->rfalST25xVPollerM24LRReadSingleBlock(flags, uid, blockNum, rxBuf, rxBufLen, rcvLen);
#else
      ret = ERR_NOTSUPP;
#endif
    } else {
      if (blockNum < NDEF_T5T_MAX_BLOCK_1_BYTE_ADDR) {
        ret = rfal_nfc->rfalNfcvPollerReadSingleBlock(flags, uid, (uint8_t)blockNum, rxBuf, rxBufLen, rcvLen);
      } else {
        ret = rfal_nfc->rfalNfcvPollerExtendedReadSingleBlock(flags, uid, blockNum, rxBuf, rxBufLen, rcvLen);
      }
    }
  } while ((retry-- != 0U) && ndefT5TIsTransmissionError(ret));

  if (ret == ERR_NONE) {
    /* Update cache */
    if (*rcvLen > 0U) {
      (void)ST_MEMCPY(ctx->subCtx.t5t.cacheBuf, rxBuf, *rcvLen);
    }
    ctx->subCtx.t5t.cacheBlock = blockNum;
  }

  return ret;
}


/*******************************************************************************/
static ReturnCode ndefT5TPollerReadMultipleBlocks(ndefContext *ctx, uint16_t firstBlockNum, uint8_t numOfBlocks, uint8_t *rxBuf, uint16_t rxBufLen, uint16_t *rcvLen)
{
  ReturnCode                ret;
  uint8_t                   flags;
  const uint8_t            *uid;
  uint32_t                  retry;

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T5T)) {
    return ERR_PARAM;
  }

  RfalNfcClass *rfal_nfc = ((NdefClass *)(ctx->ndef_class_instance))->rfal_nfc;

  uid   = ctx->subCtx.t5t.uid;
  flags = ctx->subCtx.t5t.flags;

  /* 5.5 The number of data blocks returned by the Type 5 Tag in its response is (NB +1)
     e.g. NumOfBlocks = 0 means reading 1 block */

  retry = NDEF_T5T_N_RETRY_ERROR;
  do {
    if (ctx->subCtx.t5t.legacySTHighDensity) {
#if RFAL_FEATURE_ST25xV
      ret = rfal_nfc->rfalST25xVPollerM24LRReadMultipleBlocks(flags, uid, firstBlockNum, numOfBlocks, rxBuf, rxBufLen, rcvLen);
#else
      ret = ERR_NOTSUPP;
#endif
    } else {
      if (firstBlockNum < NDEF_T5T_MAX_BLOCK_1_BYTE_ADDR) {
        ret = rfal_nfc->rfalNfcvPollerReadMultipleBlocks(flags, uid, (uint8_t)firstBlockNum, numOfBlocks, rxBuf, rxBufLen, rcvLen);
      } else {
        ret = rfal_nfc->rfalNfcvPollerExtendedReadMultipleBlocks(flags, uid, firstBlockNum, numOfBlocks, rxBuf, rxBufLen, rcvLen);
      }
    }
  } while ((retry-- != 0U) && ndefT5TIsTransmissionError(ret));

  return ret;
}


#if NDEF_FEATURE_FULL_API


/*******************************************************************************/
ReturnCode ndefT5TPollerWriteBytes(ndefContext *ctx, uint32_t offset, const uint8_t *buf, uint32_t len, bool pad, bool writeTerminator)
{
  ReturnCode      res;
  uint16_t        nbRead;
  uint16_t        blockLen;
  uint16_t        startBlock;
  uint16_t        startAddr;
  const uint8_t  *wrbuf      = buf;
  uint32_t        currentLen = len;
  bool            lvWriteTerminator = writeTerminator;

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T5T) || (len == 0U) || (ctx->subCtx.t5t.blockLen == 0U)) {
    return ERR_PARAM;
  }
  blockLen = (uint16_t)ctx->subCtx.t5t.blockLen;
  if (blockLen == 0U) {
    return ERR_SYSTEM;
  }
  startBlock = (uint16_t)(offset     / blockLen);
  startAddr  = (uint16_t)(startBlock * blockLen);

  if (startAddr != offset) {
    /* Unaligned start offset must read the first block before */
    res = ndefT5TPollerReadSingleBlock(ctx, startBlock, ctx->subCtx.t5t.txrxBuf, (uint16_t)sizeof(ctx->subCtx.t5t.txrxBuf), &nbRead);
    if (res != ERR_NONE) {
      return res;
    }
    if (nbRead != (blockLen + 1U)) {
      return ERR_PROTO;
    }
    nbRead = (uint16_t)((uint32_t)nbRead - 1U  + startAddr - offset);
    if (nbRead > (uint32_t)currentLen) {
      nbRead = (uint16_t)currentLen;
    }
    if (nbRead > 0U) {
      (void)ST_MEMCPY(&ctx->subCtx.t5t.txrxBuf[offset - startAddr + 1U], wrbuf, nbRead);
    }
    if ((offset - startAddr + nbRead) < blockLen) {
      if (pad) {
        (void)ST_MEMSET(&ctx->subCtx.t5t.txrxBuf[offset - startAddr + nbRead + 1U], 0x00, blockLen - (offset - startAddr + nbRead));
      }
      if (lvWriteTerminator) {
        ctx->subCtx.t5t.txrxBuf[offset - startAddr + nbRead + 1U] = NDEF_TERMINATOR_TLV_T;
        lvWriteTerminator = false;
      }
    }
    res = ndefT5TPollerWriteSingleBlock(ctx, startBlock, &ctx->subCtx.t5t.txrxBuf[1U]);
    if (res != ERR_NONE) {
      return res;
    }
    currentLen -= nbRead;
    wrbuf       = &wrbuf[nbRead];
    startBlock++;
  }
  while (currentLen >= blockLen) {
    res = ndefT5TPollerWriteSingleBlock(ctx, startBlock, wrbuf);
    if (res != ERR_NONE) {
      return res;
    }
    currentLen -= blockLen;
    wrbuf       = &wrbuf[blockLen];
    startBlock++;
  }
  if (currentLen != 0U) {
    if (pad) {
      (void)ST_MEMSET(ctx->subCtx.t5t.txrxBuf, 0, (uint32_t)blockLen + 1U);
    } else {
      /* Unaligned end, must read the existing block before, except if padding  */
      res = ndefT5TPollerReadSingleBlock(ctx, startBlock, ctx->subCtx.t5t.txrxBuf, (uint16_t)sizeof(ctx->subCtx.t5t.txrxBuf), &nbRead);
      if (res != ERR_NONE) {
        return res;
      }
      if (nbRead != (blockLen + 1U)) {
        return ERR_PROTO;
      }
    }
    /* MISRA: PRQA requires to check the length to copy, IAR doesn't */
    if (currentLen > 0U) {
      (void)ST_MEMCPY(&ctx->subCtx.t5t.txrxBuf[1U], wrbuf, currentLen);
    }
    if (lvWriteTerminator) {
      ctx->subCtx.t5t.txrxBuf[1U + currentLen] = NDEF_TERMINATOR_TLV_T;
      lvWriteTerminator = false;
    }
    res = ndefT5TPollerWriteSingleBlock(ctx, startBlock, &ctx->subCtx.t5t.txrxBuf[1U]);
    if (res != ERR_NONE) {
      return res;
    }
  }
  if (lvWriteTerminator) {
    (void)ST_MEMSET(ctx->subCtx.t5t.txrxBuf, 0, (uint32_t)blockLen + 1U);
    ctx->subCtx.t5t.txrxBuf[1U] = NDEF_TERMINATOR_TLV_T;
    (void)ndefT5TPollerWriteSingleBlock(ctx, startBlock, &ctx->subCtx.t5t.txrxBuf[1U]);
  }
  return ERR_NONE;
}


/*******************************************************************************/
ReturnCode ndefT5TIsDevicePresent(ndefContext *ctx)
{
  ReturnCode          ret;
  uint16_t            blockAddr;
  uint16_t            rcvLen;

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T5T)) {
    return ERR_PARAM;
  }

  ndefT5TInvalidateCache(ctx);

  blockAddr = 0U;

  ret = ndefT5TPollerReadSingleBlock(ctx, blockAddr, ctx->subCtx.t5t.txrxBuf, (uint16_t)sizeof(ctx->subCtx.t5t.txrxBuf), &rcvLen);

  return ret;
}


/*******************************************************************************/
static ReturnCode ndefT5TPollerWriteSingleBlock(ndefContext *ctx, uint16_t blockNum, const uint8_t *wrData)
{
  ReturnCode                ret;
  uint8_t                   flags;
  const uint8_t            *uid;
  uint32_t                  retry;

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T5T)) {
    return ERR_PARAM;
  }

  RfalNfcClass *rfal_nfc = ((NdefClass *)(ctx->ndef_class_instance))->rfal_nfc;

  uid   = ctx->subCtx.t5t.uid;
  flags = ctx->subCtx.t5t.flags;
  if (ctx->cc.t5t.specialFrame) {
    flags |= (uint8_t)RFAL_NFCV_REQ_FLAG_OPTION;
  }

  ndefT5TInvalidateCache(ctx);

  retry = NDEF_T5T_N_RETRY_ERROR;
  do {
    if (ctx->subCtx.t5t.legacySTHighDensity) {
#if RFAL_FEATURE_ST25xV
      ret = rfal_nfc->rfalST25xVPollerM24LRWriteSingleBlock(flags, uid, blockNum, wrData, ctx->subCtx.t5t.blockLen);
#else
      ret = ERR_NOTSUPP;
#endif
    } else {
      if (blockNum < NDEF_T5T_MAX_BLOCK_1_BYTE_ADDR) {
        ret = rfal_nfc->rfalNfcvPollerWriteSingleBlock(flags, uid, (uint8_t)blockNum, wrData, ctx->subCtx.t5t.blockLen);
      } else {
        ret = rfal_nfc->rfalNfcvPollerExtendedWriteSingleBlock(flags, uid, blockNum, wrData, ctx->subCtx.t5t.blockLen);
      }
    }
  } while ((retry-- != 0U) && ndefT5TIsTransmissionError(ret));

  return ret;
}


/*******************************************************************************/
static ReturnCode ndefT5TPollerLockSingleBlock(ndefContext *ctx, uint16_t blockNum)
{
  ReturnCode                ret;
  uint8_t                   flags;
  const uint8_t            *uid;
  uint32_t                  retry;

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T5T)) {
    return ERR_PARAM;
  }

  RfalNfcClass *rfal_nfc = ((NdefClass *)(ctx->ndef_class_instance))->rfal_nfc;

  uid   = ctx->subCtx.t5t.uid;
  flags = ctx->subCtx.t5t.flags;
  if (ctx->cc.t5t.specialFrame) {
    flags |= (uint8_t)RFAL_NFCV_REQ_FLAG_OPTION;
  }

  retry = NDEF_T5T_N_RETRY_ERROR;
  do {
    if (blockNum < NDEF_T5T_MAX_BLOCK_1_BYTE_ADDR) {
      ret = rfal_nfc->rfalNfcvPollerLockBlock(flags, uid, (uint8_t)blockNum);
    } else {
      ret = rfal_nfc->rfalNfcvPollerExtendedLockSingleBlock(flags, uid, blockNum);
    }
  } while ((retry-- != 0U) && ndefT5TIsTransmissionError(ret));

  return ret;
}


/*******************************************************************************/
ReturnCode ndefT5TLockDevice(ndefContext *ctx)
{
  ReturnCode ret;
  uint32_t   numBlocks;
  uint16_t   i;

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T5T)) {
    return ERR_PARAM;
  }

  ctx->state = NDEF_STATE_READONLY;
  numBlocks = (ctx->areaLen + (uint32_t)ctx->cc.t5t.ccLen) / (uint32_t)ctx->subCtx.t5t.blockLen;
  if (ctx->cc.t5t.lockBlock && !ctx->subCtx.t5t.legacySTHighDensity) {
    for (i = 0; i < numBlocks; i++) {
      ret = ndefT5TPollerLockSingleBlock(ctx, i);
      if (ret != ERR_NONE) {
        return ret;
      }
    }
  }

  return ERR_NONE;
}

#endif /* NDEF_FEATURE_FULL_API */

#endif /* NDEF_FEATURE_T5T */
