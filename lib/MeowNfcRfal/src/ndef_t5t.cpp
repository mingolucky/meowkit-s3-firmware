
/**
  ******************************************************************************
  * @file           : ndef_t5t.cpp
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
#include "ndef_poller.h"
#include "ndef_t5t_hal.h"
#include "ndef_t5t.h"
#include "nfc_utils.h"

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

#define NDEF_T5T_MLEN_DIVIDER                  8U    /*!<  T5T_area size is measured in bytes is equal to 8 * MLEN */

#define NDEF_T5T_TLV_T_LEN                     1U    /*!< TLV T Length: 1 bytes                             */
#define NDEF_T5T_TLV_L_1_BYTES_LEN             1U    /*!< TLV L Length: 1 bytes                             */
#define NDEF_T5T_TLV_L_3_BYTES_LEN             3U    /*!< TLV L Length: 3 bytes                             */

#define NDEF_T5T_MAX_BLOCK_1_BYTE_ADDR       256U    /*!< Max number of blocks for 1 byte addressing        */
#define NDEF_T5T_MAX_MLEN_1_BYTE_ENCODING    256U    /*!< MLEN max value for 1 byte encoding                */

#define NDEF_T5T_TL_MIN_SIZE  (NDEF_T5T_TLV_T_LEN \
                       + NDEF_T5T_TLV_L_1_BYTES_LEN) /*!< Min TL size                                       */

#define NDEF_T5T_TL_MAX_SIZE  (NDEF_T5T_TLV_T_LEN \
                       + NDEF_T5T_TLV_L_3_BYTES_LEN) /*!< Max TL size                                       */

#define NDEF_T5T_TLV_NDEF                   0x03U    /*!< TLV flag NDEF value                               */
#define NDEF_T5T_TLV_PROPRIETARY            0xFDU    /*!< TLV flag PROPRIETARY value                        */
#define NDEF_T5T_TLV_TERMINATOR             0xFEU    /*!< TLV flag TERMINATOR value                         */
#define NDEF_T5T_TLV_RFU                    0x00U    /*!< TLV flag RFU value                                */

#define NDEF_T5T_ACCESS_ALWAYS               0x0U    /*!< Read/Write Access. 00b: Always                    */
#define NDEF_T5T_ACCESS_RFU                  0x1U    /*!< Read/Write Access. 01b: RFU                       */
#define NDEF_T5T_ACCESS_PROPRIETARY          0x2U    /*!< Read/Write Access. 00b: Proprietary               */
#define NDEF_T5T_ACCESS_NEVER                0x3U    /*!< Read/Write Access. 00b: Never                     */


/*
 *****************************************************************************
 * GLOBAL TYPES
 ******************************************************************************
 */


/*
 ******************************************************************************
 * GLOBAL MACROS
 ******************************************************************************
 */


/*
 ******************************************************************************
 * LOCAL VARIABLES
 ******************************************************************************
 */
/*! Default T5T Access mode */
static ndefT5TAccessMode gAccessMode = NDEF_T5T_ACCESS_MODE_SELECTED;

/*
 ******************************************************************************
 * LOCAL FUNCTION PROTOTYPES
 ******************************************************************************
 */


#if NDEF_FEATURE_FULL_API
  static ReturnCode ndefT5TWriteCC(ndefContext *ctx);
#endif /* NDEF_FEATURE_FULL_API */

/*
 ******************************************************************************
 * GLOBAL FUNCTIONS
 ******************************************************************************
 */

/*******************************************************************************/
ReturnCode ndefT5TPollerSetAccessMode(ndefT5TAccessMode mode)
{
  gAccessMode = mode;

  return ERR_NONE;
}

#ifdef TEST_NDEF
/*******************************************************************************/
ReturnCode ndefT5TPollerMultipleBlockRead(ndefContext *ctx, bool enable)
{
  if ((ctx == NULL) || (ctx->state != NDEF_STATE_INVALID)) {
    return ERR_PARAM;
  }

  ctx->subCtx.t5t.useMultipleBlockRead = enable;

  return ERR_NONE;
}
#endif /* TEST_NDEF */

/*******************************************************************************/
static ReturnCode ndefT5TReadLField(ndefContext *ctx)
{
  ReturnCode           ret;
  uint32_t             offset;
  uint8_t              data[3];
  uint16_t             lenTLV;

  ctx->state = NDEF_STATE_INVALID;
  offset = ctx->subCtx.t5t.TlvNDEFOffset;
  offset++;
  ret = ndefT5TPollerReadBytes(ctx, offset, 1, data, NULL);
  if (ret != ERR_NONE) {
    /* Conclude procedure */
    return ret;
  }
  offset++;
  lenTLV = data[0];
  if (lenTLV == (NDEF_SHORT_VFIELD_MAX_LEN + 1U)) {
    ret = ndefT5TPollerReadBytes(ctx, offset, 2, data, NULL);
    if (ret != ERR_NONE) {
      /* Conclude procedure */
      return ret;
    }
    offset += 2U;
    lenTLV = GETU16(&data[0]);
  }
  ctx->messageLen    = lenTLV;
  ctx->messageOffset = offset;

  if (lenTLV == 0U) {
    if (!((ctx->cc.t5t.readAccess  == NDEF_T5T_ACCESS_ALWAYS) && (ctx->cc.t5t.writeAccess == NDEF_T5T_ACCESS_ALWAYS))) {
      /* Conclude procedure  */
      return ERR_REQUEST;
    }
    ctx->state = NDEF_STATE_INITIALIZED;
  } else {
    if (!(ctx->cc.t5t.readAccess == NDEF_T5T_ACCESS_ALWAYS)) {
      /* Conclude procedure  */
      return ERR_REQUEST;
    }
    ctx->state = (ctx->cc.t5t.writeAccess == NDEF_T5T_ACCESS_ALWAYS) ? NDEF_STATE_READWRITE : NDEF_STATE_READONLY;
  }

  return ERR_NONE;
}

/*******************************************************************************/
ReturnCode ndefT5TPollerContextInitialization(ndefContext *ctx, const ndefDevice *dev)
{
#if !defined NDEF_SKIP_T5T_SYS_INFO
  ReturnCode    result;
#endif

  if ((ctx == NULL) || (dev == NULL)) {
    return ERR_PARAM;
  }
  if (!ndefT5TisT5TDevice(dev)) {
    return ERR_PARAM;
  }

  (void)ST_MEMCPY(&ctx->device, dev, sizeof(ctx->device));

  ndefT5TInvalidateCache(ctx);

  /* Reset info about the card */
  ctx->type                     = NDEF_DEV_T5T;
  ctx->state                    = NDEF_STATE_INVALID;

  /* Initialize CC fields, used in NDEF detect */
  ctx->cc.t5t.ccLen             = 0U;
  ctx->cc.t5t.magicNumber       = 0U;
  ctx->cc.t5t.majorVersion      = 0U;
  ctx->cc.t5t.minorVersion      = 0U;
  ctx->cc.t5t.readAccess        = 0U;
  ctx->cc.t5t.writeAccess       = 0U;
  ctx->cc.t5t.memoryLen         = 0U;
  ctx->cc.t5t.specialFrame      = false;
  ctx->cc.t5t.lockBlock         = false;
  ctx->cc.t5t.mlenOverflow      = false;
  ctx->cc.t5t.multipleBlockRead = false;

  ctx->subCtx.t5t.blockLen      = 0U;
  ctx->subCtx.t5t.TlvNDEFOffset = 0U; /* Offset for TLV */
  ctx->subCtx.t5t.useMultipleBlockRead = false;

  ndefT5TPollerAccessMode(ctx, dev, gAccessMode);

  ctx->subCtx.t5t.stDevice = ndefT5TisSTDevice(dev);

  /* Get block length, and set subCtx.t5t.legacySTHighDensity */
  ctx->subCtx.t5t.blockLen = ndefT5TGetBlockLength(ctx);
  if (ctx->subCtx.t5t.blockLen == 0U) {
    return ERR_PROTO;
  }

  ctx->subCtx.t5t.sysInfoSupported = false;

#if !defined NDEF_SKIP_T5T_SYS_INFO
  result = ndefT5TGetMemoryConfig(ctx);
  if (result != ERR_NONE) {
    return result;
  }
#endif
  return ERR_NONE;
}

/*******************************************************************************/
ReturnCode ndefT5TPollerNdefDetect(ndefContext *ctx, ndefInfo *info)
{
  ReturnCode result;
  uint8_t    tmpBuf[NDEF_T5T_TL_MAX_SIZE];
  ReturnCode returnCode = ERR_REQUEST; /* Default return code */
  uint16_t   offset;
  uint16_t   length;
  uint32_t   TlvOffset;
  bool       exit;
  uint32_t   rcvLen;

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T5T)) {
    return ERR_PARAM;
  }

  ctx->state                           = NDEF_STATE_INVALID;
  ctx->cc.t5t.ccLen                    = 0U;
  ctx->cc.t5t.memoryLen                = 0U;
  ctx->cc.t5t.multipleBlockRead        = false;
  ctx->messageLen                      = 0U;
  ctx->messageOffset                   = 0U;
  ctx->areaLen                         = 0U;

  if (info != NULL) {
    info->state                = NDEF_STATE_INVALID;
    info->majorVersion         = 0U;
    info->minorVersion         = 0U;
    info->areaLen              = 0U;
    info->areaAvalableSpaceLen = 0U;
    info->messageLen           = 0U;
  }

  result = ndefT5TPollerReadBytes(ctx, 0U, 4U, ctx->ccBuf, &rcvLen);
  if ((result == ERR_NONE) && (rcvLen == 4U) && ((ctx->ccBuf[0] == (uint8_t)0xE1U) || (ctx->ccBuf[0] == (uint8_t)0xE2U))) {
    ctx->cc.t5t.magicNumber           =  ctx->ccBuf[0U];
    ctx->cc.t5t.majorVersion          = (ctx->ccBuf[1U] >> 6U) & 0x03U;
    ctx->cc.t5t.minorVersion          = (ctx->ccBuf[1U] >> 4U) & 0x03U;
    ctx->cc.t5t.readAccess            = (ctx->ccBuf[1U] >> 2U) & 0x03U;
    ctx->cc.t5t.writeAccess           = (ctx->ccBuf[1U] >> 0U) & 0x03U;
    ctx->cc.t5t.memoryLen             =  ctx->ccBuf[2U];
    ctx->cc.t5t.specialFrame          = (((ctx->ccBuf[3U] >> 4U) & 0x01U) != 0U);
    ctx->cc.t5t.lockBlock             = (((ctx->ccBuf[3U] >> 3U) & 0x01U) != 0U);
    ctx->cc.t5t.mlenOverflow          = (((ctx->ccBuf[3U] >> 2U) & 0x01U) != 0U);
    /* Read the CC with Single Block Read command(s) and update multipleBlockRead flag after */
    ctx->state                        = NDEF_STATE_INITIALIZED;

    /* Check Magic Number TS T5T v1.0 - 7.5.1.2 */
    if ((ctx->cc.t5t.magicNumber != NDEF_T5T_CC_MAGIC_1_BYTE_ADDR_MODE) &&
        (ctx->cc.t5t.magicNumber != NDEF_T5T_CC_MAGIC_2_BYTE_ADDR_MODE)) {
      return ERR_REQUEST;
    }

    /* Check version - 7.5.1.2 */
    if (ctx->cc.t5t.majorVersion > ndefT5TMajorVersion(NDEF_T5T_MAPPING_VERSION_1_0)) {
      return ERR_REQUEST;
    }

    /* Check read access - 7.5.1.2 */
    if (ctx->cc.t5t.readAccess != NDEF_T5T_ACCESS_ALWAYS) {
      return ERR_REQUEST;
    }

    if (ctx->cc.t5t.memoryLen != 0U) {
      /* 4-byte CC */
      ctx->cc.t5t.ccLen         = NDEF_T5T_CC_LEN_4_BYTES;
      if ((ctx->cc.t5t.memoryLen == 0xFFU) && ctx->cc.t5t.mlenOverflow) {
        if ((ctx->subCtx.t5t.sysInfoSupported == true) && (ndefT5TSysInfoMemSizePresent(ctx->subCtx.t5t.sysInfo.infoFlags) != 0U)) {
          ctx->cc.t5t.memoryLen = (uint16_t)((ctx->subCtx.t5t.sysInfo.numberOfBlock * ctx->subCtx.t5t.sysInfo.blockSize) / NDEF_T5T_MLEN_DIVIDER);
        }
      }
    } else {
      /* 8-byte CC */
      result = ndefT5TPollerReadBytes(ctx, 4U, 4U, &ctx->ccBuf[4U], &rcvLen);
      if ((result == ERR_NONE) && (rcvLen == 4U)) {
        ctx->cc.t5t.ccLen     = NDEF_T5T_CC_LEN_8_BYTES;
        ctx->cc.t5t.memoryLen = ((uint16_t)ctx->ccBuf[6U] << 8U) + (uint16_t)ctx->ccBuf[7U];
      }
    }

    /* Update multipleBlockRead flag after having read the second half of 8-byte CC */
    ctx->cc.t5t.multipleBlockRead     = (((ctx->ccBuf[3U] >> 0U) & 0x01U) != 0U);

    if ((ctx->subCtx.t5t.sysInfoSupported == true) &&
        (ndefT5TSysInfoMemSizePresent(ctx->subCtx.t5t.sysInfo.infoFlags) != 0U) &&
        (ctx->cc.t5t.memoryLen == (uint16_t)((ctx->subCtx.t5t.sysInfo.numberOfBlock * ctx->subCtx.t5t.sysInfo.blockSize) / NDEF_T5T_MLEN_DIVIDER)) &&
        (ctx->cc.t5t.memoryLen > 0U)) {
      ctx->cc.t5t.memoryLen--; /* remove CC area from memory length */
    }

    ctx->messageLen     = 0U;
    ctx->messageOffset  = ctx->cc.t5t.ccLen;
    /* TS T5T v1.0 4.3.1.17 T5T_Area size is measured in bytes, is equal to MLEN * 8 */
    ctx->areaLen        = (uint32_t)ctx->cc.t5t.memoryLen * NDEF_T5T_MLEN_DIVIDER;

    TlvOffset = ctx->cc.t5t.ccLen;
    exit      = false;
    while ((exit == false) && (TlvOffset < (ctx->cc.t5t.ccLen + ctx->areaLen))) {
      result = ndefT5TPollerReadBytes(ctx, TlvOffset, NDEF_T5T_TL_MIN_SIZE, tmpBuf, &rcvLen);
      if ((result != ERR_NONE) || (rcvLen != NDEF_T5T_TL_MIN_SIZE)) {
        return result;
      }
      offset = NDEF_T5T_TLV_T_LEN + NDEF_T5T_TLV_L_1_BYTES_LEN;
      length = tmpBuf[1U];
      if (length == (NDEF_SHORT_VFIELD_MAX_LEN + 1U)) {
        /* Size is encoded in 1 + 2 bytes */
        result = ndefT5TPollerReadBytes(ctx, TlvOffset, NDEF_T5T_TL_MAX_SIZE, tmpBuf, &rcvLen);
        if ((result != ERR_NONE) || (rcvLen != NDEF_T5T_TL_MAX_SIZE)) {
          return result;
        }
        length = (((uint16_t)tmpBuf[2U]) << 8U) + (uint16_t)tmpBuf[3U];
        offset += 2U;
      }
      if (tmpBuf[0U] == (uint8_t)NDEF_T5T_TLV_NDEF) {
        /* NDEF record return it */
        returnCode                    = ERR_NONE;  /* Default */
        ctx->subCtx.t5t.TlvNDEFOffset = TlvOffset; /* Offset for TLV */
        ctx->messageOffset            = TlvOffset + offset;
        ctx->messageLen               = length;
        if (length == 0U) {
          /* Req 40 7.5.1.6 */
          if ((ctx->cc.t5t.readAccess  == NDEF_T5T_ACCESS_ALWAYS) &&
              (ctx->cc.t5t.writeAccess == NDEF_T5T_ACCESS_ALWAYS)) {
            ctx->state = NDEF_STATE_INITIALIZED;
          } else {
            ctx->state = NDEF_STATE_INVALID;
            returnCode = ERR_REQUEST; /* Default */
          }
          exit = true;
        } else {
          if (ctx->cc.t5t.readAccess == NDEF_T5T_ACCESS_ALWAYS) {
            if (ctx->cc.t5t.writeAccess == NDEF_T5T_ACCESS_ALWAYS) {
              ctx->state = NDEF_STATE_READWRITE;
            } else {
              ctx->state = NDEF_STATE_READONLY;
            }
          }
          exit = true;
        }
      } else if (tmpBuf[0U] == (uint8_t) NDEF_T5T_TLV_TERMINATOR) {
        /* NDEF end */
        exit = true;
      } else {
        /* Skip Proprietary and RFU too */
        TlvOffset += (uint32_t)offset + (uint32_t)length;
      }
    }
  } else {
    /* No CC File */
    returnCode = ERR_REQUEST;
    if (result != ERR_NONE) {
      returnCode = result;
    }
  }

  if (info != NULL) {
    info->state                = ctx->state;
    info->majorVersion         = ctx->cc.t5t.majorVersion;
    info->minorVersion         = ctx->cc.t5t.minorVersion;
    info->areaLen              = ctx->areaLen;
    info->areaAvalableSpaceLen = (uint32_t)ctx->cc.t5t.ccLen + ctx->areaLen - ctx->messageOffset;
    info->messageLen           = ctx->messageLen;
  }
  return returnCode;
}

/*******************************************************************************/
ReturnCode ndefT5TPollerReadRawMessage(ndefContext *ctx, uint8_t *buf, uint32_t bufLen, uint32_t *rcvdLen, bool single)
{
  ReturnCode result;

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T5T) || (buf == NULL)) {
    return ERR_PARAM;
  }

  if (!single) {
    ndefT5TInvalidateCache(ctx);
    result = ndefT5TReadLField(ctx);
    if (result != ERR_NONE) {
      /* Conclude procedure */
      return result;
    }
  }

  if (ctx->state <= NDEF_STATE_INITIALIZED) {
    /* Conclude procedure  */
    return ERR_WRONG_STATE;
  }

  if (ctx->messageLen > bufLen) {
    return ERR_NOMEM;
  }

  result = ndefT5TPollerReadBytes(ctx, ctx->messageOffset, ctx->messageLen, buf, rcvdLen);
  if (result != ERR_NONE) {
    ctx->state = NDEF_STATE_INVALID;
  }
  return result;
}

#if NDEF_FEATURE_FULL_API


/*******************************************************************************/
ReturnCode ndefT5TPollerWriteRawMessageLen(ndefContext *ctx, uint32_t rawMessageLen, bool writeTerminator)
{
  uint8_t    TLV[8U];
  ReturnCode result;
  uint8_t    len;

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T5T)) {
    return ERR_PARAM;
  }

  if ((ctx->state != NDEF_STATE_INITIALIZED) && (ctx->state != NDEF_STATE_READWRITE)) {
    return ERR_WRONG_STATE;
  }

  if (writeTerminator && (rawMessageLen != 0U) && ((ctx->messageOffset + rawMessageLen) < ctx->areaLen)) {
    /* Write T5T TLV terminator */
    len = 0U;
    TLV[len] = NDEF_TERMINATOR_TLV_T; /* TLV terminator */
    len++;
    result = ndefT5TPollerWriteBytes(ctx, ctx->messageOffset + rawMessageLen, TLV, len, true, false);
    if (result != ERR_NONE) {
      return result;
    }
  }

  len = 0U;
  TLV[len] = NDEF_T5T_TLV_NDEF;
  len++;
  if (rawMessageLen <= NDEF_SHORT_VFIELD_MAX_LEN) {
    TLV[len] = (uint8_t) rawMessageLen;
    len++;
  } else {
    TLV[len] = (uint8_t)(NDEF_SHORT_VFIELD_MAX_LEN + 1U);
    len++;
    TLV[len] = (uint8_t)(rawMessageLen >> 8U);
    len++;
    TLV[len] = (uint8_t) rawMessageLen;
    len++;
  }
  if (writeTerminator && (rawMessageLen == 0U)) {
    TLV[len] = NDEF_TERMINATOR_TLV_T; /* TLV terminator */
    len++;
  }

  result = ndefT5TPollerWriteBytes(ctx, ctx->subCtx.t5t.TlvNDEFOffset, TLV, len, writeTerminator && (rawMessageLen == 0U), false);

  return result;
}

/*******************************************************************************/
ReturnCode ndefT5TPollerWriteRawMessage(ndefContext *ctx, const uint8_t *buf, uint32_t bufLen)
{
  uint32_t   len = bufLen;
  ReturnCode result;

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T5T) || (buf == NULL)) {
    return ERR_PARAM;
  }

  /* TS T5T v1.0 7.5.3.1/2: T5T NDEF Detect should have been called before NDEF write procedure */
  /* Warning: current tag content must not be changed between NDEF Detect procedure and NDEF Write procedure*/

  /* TS T5T v1.0 7.5.3.3: check write access condition */
  if ((ctx->state != NDEF_STATE_INITIALIZED) && (ctx->state != NDEF_STATE_READWRITE)) {
    /* Conclude procedure */
    return ERR_WRONG_STATE;
  }

  /* TS T5T v1.0 7.5.3.3: verify available space */
  result = ndefT5TPollerCheckAvailableSpace(ctx, bufLen);
  if (result != ERR_NONE) {
    /* Conclude procedure */
    return ERR_PARAM;
  }
  /* TS T5T v1.0 7.5.3.4: reset L-Field to 0 */
  /* and update ctx->messageOffset according to L-field len */
  result = ndefT5TPollerBeginWriteMessage(ctx, bufLen);
  if (result != ERR_NONE) {
    ctx->state = NDEF_STATE_INVALID;
    /* Conclude procedure */
    return result;
  }
  if (bufLen != 0U) {
    /* TS T5T v1.0 7.5.3.5: write new NDEF message and write terminator TLV is enough space for it*/
    result = ndefT5TPollerWriteBytes(ctx, ctx->messageOffset, buf, len, true, ndefT5TPollerCheckAvailableSpace(ctx, bufLen + 1U) == ERR_NONE);
    if (result != ERR_NONE) {
      /* Conclude procedure */
      ctx->state = NDEF_STATE_INVALID;
      return result;
    }
    /* TS T5T v1.0 7.5.3.6 & 7.5.3.7: update L-Field and write Terminator TLV */
    result = ndefT5TPollerEndWriteMessage(ctx, len, false);
    if (result != ERR_NONE) {
      /* Conclude procedure */
      ctx->state = NDEF_STATE_INVALID;
      return result;
    }
  }
  return result;
}

/*******************************************************************************/
static ReturnCode ndefT5TWriteCC(ndefContext *ctx)
{
  ReturnCode  ret;
  uint8_t    *buf;
  uint8_t     dataIt;

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T5T)) {
    return ERR_PARAM;
  }

  buf    = ctx->ccBuf;
  dataIt = 0U;
  /* Encode CC */
  buf[dataIt] = ctx->cc.t5t.magicNumber;                                                                /* Byte 0 */
  dataIt++;
  buf[dataIt] = (uint8_t)(((ctx->cc.t5t.majorVersion  & 0x03U) << 6) |                                  /* Byte 1 */
                          ((ctx->cc.t5t.minorVersion  & 0x03U) << 4) |                                  /*        */
                          ((ctx->cc.t5t.readAccess    & 0x03U) << 2) |                                  /*        */
                          ((ctx->cc.t5t.writeAccess   & 0x03U) << 0));                                  /*        */
  dataIt++;
  buf[dataIt] = (ctx->cc.t5t.ccLen == NDEF_T5T_CC_LEN_8_BYTES) ? 0U : (uint8_t)ctx->cc.t5t.memoryLen;   /* Byte 2 */
  dataIt++;
  buf[dataIt] = 0U;                                                                                     /* Byte 3 */
  if (ctx->cc.t5t.multipleBlockRead) {
    buf[dataIt] |= 0x01U;  /* Byte 3  b0 MBREAD                */
  }
  if (ctx->cc.t5t.mlenOverflow)      {
    buf[dataIt] |= 0x04U;  /* Byte 3  b2 Android MLEN overflow */
  }
  if (ctx->cc.t5t.lockBlock)         {
    buf[dataIt] |= 0x08U;  /* Byte 3  b3 Lock Block            */
  }
  if (ctx->cc.t5t.specialFrame)      {
    buf[dataIt] |= 0x10U;  /* Byte 3  b4 Special Frame         */
  }
  dataIt++;
  if (ctx->cc.t5t.ccLen == NDEF_T5T_CC_LEN_8_BYTES) {
    buf[dataIt] = 0U;                                                                                 /* Byte 4 */
    dataIt++;
    buf[dataIt] = 0U;                                                                                 /* Byte 5 */
    dataIt++;
    buf[dataIt] = (uint8_t)(ctx->cc.t5t.memoryLen >> 8);                                              /* Byte 6 */
    dataIt++;
    buf[dataIt] = (uint8_t)(ctx->cc.t5t.memoryLen >> 0);                                              /* Byte 7 */
    dataIt++;
  }

  ret = ndefT5TPollerWriteBytes(ctx, 0U, buf, ctx->cc.t5t.ccLen, false, false);
  return ret;
}

/*******************************************************************************/
ReturnCode ndefT5TPollerTagFormat(ndefContext *ctx, const ndefCapabilityContainer *cc, uint32_t options)
{
  ReturnCode               result;
  static const uint8_t     emptyNDEF[] = { 0x03U, 0x00U };

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T5T)) {
    return ERR_PARAM;
  }

  /* Reset previous potential info about NDEF messages */
  ctx->messageLen               = 0U;
  ctx->messageOffset            = 0U;
  ctx->subCtx.t5t.TlvNDEFOffset = 0U;

  if (cc != NULL) {
    if ((cc->t5t.ccLen != NDEF_T5T_CC_LEN_8_BYTES) && (cc->t5t.ccLen != NDEF_T5T_CC_LEN_4_BYTES)) {
      return ERR_PARAM;
    }
    (void)ST_MEMCPY(&ctx->cc, cc, sizeof(ndefCapabilityContainer));
  } else {
    /* Try to find the appropriate cc values */
    ctx->cc.t5t.magicNumber  = NDEF_T5T_CC_MAGIC_1_BYTE_ADDR_MODE; /* E1 */
    ctx->cc.t5t.majorVersion = ndefT5TMajorVersion(NDEF_T5T_MAPPING_VERSION_1_0);
    ctx->cc.t5t.minorVersion = ndefT5TMinorVersion(NDEF_T5T_MAPPING_VERSION_1_0);
    ctx->cc.t5t.readAccess   = NDEF_T5T_ACCESS_ALWAYS;
    ctx->cc.t5t.writeAccess  = NDEF_T5T_ACCESS_ALWAYS;

    ctx->cc.t5t.specialFrame = false;
    ctx->cc.t5t.lockBlock    = false;
    ctx->cc.t5t.memoryLen    = 0U;
    ctx->cc.t5t.mlenOverflow = false;

    ctx->cc.t5t.multipleBlockRead = ndefT5TIsMultipleBlockReadSupported(ctx);

    /* Try to retrieve the tag's size using getSystemInfo and GetExtSystemInfo */
    if ((ctx->subCtx.t5t.sysInfoSupported == false) || (ndefT5TSysInfoMemSizePresent(ctx->subCtx.t5t.sysInfo.infoFlags) == 0U)) {
      return ERR_REQUEST;
    }
    ctx->cc.t5t.memoryLen = (uint16_t)((ctx->subCtx.t5t.sysInfo.numberOfBlock * ctx->subCtx.t5t.sysInfo.blockSize) / NDEF_T5T_MLEN_DIVIDER);

    if ((options & NDEF_T5T_FORMAT_OPTION_NFC_FORUM) == NDEF_T5T_FORMAT_OPTION_NFC_FORUM) { /* NFC Forum format */
      if (ctx->cc.t5t.memoryLen >= NDEF_T5T_MAX_MLEN_1_BYTE_ENCODING) {
        ctx->cc.t5t.ccLen = NDEF_T5T_CC_LEN_8_BYTES;
      }
      if (ctx->cc.t5t.memoryLen > 0U) {
        ctx->cc.t5t.memoryLen--; /* remove CC area from memory length */
      }
    } else { /* Android format */
      ctx->cc.t5t.ccLen = NDEF_T5T_CC_LEN_4_BYTES;
      if (ctx->cc.t5t.memoryLen >= NDEF_T5T_MAX_MLEN_1_BYTE_ENCODING) {
        ctx->cc.t5t.mlenOverflow = true;
        ctx->cc.t5t.memoryLen    = 0xFFU;
      }
    }

    if (!ctx->subCtx.t5t.legacySTHighDensity && (ctx->subCtx.t5t.sysInfo.numberOfBlock > NDEF_T5T_MAX_BLOCK_1_BYTE_ADDR)) {
      ctx->cc.t5t.magicNumber = NDEF_T5T_CC_MAGIC_2_BYTE_ADDR_MODE; /* E2 */
    }
  }

  result = ndefT5TWriteCC(ctx);
  if (result != ERR_NONE) {
    /* If write fails, try to use special frame if not yet used */
    if (!ctx->cc.t5t.specialFrame) {
      delay(20U); /* Wait to be sure that previous command has ended */
      ctx->cc.t5t.specialFrame = true; /* Add option flag */
      result = ndefT5TWriteCC(ctx);
      if (result != ERR_NONE) {
        ctx->cc.t5t.specialFrame = false; /* Add option flag */
        return result;
      }
    } else {
      return result;
    }
  }

  /* Update info about current NDEF */

  ctx->subCtx.t5t.TlvNDEFOffset = ctx->cc.t5t.ccLen;

  result = ndefT5TPollerWriteBytes(ctx, ctx->subCtx.t5t.TlvNDEFOffset, emptyNDEF, sizeof(emptyNDEF), true, true);
  if (result == ERR_NONE) {
    /* Update info about current NDEF */
    ctx->messageOffset = (uint32_t)ctx->cc.t5t.ccLen + NDEF_T5T_TLV_T_LEN + NDEF_T5T_TLV_L_1_BYTES_LEN;
    ctx->state         = NDEF_STATE_INITIALIZED;
  }
  return result;
}

/*******************************************************************************/
ReturnCode ndefT5TPollerCheckPresence(ndefContext *ctx)
{
  ReturnCode ret;

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T5T)) {
    return ERR_PARAM;
  }

  ret = ndefT5TIsDevicePresent(ctx);

  return ret;
}


/*******************************************************************************/
ReturnCode ndefT5TPollerCheckAvailableSpace(const ndefContext *ctx, uint32_t messageLen)
{
  uint32_t lLen;

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T5T)) {
    return ERR_PARAM;
  }

  if (ctx->state == NDEF_STATE_INVALID) {
    return ERR_WRONG_STATE;
  }

  lLen = (messageLen > NDEF_SHORT_VFIELD_MAX_LEN) ? NDEF_T5T_TLV_L_3_BYTES_LEN : NDEF_T5T_TLV_L_1_BYTES_LEN;

  if ((messageLen + ctx->subCtx.t5t.TlvNDEFOffset + NDEF_T5T_TLV_T_LEN + lLen) > (ctx->areaLen + ctx->cc.t5t.ccLen)) {
    return ERR_NOMEM;
  }
  return ERR_NONE;
}

/*******************************************************************************/
ReturnCode ndefT5TPollerBeginWriteMessage(ndefContext *ctx, uint32_t messageLen)
{
  ReturnCode ret;
  uint32_t   lLen;

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T5T)) {
    return ERR_PARAM;
  }

  if ((ctx->state != NDEF_STATE_INITIALIZED) && (ctx->state != NDEF_STATE_READWRITE)) {
    return ERR_WRONG_STATE;
  }

  /* TS T5T v1.0 7.5.3.4: reset L-Field to 0 */
  ret = ndefT5TPollerWriteRawMessageLen(ctx, 0U, true);
  if (ret != ERR_NONE) {
    /* Conclude procedure */
    ctx->state = NDEF_STATE_INVALID;
    return ret;
  }

  lLen                = (messageLen > NDEF_SHORT_VFIELD_MAX_LEN) ? NDEF_T5T_TLV_L_3_BYTES_LEN : NDEF_T5T_TLV_L_1_BYTES_LEN;
  ctx->messageOffset  = ctx->subCtx.t5t.TlvNDEFOffset;
  ctx->messageOffset += NDEF_T5T_TLV_T_LEN; /* T Length */
  ctx->messageOffset += lLen;               /* L Length */
  ctx->state          = NDEF_STATE_INITIALIZED;

  return ERR_NONE;
}

/*******************************************************************************/
ReturnCode ndefT5TPollerEndWriteMessage(ndefContext *ctx, uint32_t messageLen, bool writeTerminator)
{
  ReturnCode ret;

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T5T)) {
    return ERR_PARAM;
  }

  if (ctx->state != NDEF_STATE_INITIALIZED) {
    return ERR_WRONG_STATE;
  }

  /* TS T5T v1.0 7.5.3.6 & 7.5.3.7: update L-Field and write Terminator TLV */
  ret = ndefT5TPollerWriteRawMessageLen(ctx, messageLen, writeTerminator);
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
ReturnCode ndefT5TPollerSetReadOnly(ndefContext *ctx)
{
  ReturnCode ret;

  if ((ctx == NULL) || (ctx->type != NDEF_DEV_T5T)) {
    return ERR_PARAM;
  }

  if (ctx->state != NDEF_STATE_READWRITE) {
    return ERR_WRONG_STATE;
  }

  /* Change write access */
  ctx->cc.t5t.writeAccess = NDEF_T5T_ACCESS_NEVER;

  ret = ndefT5TWriteCC(ctx);
  if (ret != ERR_NONE) {
    return ret;
  }

  ret = ndefT5TLockDevice(ctx);
  if (ret != ERR_NONE) {
    return ret;
  }
  return ERR_NONE;
}


#endif /* NDEF_FEATURE_FULL_API */


#endif /* NDEF_FEATURE_T5T */
