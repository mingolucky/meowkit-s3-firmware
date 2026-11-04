
/**
  ******************************************************************************
  * @file           : ndef_t5t.h
  * @brief          : Provides NDEF methods and definitions to access NFC Forum Tags
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

#ifndef NDEF_T5T_H
#define NDEF_T5T_H



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

#define NDEF_T5T_MAPPING_VERSION_1_0                     (1U << 6)    /*!< T5T Version 1.0                                    */

#define NDEF_SYSINFO_FLAG_DFSID_POS                           (0U)    /*!< Info flags DFSID flag position                     */
#define NDEF_SYSINFO_FLAG_AFI_POS                             (1U)    /*!< Info flags AFI flag position                       */
#define NDEF_SYSINFO_FLAG_MEMSIZE_POS                         (2U)    /*!< Info flags Memory Size flag position               */
#define NDEF_SYSINFO_FLAG_ICREF_POS                           (3U)    /*!< Info flags IC reference flag position              */
#define NDEF_SYSINFO_FLAG_MOI_POS                             (4U)    /*!< Info flags MOI flag position                       */
#define NDEF_SYSINFO_FLAG_CMDLIST_POS                         (5U)    /*!< Info flags Command List flag position              */
#define NDEF_SYSINFO_FLAG_CSI_POS                             (6U)    /*!< Info flags CSI flag position                       */
#define NDEF_SYSINFO_FLAG_LEN_POS                             (7U)    /*!< Info flags Length  position                        */

#define NDEF_CMDLIST_READSINGLEBLOCK_POS                      (0U)    /*!< Cmd List: ReadSingleBlock position                 */
#define NDEF_CMDLIST_WRITESINGLEBLOCK_POS                     (1U)    /*!< Cmd List: WriteSingleBlock position                */
#define NDEF_CMDLIST_LOCKSINGLEBLOCK_POS                      (2U)    /*!< Cmd List: LockSingleBlock position                 */
#define NDEF_CMDLIST_READMULTIPLEBLOCKS_POS                   (3U)    /*!< Cmd List: ReadMultipleBlocks position              */
#define NDEF_CMDLIST_WRITEMULTIPLEBLOCKS_POS                  (4U)    /*!< Cmd List: WriteMultipleBlocks position             */
#define NDEF_CMDLIST_SELECT_POS                               (5U)    /*!< Cmd List: SelectSupported position                 */
#define NDEF_CMDLIST_RESETTOREADY_POS                         (6U)    /*!< Cmd List: ResetToReady position                    */
#define NDEF_CMDLIST_GETMULTIPLEBLOCKSECSTATUS_POS            (7U)    /*!< Cmd List: GetMultipleBlockSecStatus position       */

#define NDEF_CMDLIST_WRITEAFI_POS                             (0U)    /*!< Cmd List: WriteAFI position                        */
#define NDEF_CMDLIST_LOCKAFI_POS                              (1U)    /*!< Cmd List: LockAFI position                         */
#define NDEF_CMDLIST_WRITEDSFID_POS                           (2U)    /*!< Cmd List: WriteDSFID position                      */
#define NDEF_CMDLIST_LOCKDSFID_POS                            (3U)    /*!< Cmd List: LockDSFID position                       */
#define NDEF_CMDLIST_GETSYSTEMINFORMATION_POS                 (4U)    /*!< Cmd List: GetSystemInformation position            */
#define NDEF_CMDLIST_CUSTOMCMDS_POS                           (5U)    /*!< Cmd List: CustomCmds position                      */
#define NDEF_CMDLIST_FASTREADMULTIPLEBLOCKS_POS               (6U)    /*!< Cmd List: FastReadMultipleBlocks position          */

#define NDEF_CMDLIST_EXTREADSINGLEBLOCK_POS                   (0U)    /*!< Cmd List: ExtReadSingleBlock position              */
#define NDEF_CMDLIST_EXTWRITESINGLEBLOCK_POS                  (1U)    /*!< Cmd List: ExtWriteSingleBlock position             */
#define NDEF_CMDLIST_EXTLOCKSINGLEBLOCK_POS                   (2U)    /*!< Cmd List: ExtLockSingleBlock position              */
#define NDEF_CMDLIST_EXTREADMULTIPLEBLOCKS_POS                (3U)    /*!< Cmd List: ExtReadMultipleBlocks position           */
#define NDEF_CMDLIST_EXTWRITEMULTIPLEBLOCKS_POS               (4U)    /*!< Cmd List: ExtWriteMultipleBlocks position          */
#define NDEF_CMDLIST_EXTGETMULTIPLEBLOCKSECSTATUS_POS         (5U)    /*!< Cmd List: ExtGetMultipleBlockSecStatus position    */
#define NDEF_CMDLIST_FASTEXTENDEDREADMULTIPLEBLOCKS_POS       (6U)    /*!< Cmd List: FastExtendedReadMultipleBlocks position  */

#define NDEF_T5T_CC_MAGIC_1_BYTE_ADDR_MODE                   0xE1U    /*!< T5T CC Magic Number (1-byte Address Mode)           */
#define NDEF_T5T_CC_MAGIC_2_BYTE_ADDR_MODE                   0xE2U    /*!< T5T CC Magic Number (2-byte Address Mode)           */
#define NDEF_T5T_CC_LEN_4_BYTES                                 4U    /*!< T5T CC Length (4 bytes)                            */
#define NDEF_T5T_CC_LEN_8_BYTES                                 8U    /*!< T5T CC Length (8 bytes)                            */
#define NDEF_T5T_FORMAT_OPTION_NFC_FORUM                        1U    /*!< Format tag according to NFC Forum MLEN computation */

/*
 ******************************************************************************
 * GLOBAL MACROS
 ******************************************************************************
 */
#define ndefT5TMajorVersion(V)                                         ((uint8_t)( (V) >> 6U))                                            /*!< Get major version                                  */
#define ndefT5TMinorVersion(V)                                         ((uint8_t)(((V) >> 4U) & 3U))                                      /*!< Get minor version                                  */

#define ndefT5TSysInfoDFSIDPresent(infoFlags)                          (((infoFlags) >> NDEF_SYSINFO_FLAG_DFSID_POS)   & 0x01U)           /*!< Returns DFSID presence flag                        */
#define ndefT5TSysInfoAFIPresent(infoFlags)                            (((infoFlags) >> NDEF_SYSINFO_FLAG_AFI_POS)     & 0x01U)           /*!< Returns AFI presence flag                          */
#define ndefT5TSysInfoMemSizePresent(infoFlags)                        (((infoFlags) >> NDEF_SYSINFO_FLAG_MEMSIZE_POS) & 0x01U)           /*!< Returns Memory size presence flag                  */
#define ndefT5TSysInfoICRefPresent(infoFlags)                          (((infoFlags) >> NDEF_SYSINFO_FLAG_ICREF_POS)   & 0x01U)           /*!< Returns IC Reference presence flag                 */
#define ndefT5TSysInfoMOIValue(infoFlags)                              (((infoFlags) >> NDEF_SYSINFO_FLAG_MOI_POS)     & 0x01U)           /*!< Returns MOI value                                  */
#define ndefT5TSysInfoCmdListPresent(infoFlags)                        (((infoFlags) >> NDEF_SYSINFO_FLAG_CMDLIST_POS) & 0x01U)           /*!< Returns Command List presence flag                 */
#define ndefT5TSysInfoCSIPresent(infoFlags)                            (((infoFlags) >> NDEF_SYSINFO_FLAG_CSI_POS)     & 0x01U)           /*!< Returns CSI presence flag                          */
#define ndefT5TSysInfoLenValue(infoFlags)                              (((infoFlags) >> NDEF_SYSINFO_FLAG_LEN_POS)     & 0x01U)           /*!< Returns Info flag length value                     */

#define ndefT5TSysInfoReadSingleBlockSupported(cmdList)                (((cmdList)[0] >> NDEF_CMDLIST_READSINGLEBLOCK_POS)                & 0x01U) /*!< Returns ReadSingleBlock support flag                 */
#define ndefT5TSysInfoWriteSingleBlockSupported(cmdList)               (((cmdList)[0] >> NDEF_CMDLIST_WRITESINGLEBLOCK_POS)               & 0x01U) /*!< Returns WriteSingleBlock support flag                */
#define ndefT5TSysInfoLockSingleBlockSupported(cmdList)                (((cmdList)[0] >> NDEF_CMDLIST_LOCKSINGLEBLOCK_POS)                & 0x01U) /*!< Returns LockSingleBlock support flag                 */
#define ndefT5TSysInfoReadMultipleBlocksSupported(cmdList)             (((cmdList)[0] >> NDEF_CMDLIST_READMULTIPLEBLOCKS_POS)             & 0x01U) /*!< Returns ReadMultipleBlocks support flag              */
#define ndefT5TSysInfoWriteMultipleBlocksSupported(cmdList)            (((cmdList)[0] >> NDEF_CMDLIST_WRITEMULTIPLEBLOCKS_POS)            & 0x01U) /*!< Returns WriteMultipleBlocks support flag             */
#define ndefT5TSysInfoSelectSupported(cmdList)                         (((cmdList)[0] >> NDEF_CMDLIST_SELECT_POS)                         & 0x01U) /*!< Returns SelectSupported support flag                 */
#define ndefT5TSysInfoResetToReadySupported(cmdList)                   (((cmdList)[0] >> NDEF_CMDLIST_RESETTOREADY_POS)                   & 0x01U) /*!< Returns ResetToReady support flag                    */
#define ndefT5TSysInfoGetMultipleBlockSecStatusSupported(cmdList)      (((cmdList)[0] >> NDEF_CMDLIST_GETMULTIPLEBLOCKSECSTATUS_POS)      & 0x01U) /*!< Returns GetMultipleBlockSecStatus support flag       */

#define ndefT5TSysInfoWriteAFISupported(cmdList)                       (((cmdList)[1] >> NDEF_CMDLIST_WRITEAFI_POS)                       & 0x01U) /*!< Returns WriteAFI support flag                        */
#define ndefT5TSysInfoLockAFISupported(cmdList)                        (((cmdList)[1] >> NDEF_CMDLIST_LOCKAFI_POS)                        & 0x01U) /*!< Returns LockAFI support flag                         */
#define ndefT5TSysInfoWriteDSFIDSupported(cmdList)                     (((cmdList)[1] >> NDEF_CMDLIST_WRITEDSFID_POS)                     & 0x01U) /*!< Returns WriteDSFID support flag                      */
#define ndefT5TSysInfoLockDSFIDSupported(cmdList)                      (((cmdList)[1] >> NDEF_CMDLIST_LOCKDSFID_POS)                      & 0x01U) /*!< Returns LockDSFID support flag                       */
#define ndefT5TSysInfoGetSystemInformationSupported(cmdList)           (((cmdList)[1] >> NDEF_CMDLIST_GETSYSTEMINFORMATION_POS)           & 0x01U) /*!< Returns GetSystemInformation support flag            */
#define ndefT5TSysInfoCustomCmdsSupported(cmdList)                     (((cmdList)[1] >> NDEF_CMDLIST_CUSTOMCMDS_POS)                     & 0x01U) /*!< Returns CustomCmds support flag                      */
#define ndefT5TSysInfoFastReadMultipleBlocksSupported(cmdList)         (((cmdList)[1] >> NDEF_CMDLIST_FASTREADMULTIPLEBLOCKS_POS)         & 0x01U) /*!< Returns FastReadMultipleBlocks support flag          */

#define ndefT5TSysInfoExtReadSingleBlockSupported(cmdList)             (((cmdList)[2] >> NDEF_CMDLIST_EXTREADSINGLEBLOCK_POS)             & 0x01U) /*!< Returns ExtReadSingleBlock support flag              */
#define ndefT5TSysInfoExtWriteSingleBlockSupported(cmdList)            (((cmdList)[2] >> NDEF_CMDLIST_EXTWRITESINGLEBLOCK_POS)            & 0x01U) /*!< Returns ExtWriteSingleBlock support flag             */
#define ndefT5TSysInfoExtLockSingleBlockSupported(cmdList)             (((cmdList)[2] >> NDEF_CMDLIST_EXTLOCKSINGLEBLOCK_POS)             & 0x01U) /*!< Returns ExtLockSingleBlock support flag              */
#define ndefT5TSysInfoExtReadMultipleBlocksSupported(cmdList)          (((cmdList)[2] >> NDEF_CMDLIST_EXTREADMULTIPLEBLOCKS_POS)          & 0x01U) /*!< Returns ExtReadMultipleBlocks support flag           */
#define ndefT5TSysInfoExtWriteMultipleBlocksSupported(cmdList)         (((cmdList)[2] >> NDEF_CMDLIST_EXTWRITEMULTIPLEBLOCKS_POS)         & 0x01U) /*!< Returns ExtWriteMultipleBlocks support flag          */
#define ndefT5TSysInfoExtGetMultipleBlockSecStatusSupported(cmdList)   (((cmdList)[2] >> NDEF_CMDLIST_EXTGETMULTIPLEBLOCKSECSTATUS_POS)   & 0x01U) /*!< Returns ExtGetMultipleBlockSecStatus support flag    */
#define ndefT5TSysInfoFastExtendedReadMultipleBlocksSupported(cmdList) (((cmdList)[2] >> NDEF_CMDLIST_FASTEXTENDEDREADMULTIPLEBLOCKS_POS) & 0x01U) /*!< Returns FastExtendedReadMultipleBlocks support flag  */

#define ndefT5TInvalidateCache(ctx)     { (ctx)->subCtx.t5t.cacheBlock = 0xFFFFFFFFU; }    /*!< Invalidate the internal cache, before reading a buffer  */
#define ndefT5TIsValidCache(ctx, block) ( (ctx)->subCtx.t5t.cacheBlock == (block) )        /*!< Check the internal cache is valid to avoid useless read */

/*
 ******************************************************************************
 * GLOBAL TYPES
 ******************************************************************************
 */

/*! T5T Access mode */
typedef enum {
  NDEF_T5T_ACCESS_MODE_SELECTED,
  NDEF_T5T_ACCESS_MODE_ADDRESSED,
  NDEF_T5T_ACCESS_MODE_NON_ADDRESSED,
} ndefT5TAccessMode;


/*
 ******************************************************************************
 * GLOBAL FUNCTION PROTOTYPES
 ******************************************************************************
 */


/*!
 *****************************************************************************
 * \brief Set T5T access mode
 *
 * This method allows to set the access mode, among addressed, non-addressed
 * and selected modes.
 * It must be called before calling ndefT5TPollerContextInitialization().
 *
 * \param[in]   mode   : access mode
 *
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode ndefT5TPollerSetAccessMode(ndefT5TAccessMode mode);


#ifdef TEST_NDEF
  /*!
  *****************************************************************************
  * \brief Control the Multiple Read Block mode
  *
  * This method allows to turn on of off the Read Multiple Block feature.
  * It will be enabled when the MBREAD bit is set in the CC file.
  * It must be called after calling ndefT5TPollerContextInitialization() and
  * before ndefT5TPollerNdefDetect().
  *
  * \param[in]   ctx    : ndef Context
  * \param[in]   enable : enable the feature or not
  *
  * \return ERR_PARAM        : Invalid parameter
  * \return ERR_NONE         : No error
  *****************************************************************************
  */
  ReturnCode ndefT5TPollerMultipleBlockRead(ndefContext *ctx, bool enable);
#endif


/*!
 *****************************************************************************
 * \brief Handle T5T NDEF context activation
 *
 * This method performs the initialisation of the NDEF context.
 * It must be called after a successful
 * anticollision procedure and prior to any NDEF procedures such as NDEF
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
ReturnCode ndefT5TPollerContextInitialization(ndefContext *ctx, const ndefDevice *dev);


/*!
 *****************************************************************************
 * \brief T5T NDEF Detection procedure
 *
 * This method performs the T5T NDEF Detection procedure
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
ReturnCode ndefT5TPollerNdefDetect(ndefContext *ctx, ndefInfo *info);


/*!
 *****************************************************************************
 * \brief T5T Read data from tag memory
 *
 * This method reads arbitrary length data from tag memory
 *
 * \param[in]  ctx    : ndef Context
 * \param[in]  offset : file offset of where to start reading data
 * \param[in]  len    : requested length
 * \param[out] buf    : buffer to place the data read from the tag
 * \param[out] rcvdLen: received length
 *
 * \return ERR_WRONG_STATE: Library not initialized or mode not set
 * \return ERR_REQUEST    : Read failed
 * \return ERR_PARAM      : Invalid parameter
 * \return ERR_PROTO      : Protocol error
 * \return ERR_NONE       : No error
 *****************************************************************************
 */
ReturnCode ndefT5TPollerReadBytes(ndefContext *ctx, uint32_t offset, uint32_t len, uint8_t *buf, uint32_t *rcvdLen);


/*!
 *****************************************************************************
 * \brief T5T Write data to tag memory
 *
 * This method writes arbitrary length data to tag memory
 *
 * \param[in] ctx            : ndef Context
 * \param[in] offset         : file offset where to start writing data
 * \param[in] buf            : data to write
 * \param[in] len            : buf length
 * \param[in] pad            : pad remaining bytes of last modified block with 0s
 * \param[in] writeTerminator: write Terminator TLV after data
 *
 * \return ERR_WRONG_STATE: Library not initialized or mode not set
 * \return ERR_REQUEST    : Write failed
 * \return ERR_PARAM      : Invalid parameter
 * \return ERR_PROTO      : Protocol error
 * \return ERR_NONE       : No error
 *****************************************************************************
 */
ReturnCode ndefT5TPollerWriteBytes(ndefContext *ctx, uint32_t offset, const uint8_t *buf, uint32_t len, bool pad, bool writeTerminator);


/*!
 *****************************************************************************
 * \brief T5T Read raw NDEF message
 *
 * This method reads a raw NDEF message from the current selected file.
 * Prior to NDEF Read procedure, a successful ndefT5TPollerNdefDetect()
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
ReturnCode ndefT5TPollerReadRawMessage(ndefContext *ctx, uint8_t *buf, uint32_t bufLen, uint32_t *rcvdLen, bool single);


/*!
 *****************************************************************************
 * \brief T5T Write raw NDEF message
 *
 * This method writes a raw NDEF message in the current selected file.
 * Prior to NDEF Write procedure, a successful ndefT5TPollerNdefDetect()
 * has to be performed.
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
ReturnCode ndefT5TPollerWriteRawMessage(ndefContext *ctx, const uint8_t *buf, uint32_t bufLen);


/*!
 *****************************************************************************
 * \brief T5T Write NDEF message length
 *
 * This method writes the L field to the tag memory
 *
 * \param[in]   ctx            : ndef Context
 * \param[in]   rawMessageLen  : len
 * \param[in]   writeTerminator: write Terminator TLV after data
 *
 * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
 * \return ERR_REQUEST      : write failed
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_PROTO        : Protocol error
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode ndefT5TPollerWriteRawMessageLen(ndefContext *ctx, uint32_t rawMessageLen, bool writeTerminator);


/*!
 *****************************************************************************
 * \brief T5T Format Tag
 *
 * This method formats a tag to make it ready for NDEF storage.
 * When the cc parameter is provided, the Capability Container is initialized
 * with the value provided by the user.
 * When the cc parameter is not provided (i.e. NULL), this method
 * retrieves the appropriate CC field from (Extended) Get System Information
 * or returns ERR_REQUEST if (Extended) Get System Information is not supported.
 * The option parameter is used in case of NULL cc parameter to choose
 * between Android CC format (default when option = 0)
 * or NFC Forum Format (option = NDEF_T5T_FORMAT_OPTION_NFC_FORUM)
 *
 *
 * \param[in]   ctx     : ndef Context
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
ReturnCode ndefT5TPollerTagFormat(ndefContext *ctx, const ndefCapabilityContainer *cc, uint32_t options);


/*!
 *****************************************************************************
 * \brief T5T Check Presence
 *
 * This method checks whether a T5T tag is still present in the operating field
 *
 * \param[in]   ctx    : ndef Context

 *
 * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_PROTO        : Protocol error
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode ndefT5TPollerCheckPresence(ndefContext *ctx);


/*!
 *****************************************************************************
 * \brief T5T Check Available Space
 *
 * This method checks whether a T5T tag has enough space to write a message of a given length
 *
 * \param[in]   ctx       : ndef Context
 * \param[in]   messageLen: message length
 *
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_NOMEM        : not enough space
 * \return ERR_NONE         : Enough space for message of messageLen length
 *****************************************************************************
 */
ReturnCode ndefT5TPollerCheckAvailableSpace(const ndefContext *ctx, uint32_t messageLen);


/*!
 *****************************************************************************
 * \brief T5T Begin Write Message
 *
 * This method sets the L-field to 0 and sets the message offset to the proper value according to messageLen
 *
 * \param[in]   ctx       : ndef Context
 * \param[in]   messageLen: message length
 *
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_NOMEM        : not enough space
 * \return ERR_NONE         : Enough space for message of messageLen length
 *****************************************************************************
 */
ReturnCode ndefT5TPollerBeginWriteMessage(ndefContext *ctx, uint32_t messageLen);


/*!
 *****************************************************************************
 * \brief T5T End Write Message
 *
 * This method updates the L-field value after the message has been written
 *
 * \param[in]   ctx            : ndef Context
 * \param[in]   messageLen     : message length
 * \param[in]   writeTerminator: write Terminator TLV after data
 *
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_NOMEM        : not enough space
 * \return ERR_NONE         : Enough space for message of messageLen length
 *****************************************************************************
 */
ReturnCode ndefT5TPollerEndWriteMessage(ndefContext *ctx, uint32_t messageLen, bool writeTerminator);


/*!
 *****************************************************************************
 * \brief T5T Set Read Only
 *
 * This method performs the transition from the READ/WRITE state to the READ-ONLY state
 *
 * \param[in]   ctx       : ndef Context
 *
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode ndefT5TPollerSetReadOnly(ndefContext *ctx);



#endif /* NDEF_T5T_H */

/**
  * @}
  */
