/******************************************************************************
  * \attention
  *
  * <h2><center>&copy; COPYRIGHT 2021 STMicroelectronics</center></h2>
  *
  * Licensed under ST MIX MYLIBERTY SOFTWARE LICENSE AGREEMENT (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        www.st.com/mix_myliberty
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied,
  * AND SPECIFICALLY DISCLAIMING THE IMPLIED WARRANTIES OF MERCHANTABILITY,
  * FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
******************************************************************************/

/*! \file
 *
 *  \author SRA
 *
 *  \brief Provides NDEF methods and definitions to access NFC Forum Tags
 *
 *  NDEF provides several functionalities required to perform NFC NDEF activities.
 *  <br>The NDEF encapsulates the different tag technologies (T2T, T3T, T4AT, T4BT, T5T)
 *  into a common and easy to use interface.
 *
 *  It provides interfaces to Detect, Read, Write and Format NDEF.
 *
 *
 * \addtogroup NDEF
 * @{
 *
 */


#ifndef NDEF_CLASS_H
#define NDEF_CLASS_H

/*
 ******************************************************************************
 * INCLUDES
 ******************************************************************************
 */
#include "st_errno.h"
#include "nfc_utils.h"
#include "rfal_nfc.h"
#include "ndef_config.h"
#include "ndef_poller.h"
#include "ndef_record.h"
#include "ndef_message.h"
#include "ndef_types.h"


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
 * NDEF FEATURES CONFIGURATION
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

class NdefClass {

  public:

    NdefClass(RfalNfcClass *rfal_nfc) : rfal_nfc(rfal_nfc)
    {
      memset(&ctx, 0, sizeof(ndefContext));
      ctx.ndef_class_instance = this;
    }

    /*
    ******************************************************************************
    * NDEF POLLER FUNCTION PROTOTYPES
    ******************************************************************************
    */

    /*!
     *****************************************************************************
     * \brief Handle NDEF context activation
     *
     * This method performs the initialization of the NDEF context.
     * It must be called after a successful
     * anti-collision procedure and prior to any NDEF procedures such as NDEF
     * detection procedure.
     *
     * \param[in]   dev    : ndef Device
     *
     * \return ERR_PARAM        : Invalid parameter
     * \return ERR_PROTO        : Protocol error
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode ndefPollerContextInitializationWrapper(rfalNfcDevice *dev)
    {
      return ndefPollerContextInitialization(&ctx, dev);
    }


    /*!
     *****************************************************************************
     * \brief NDEF Detection procedure
     *
     * This method performs the NDEF Detection procedure
     *
     * \param[out]  info   : ndef Information (optional parameter, NULL may be used when no NDEF Information is needed)
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
     * \return ERR_REQUEST      : Detection failed
     * \return ERR_PARAM        : Invalid parameter
     * \return ERR_PROTO        : Protocol error
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode ndefPollerNdefDetectWrapper(ndefInfo *info)
    {
      return ndefPollerNdefDetect(&ctx, info);
    }


    /*!
     *****************************************************************************
     * \brief Read data
     *
     * This method reads arbitrary length data
     *
     * \param[in]   offset : file offset of where to start reading data
     * \param[in]   len    : requested len
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
    ReturnCode ndefPollerReadBytesWrapper(uint32_t offset, uint32_t len, uint8_t *buf, uint32_t *rcvdLen)
    {
      return ndefPollerReadBytes(&ctx, offset, len, buf, rcvdLen);
    }


    /*!
     *****************************************************************************
     * \brief  Write data
     *
     * This method writes arbitrary length data from the current selected file
     *
     * \param[in]   offset : file offset of where to start writing data
     * \param[in]   buf    : data to write
     * \param[in]   len    : buf len
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
     * \return ERR_REQUEST      : read failed
     * \return ERR_PARAM        : Invalid parameter
     * \return ERR_PROTO        : Protocol error
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode ndefPollerWriteBytesWrapper(uint32_t offset, const uint8_t *buf, uint32_t len)
    {
      return ndefPollerWriteBytes(&ctx, offset, buf, len);
    }


    /*!
     *****************************************************************************
     * \brief Read raw NDEF message
     *
     * This method reads a raw NDEF message.
     * Prior to NDEF Read procedure, a successful ndefPollerNdefDetect()
     * has to be performed.
     *
     *
     * \param[out]  buf    : buffer to place the NDEF message
     * \param[in]   bufLen : buffer length
     * \param[out]  rcvdLen: received length
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
     * \return ERR_REQUEST      : read failed
     * \return ERR_PARAM        : Invalid parameter
     * \return ERR_PROTO        : Protocol error
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode ndefPollerReadRawMessageWrapper(uint8_t *buf, uint32_t bufLen, uint32_t *rcvdLen, bool single)
    {
      return ndefPollerReadRawMessage(&ctx, buf, bufLen, rcvdLen, single);
    }


    /*!
     *****************************************************************************
     * \brief Write raw NDEF message
     *
     * This method writes a raw NDEF message.
     * Prior to NDEF Write procedure, a successful ndefPollerNdefDetect()
     * has to be performed.
     *
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
    ReturnCode ndefPollerWriteRawMessageWrapper(const uint8_t *buf, uint32_t bufLen)
    {
      return ndefPollerWriteRawMessage(&ctx, buf, bufLen);
    }


    /*!
     *****************************************************************************
     * \brief Format Tag
     *
     * This method format a tag to make it ready for NDEF storage.
     * cc and options parameters usage is described in each technology method
     * (ndefT[2345]TPollerTagFormat)
     *
     * \param[in]   cc      : Capability Container
     * \param[in]   options : specific flags
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
     * \return ERR_REQUEST      : write failed
     * \return ERR_PARAM        : Invalid parameter
     * \return ERR_PROTO        : Protocol error
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode ndefPollerTagFormatWrapper(const ndefCapabilityContainer *cc, uint32_t options)
    {
      return ndefPollerTagFormat(&ctx, cc, options);
    }


    /*!
     *****************************************************************************
     * \brief Write NDEF message length
     *
     * This method writes the NLEN field
     *
     * \param[in]   rawMessageLen: len
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
     * \return ERR_REQUEST      : write failed
     * \return ERR_PARAM        : Invalid parameter
     * \return ERR_PROTO        : Protocol error
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode ndefPollerWriteRawMessageLenWrapper(uint32_t rawMessageLen)
    {
      return ndefPollerWriteRawMessageLen(&ctx, rawMessageLen);
    }


    /*!
    *****************************************************************************
    * \brief Write an NDEF message
    *
    * Write the NDEF message to the tag
    *
    * \param[in] message: Message to write
    *
    * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
    * \return ERR_REQUEST      : write failed
    * \return ERR_PARAM        : Invalid parameter
    * \return ERR_PROTO        : Protocol error
    * \return ERR_NONE         : No error
    *****************************************************************************
    */
    ReturnCode ndefPollerWriteMessageWrapper(const ndefMessage *message)
    {
      return ndefPollerWriteMessage(&ctx, message);
    }


    /*!
     *****************************************************************************
     * \brief Check Presence
     *
     * This method check whether an NFC tag is still present in the operating field
     *
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
     * \return ERR_PARAM        : Invalid parameter
     * \return ERR_PROTO        : Protocol error
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode ndefPollerCheckPresenceWrapper()
    {
      return ndefPollerCheckPresence(&ctx);
    }


    /*!
     *****************************************************************************
     * \brief Check Available Space
     *
     * This method check whether a NFC tag has enough space to write a message of a given length
     *
     * \param[in]   messageLen: message length
     *
     * \return ERR_PARAM        : Invalid parameter
     * \return ERR_NOMEM        : not enough space
     * \return ERR_NONE         : Enough space for message of messageLen length
     *****************************************************************************
     */
    ReturnCode ndefPollerCheckAvailableSpaceWrapper(uint32_t messageLen)
    {
      return ndefPollerCheckAvailableSpace(&ctx, messageLen);
    }


    /*!
     *****************************************************************************
     * \brief Begin Write Message
     *
     * This method sets the L-field to 0 (T1T, T2T, T4T, T5T) or set the WriteFlag (T3T) and sets the message offset to the proper value according to messageLen
     *
     * \param[in]   messageLen: message length
     *
     * \return ERR_PARAM        : Invalid parameter
     * \return ERR_NOMEM        : not enough space
     * \return ERR_NONE         : Enough space for message of messageLen length
     *****************************************************************************
     */
    ReturnCode ndefPollerBeginWriteMessageWrapper(uint32_t messageLen)
    {
      return ndefPollerBeginWriteMessage(&ctx, messageLen);
    }


    /*!
     *****************************************************************************
     * \brief End Write Message
     *
     * This method updates the L-field value after the message has been written and resets the WriteFlag (for T3T only)
     *
     * \param[in]   messageLen: message length
     *
     * \return ERR_PARAM        : Invalid parameter
     * \return ERR_NOMEM        : not enough space
     * \return ERR_NONE         : Enough space for message of messageLen length
     *****************************************************************************
     */
    ReturnCode ndefPollerEndWriteMessageWrapper(uint32_t messageLen)
    {
      return ndefPollerEndWriteMessage(&ctx, messageLen);
    }

    ndefContext ctx;
    RfalNfcClass *rfal_nfc;
};

#endif /* NDEF_CLASS_H */

/**
  * @}
  *
  */
