
/**
  ******************************************************************************
  * @file           : ndef_t3t.h
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


#ifndef NDEF_T3T_H
#define NDEF_T3T_H



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


/*
 ******************************************************************************
 * GLOBAL MACROS
 ******************************************************************************
 */

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
 * \brief Handle T3T NDEF context activation
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
ReturnCode ndefT3TPollerContextInitialization(ndefContext *ctx, const ndefDevice *dev);


/*!
 *****************************************************************************
 * \brief T3T NDEF Detection procedure
 *
 * This method performs the T3T NDEF Detection procedure
 *
 *
 * \param[in]   ctx    : ndef Context
 * \param[out]  info   : ndef Information (optional parameter, NULL may be used when no NDEF Information is needed)
 *
 * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
 * \return ERR_REQUEST      : Detection failed
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_PROTO        : Protocol error
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode ndefT3TPollerNdefDetect(ndefContext *ctx, ndefInfo *info);


/*!
 *****************************************************************************
 * \brief T3T Read data from file
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
 * \return ERR_REQUEST      : read failed
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_PROTO        : Protocol error
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode ndefT3TPollerReadBytes(ndefContext *ctx, uint32_t offset, uint32_t len, uint8_t *buf, uint32_t *rcvdLen);


/*!
 *****************************************************************************
 * \brief T3T write data to file
 *
 * This method reads arbitrary length data from the current selected file
 *
 * \param[in]   ctx            : ndef Context
 * \param[in]   offset         : file offset of where to start writing data
 * \param[in]   buf            : data to write
 * \param[in]   len            : buf length
 * \param[in]   pad            : pad remaining bytes of last modified block with 0s
 * \param[in]   writeTerminator: write Terminator TLV after data
 *
 * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
 * \return ERR_REQUEST      : read failed
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_PROTO        : Protocol error
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode ndefT3TPollerWriteBytes(ndefContext *ctx, uint32_t offset, const uint8_t *buf, uint32_t len, bool pad, bool writeTerminator);


/*!
 *****************************************************************************
 * \brief T3T Read raw NDEF message
 *
 * This method reads a raw NDEF message from the current selected file.
 * Prior to NDEF Read procedure, a successful ndefT3TPollerNdefDetect()
 * has to be performed.
 *
 * \param[in]   ctx    : ndef Context
 * \param[out]  buf    : buffer to place the NDEF message
 * \param[in]   bufLen : buffer length
 * \param[out]  rcvdLen: received length
 * \param[in]   single : performs the procedure as part of a single NDEF read operation
 *
 * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
 * \return ERR_REQUEST      : read failed
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_PROTO        : Protocol error
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode ndefT3TPollerReadRawMessage(ndefContext *ctx, uint8_t *buf, uint32_t bufLen, uint32_t *rcvdLen, bool single);


/*!
 *****************************************************************************
 * \brief T3T Write raw NDEF message
 *
 * This method writes a raw NDEF message in the current selected file.
 * Prior to NDEF Write procedure, a successful ndefT3TPollerNdefDetect()
 * has to be performed.
 *
 * \warning Current selected file must not be changed between NDEF Detect
 * procedure and NDEF Write procedure. If another file is selected before
 * NDEF Write procedure, it is user responsibility to re-select NDEF file
 * or to call ndefT3TPollerNdefDetect() to restore proper context.
 *
 * \param[in]   ctx    : ndef Context
 * \param[in]   buf    : raw message buffer
 * \param[in]   bufLen : buffer length
 *
 * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
 * \return ERR_REQUEST      : write failed
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_PROTO        : Protocol error
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode ndefT3TPollerWriteRawMessage(ndefContext *ctx, const uint8_t *buf, uint32_t bufLen);


/*!
 *****************************************************************************
 * \brief T3T Write NDEF message length
 *
 * This method writes the NLEN field (V2 mapping) or the ENLEN (V3 mapping).
 *
 * \param[in]   ctx            : ndef Context
 * \param[in]   rawMessageLen  : len
 * \param[in]   writeTerminator: unused
 *
 * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
 * \return ERR_REQUEST      : write failed
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_PROTO        : Protocol error
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode ndefT3TPollerWriteRawMessageLen(ndefContext *ctx, uint32_t rawMessageLen, bool writeTerminator);


/*!
 *****************************************************************************
 * \brief T3T Format Tag
 *
 * This method formats a tag to make it ready for NDEF storage.
 * cc parameter contains the Attribute Information Block fields
 * If cc parameter is not provided (i.e. NULL), this method assumes
 * that the AIB is already present.
 * options parameter is not used for T3T Tag Format method
 *
 *
 * \param[in]   ctx    : ndef Context
 * \param[in]   cc     : Capability Container
 * \param[in]   options: specific flags
 *
 * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
 * \return ERR_REQUEST      : write failed
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_PROTO        : Protocol error
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode ndefT3TPollerTagFormat(ndefContext *ctx, const ndefCapabilityContainer *cc, uint32_t options);


/*!
 *****************************************************************************
 * \brief T3T Check Presence
 *
 * This method checks whether a T3T tag is still present in the operating field
 *
 * \param[in]   ctx    : ndef Context

 *
 * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_PROTO        : Protocol error
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode ndefT3TPollerCheckPresence(ndefContext *ctx);


/*!
 *****************************************************************************
 * \brief T3T Check Available Space
 *
 * This method checks whether a T3T tag has enough space to write a message of a given length
 *
 * \param[in]   ctx       : ndef Context
 * \param[in]   messageLen: message length
 *
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_NOMEM        : not enough space
 * \return ERR_NONE         : Enough space for message of messageLen length
 *****************************************************************************
 */
ReturnCode ndefT3TPollerCheckAvailableSpace(const ndefContext *ctx, uint32_t messageLen);


/*!
 *****************************************************************************
 * \brief T3T Begin Write Message
 *
 * This method sets the WriteFlag to the appropriate value before starting to write the NDEF message
 *
 * \param[in]   ctx       : ndef Context
 * \param[in]   messageLen: message length
 *
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_NOMEM        : not enough space
 * \return ERR_NONE         : Enough space for message of messageLen length
 *****************************************************************************
 */
ReturnCode ndefT3TPollerBeginWriteMessage(ndefContext *ctx, uint32_t messageLen);


/*!
 *****************************************************************************
 * \brief T3T End Write Message
 *
 * This method updates the Ln field of the AIB and resets the WriteFlag
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
ReturnCode ndefT3TPollerEndWriteMessage(ndefContext *ctx, uint32_t messageLen, bool writeTerminator);


/*!
 *****************************************************************************
 * \brief T3T Set Read Only
 *
 * This method perform the transition from the READ/WRITE state to the READ-ONLY state
 *
 * \param[in]   ctx       : ndef Context
 *
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode ndefT3TPollerSetReadOnly(ndefContext *ctx);



#endif /* NDEF_T3T_H */

/**
  * @}
  */
