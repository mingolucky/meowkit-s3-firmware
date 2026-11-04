
/**
  ******************************************************************************
  * @file           : ndef_t4t.h
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


#ifndef NDEF_T4T_H
#define NDEF_T4T_H



/*
 ******************************************************************************
 * INCLUDES
 ******************************************************************************
 */
#include "ndef_poller.h"

/*
 ******************************************************************************
 * GLOBAL DEFINES
 ******************************************************************************
 */

#define NDEF_T4T_MAPPING_VERSION_2_0  0x20U                    /*!< Mapping version 2.0                   */
#define NDEF_T4T_MAPPING_VERSION_3_0  0x30U                    /*!< Mapping version 3.0                   */

/*! Minimum size for an APDU (corresponding to Select NDEF App)     */
#define NDEF_T4T_MIN_APDU_LEN                                   13U

/*! Maximum Response-APDU response body length (short field coding) */
#if RFAL_FEATURE_ISO_DEP_APDU_MAX_LEN > (256 + RFAL_T4T_MAX_RAPDU_SW1SW2_LEN)
  #define NDEF_T4T_MAX_RAPDU_BODY_LEN                            256U
#else
  #define NDEF_T4T_MAX_RAPDU_BODY_LEN (RFAL_FEATURE_ISO_DEP_APDU_MAX_LEN - RFAL_T4T_MAX_RAPDU_SW1SW2_LEN)
#endif

/*! Maximum Command-APDU data length (short field coding)           */
#if RFAL_FEATURE_ISO_DEP_APDU_MAX_LEN > (255 + RFAL_T4T_MAX_CAPDU_PROLOGUE_LEN + RFAL_T4T_LC_LEN + RFAL_T4T_LE_LEN)
  #define NDEF_T4T_MAX_CAPDU_BODY_LEN                            255U
#else
  #define NDEF_T4T_MAX_CAPDU_BODY_LEN (RFAL_FEATURE_ISO_DEP_APDU_MAX_LEN - (RFAL_T4T_MAX_CAPDU_PROLOGUE_LEN + RFAL_T4T_LC_LEN + RFAL_T4T_LE_LEN))
#endif


/*
 ******************************************************************************
 * GLOBAL MACROS
 ******************************************************************************
 */
#define ndefT4TIsReadAccessGranted(r)  ( ((r) == 0x00U) || (((r) >= 0x80U) && ((r) <= 0xFEU)) ) /*!< Read access status  */
#define ndefT4TIsWriteAccessGranted(w) ( ((w) == 0x00U) || (((w) >= 0x80U) && ((w) <= 0xFEU)) ) /*!< Write access status */

/*
 ******************************************************************************
 * GLOBAL TYPES
 ******************************************************************************
 */

/*
******************************************************************************
* GLOBAL FUNCTION PROTOTYPES
******************************************************************************
*/

/*!
 *****************************************************************************
 * \brief Handle T4T NDEF context activation
 *
 * This method performs the initialization of the NDEF context and handles
 * the activation of the ISO-DEP layer. It must be called after a successful
 * anti-collision procedure and prior to any NDEF procedures such as NDEF
 * detection procedure.
 *
 * \param[in]   ctx    : ndef Context
 * \param[in]   dev    : ndef Device
 *
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_PROTO        : Protocol error
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode ndefT4TPollerContextInitialization(ndefContext *ctx, const ndefDevice *dev);


/*!
 *****************************************************************************
 * \brief T4T NDEF Detection procedure
 *
 * This method performs the T4T NDEF Detection procedure
 *
 *
 * \param[in]   ctx    : ndef Context
 * \param[out]  info   : ndef Information (optional parameter, NULL may be used when no NDEF Information is needed)
 *
 * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
 * \return ERR_REQUEST      : Detection failed (application or ccfile not found)
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_PROTO        : Protocol error
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode ndefT4TPollerNdefDetect(ndefContext *ctx, ndefInfo *info);


/*!
 *****************************************************************************
 * \brief T4T Select NDEF Tag Application
 *
 * This method sends the Select NDEF tag application. If V2 Tag Application
 * is not found, a Select NDEF tag V1 application is sent/
 *
 * \param[in]   ctx    : ndef Context
 *
 * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
 * \return ERR_REQUEST      : Application not found
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_PROTO        : Protocol error
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode ndefT4TPollerSelectNdefTagApplication(ndefContext *ctx);


/*!
 *****************************************************************************
 * \brief T4T Select File
 *
 * This method sends a Select File Command-APDU.
 *
 * The following fields of the ndef Context must be filled up before calling
 * this method:
 * - devType: device type
 * - subCtx.t4t.mv1Flag: Mapping version 1 flag
 *
 * \param[in]   ctx    : ndef Context
 * \param[in]   fileId : file identifier
 *
 * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
 * \return ERR_REQUEST      : File not found
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_PROTO        : Protocol error
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode ndefT4TPollerSelectFile(ndefContext *ctx, const uint8_t *fileId);


/*!
 *****************************************************************************
 * \brief T4T ReadBinary
 *
 * This method reads the data from the tag using a single
 * ReadBinary command
 *
 * \param[in]   ctx    : ndef Context
 * \param[in]   offset : file offset of where to star reading data; valid range 0000h-7FFFh
 * \param[in]   len    : requested length (short field coding)
 *
 * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
 * \return ERR_REQUEST      : read failed (SW1SW2 <> 9000h)
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_PROTO        : Protocol error
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode ndefT4TPollerReadBinary(ndefContext *ctx, uint16_t offset, uint8_t len);


/*!
 *****************************************************************************
 * \brief T4T ReadBinary with ODO
 *
 * This method reads the data from the tag using a single
 * ReadBinary ODO command
 *
 * \param[in]   ctx    : ndef Context
 * \param[in]   offset : file offset of where to star reading data; valid range 0000h-7FFFh
 * \param[in]   len    : requested length (short field coding)
 *
 * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
 * \return ERR_REQUEST      : read failed (SW1SW2 <> 9000h)
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_PROTO        : Protocol error
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode ndefT4TPollerReadBinaryODO(ndefContext *ctx, uint32_t offset, uint8_t len);


/*!
 *****************************************************************************
 * \brief T4T Read data from file
 *
 * This method reads arbitrary length data from the current selected file
 *
 * \param[in]   ctx    : ndef Context
 * \param[in]   len    : requested length
 * \param[in]   offset : file offset of where to start reading data
 * \param[out]  buf    : buffer to place the data read from the tag
 * \param[out]  rcvdLen: received length
 *
 * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
 * \return ERR_REQUEST      : read failed (SW1SW2 <> 9000h)
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_PROTO        : Protocol error
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode ndefT4TPollerReadBytes(ndefContext *ctx, uint32_t offset, uint32_t len, uint8_t *buf, uint32_t *rcvdLen);

/*!
 *****************************************************************************
 * \brief T4T WriteBinary
 *
 * This method writes the data to the tag using a single
 * WriteBinary command
 *
 * \param[in]   ctx    : ndef Context
 * \param[in]   offset : file offset of where to star reading data; valid range 0000h-7FFFh
 * \param[in]   data   : data to be written
 * \param[in]   len    : data length
 *
 * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
 * \return ERR_REQUEST      : read failed (SW1SW2 <> 9000h)
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_PROTO        : Protocol error
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode ndefT4TPollerWriteBinary(ndefContext *ctx, uint16_t offset, const uint8_t *data, uint8_t len);


/*!
 *****************************************************************************
 * \brief T4T WriteBinary with ODO
 *
 * This method writes the data to the tag using a single
 * WriteBinary ODO command
 *
 * \param[in]   ctx    : ndef Context
 * \param[in]   offset : file offset of where to star reading data; valid range 0000h-7FFFh
 * \param[in]   data   : data to be written
 * \param[in]   len    : data length
 *
 * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
 * \return ERR_REQUEST      : read failed (SW1SW2 <> 9000h)
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_PROTO        : Protocol error
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode ndefT4TPollerWriteBinaryODO(ndefContext *ctx, uint32_t offset, const uint8_t *data, uint8_t len);

/*!
 *****************************************************************************
 * \brief T4T write data to file
 *
 * This method reads arbitrary length data from the current selected file
 *
 * \param[in]   ctx            : ndef Context
 * \param[in]   offset         : file offset of where to start writing data
 * \param[in]   buf            : data to write
 * \param[in]   len            : buf length
 * \param[in]   pad            : unused
 * \param[in]   writeTerminator: unused
 *
 * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
 * \return ERR_REQUEST      : read failed (SW1SW2 <> 9000h)
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_PROTO        : Protocol error
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode ndefT4TPollerWriteBytes(ndefContext *ctx, uint32_t offset, const uint8_t *buf, uint32_t len, bool pad, bool writeTerminator);


/*!
 *****************************************************************************
 * \brief T4T Read raw NDEF message
 *
 * This method reads a raw NDEF message from the current selected file.
 * Prior to NDEF Read procedure, a successful ndefT4TPollerNdefDetect()
 * has to be performed.
 *
 * \warning Current selected file must not be changed between NDEF Detect
 * procedure and NDEF Read procedure. If another file is selected before
 * NDEF Read procedure, it is user responsibility to re-select NDEF file
 * or to call ndefT4TPollerNdefDetect() to restore proper context.
 *
 * \param[in]   ctx    : ndef Context
 * \param[out]  buf    : buffer to place the NDEF message
 * \param[in]   bufLen : buffer length
 * \param[out]  rcvdLen: received length
 * \param[in]   single : performs the procedure as part of a single NDEF read operation
 *
 * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
 * \return ERR_REQUEST      : read failed (SW1SW2 <> 9000h)
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_PROTO        : Protocol error
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode ndefT4TPollerReadRawMessage(ndefContext *ctx, uint8_t *buf, uint32_t bufLen, uint32_t *rcvdLen, bool single);


/*!
 *****************************************************************************
 * \brief T4T Write raw NDEF message
 *
 * This method writes a raw NDEF message in the current selected file.
 * Prior to NDEF Write procedure, a successful ndefT4TPollerNdefDetect()
 * has to be performed.
 *
 * \warning Current selected file must not be changed between NDEF Detect
 * procedure and NDEF Write procedure. If another file is selected before
 * NDEF Write procedure, it is user responsibility to re-select NDEF file
 * or to call ndefT4TPollerNdefDetect() to restore proper context.
 *
 * \param[in]   ctx    : ndef Context
 * \param[in]   buf    : raw message buffer
 * \param[in]   bufLen : buffer length
 *
 * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
 * \return ERR_REQUEST      : write failed (SW1SW2 <> 9000h)
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_PROTO        : Protocol error
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode ndefT4TPollerWriteRawMessage(ndefContext *ctx, const uint8_t *buf, uint32_t bufLen);


/*!
 *****************************************************************************
 * \brief T4T Write NDEF message length
 *
 * This method writes the NLEN field (V2 mapping) or the ENLEN (V3 mapping).
 *
 * \param[in]   ctx            : ndef Context
 * \param[in]   rawMessageLen  : len
 * \param[in]   writeTerminator: unused
 *
 * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
 * \return ERR_REQUEST      : write failed (SW1SW2 <> 9000h)
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_PROTO        : Protocol error
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode ndefT4TPollerWriteRawMessageLen(ndefContext *ctx, uint32_t rawMessageLen, bool writeTerminator);


/*!
 *****************************************************************************
 * \brief T4T Format Tag
 *
 * This method formats a tag to make it ready for NDEF storage. In case of T4T,
 * it writes NLEN/ENLEN=0 to the NDEF File.
 * cc and options parameters are not used for T4T Tag Format method.
 *
 * \param[in]   ctx    : ndef Context
 * \param[in]   cc     : Capability Container
 * \param[in]   options: specific flags
 *
 * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
 * \return ERR_REQUEST      : write failed (SW1SW2 <> 9000h)
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_PROTO        : Protocol error
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode ndefT4TPollerTagFormat(ndefContext *ctx, const ndefCapabilityContainer *cc, uint32_t options);


/*!
 *****************************************************************************
 * \brief T4T Check Presence
 *
 * This method checks whether a T4T tag is still present in the operating field
 *
 * \param[in]   ctx    : ndef Context

 *
 * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_PROTO        : Protocol error
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode ndefT4TPollerCheckPresence(ndefContext *ctx);


/*!
 *****************************************************************************
 * \brief T4T Check Available Space
 *
 * This method checks whether a T4T tag has enough space to write a message of a given length
 *
 * \param[in]   ctx       : ndef Context
 * \param[in]   messageLen: message length
 *
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_NOMEM        : not enough space
 * \return ERR_NONE         : Enough space for message of messageLen length
 *****************************************************************************
 */
ReturnCode ndefT4TPollerCheckAvailableSpace(const ndefContext *ctx, uint32_t messageLen);


/*!
 *****************************************************************************
 * \brief T4T Begin Write Message
 *
 * This method sets the L-field to 0
 *
 * \param[in]   ctx       : ndef Context
 * \param[in]   messageLen: message length
 *
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_NOMEM        : not enough space
 * \return ERR_NONE         : Enough space for message of messageLen length
 *****************************************************************************
 */
ReturnCode ndefT4TPollerBeginWriteMessage(ndefContext *ctx, uint32_t messageLen);


/*!
 *****************************************************************************
 * \brief T4T End Write Message
 *
 * This method updates the L-field value after the message has been written
 *
 * \param[in]   ctx            : ndef Context
 * \param[in]   messageLen     : message length
 * \param[in]   writeTerminator: unused
 *
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_NOMEM        : not enough space
 * \return ERR_NONE         : Enough space for message of messageLen length
 *****************************************************************************
 */
ReturnCode ndefT4TPollerEndWriteMessage(ndefContext *ctx, uint32_t messageLen, bool writeTerminator);


/*!
 *****************************************************************************
 * \brief T4T Set Read Only
 *
 * This method perform the transition from the READ/WRITE state to the READ-ONLY state
 *
 * \param[in]   ctx       : ndef Context
 *
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode ndefT4TPollerSetReadOnly(ndefContext *ctx);



#endif /* NDEF_T4T_H */

/**
  * @}
  */
