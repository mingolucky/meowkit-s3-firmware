
/**
  ******************************************************************************
  * @file           : ndef_poller.h
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


#ifndef NDEF_POLLER_H
#define NDEF_POLLER_H



/*
 ******************************************************************************
 * INCLUDES
 ******************************************************************************
 */
#include "ndef_message.h"


/*
 ******************************************************************************
 * GLOBAL DEFINES
 ******************************************************************************
 */

#define NDEF_CC_BUF_LEN             17U                                                /*!< CC buffer len. Max length = 17 in case of T4T v3             */
#define NDEF_NFCV_SUPPORTED_CMD_LEN  4U                                                /*!< Ext sys info supported commands list length                  */
#define NDEF_NFCV_UID_LEN            8U                                                /*!< NFC-V UID length                                             */

#define NDEF_SHORT_VFIELD_MAX_LEN  254U                                                /*!< Max V-field length for 1-byte Length encoding                 */
#define NDEF_TERMINATOR_TLV_LEN      1U                                                /*!< Terminator TLV size                                          */
#define NDEF_TERMINATOR_TLV_T     0xFEU                                                /*!< Terminator TLV T=FEh                                         */

#define NDEF_T2T_READ_RESP_SIZE     16U                                                /*!< Size of the READ response i.e. four blocks                   */
#define NDEF_T2T_MAX_RSVD_AREAS      3U                                                /*!< Number of reserved areas including 1 Dyn Lock area           */

#define NDEF_T3T_BLOCK_SIZE         16U                                                /*!< size for a block in t3t                                      */
#define NDEF_T3T_MAX_NB_BLOCKS       4U                                                /*!< size for a block in t3t                                      */
#define NDEF_T3T_BLOCK_NUM_MAX_SIZE  3U                                                /*!< Maximum size for a block number                              */
#define NDEF_T3T_MAX_RX_SIZE      ((NDEF_T3T_BLOCK_SIZE*NDEF_T3T_MAX_NB_BLOCKS) + 13U) /*!< size for a CHECK Response 13 bytes (LEN+07h+NFCID2+Status+Nos) + (block size x Max Nob)                                                */
#define NDEF_T3T_MAX_TX_SIZE      (((NDEF_T3T_BLOCK_SIZE + NDEF_T3T_BLOCK_NUM_MAX_SIZE) * NDEF_T3T_MAX_NB_BLOCKS) + 14U) \
                                                                                       /*!< size for an UPDATE command, 11 bytes (LEN+08h+NFCID2+Nos) + 2 bytes for 1 SC + 1 byte for NoB + (block size + block num Len) x Max NoB */

#define NDEF_T5T_TxRx_BUFF_HEADER_SIZE        1U                                       /*!< Request Flags/Responses Flags size                           */
#define NDEF_T5T_TxRx_BUFF_FOOTER_SIZE        2U                                       /*!< CRC size                                                     */

#define NDEF_T5T_TxRx_BUFF_SIZE               \
          (32U +  NDEF_T5T_TxRx_BUFF_HEADER_SIZE + NDEF_T5T_TxRx_BUFF_FOOTER_SIZE)     /*!< T5T working buffer size                                      */

/*
 ******************************************************************************
 * GLOBAL MACROS
 ******************************************************************************
 */

#define ndefBytes2Uint16(hiB, loB)          ((uint16_t)((((uint32_t)(hiB)) << 8U) | ((uint32_t)(loB))))                                                  /*!< convert 2 bytes to a u16 */

#define ndefMajorVersion(V)                 ((uint8_t)((V) >>  4U))    /*!< Get major version */
#define ndefMinorVersion(V)                 ((uint8_t)((V) & 0xFU))    /*!< Get minor version */


/*
 ******************************************************************************
 * GLOBAL TYPES
 ******************************************************************************
 */


/*! NDEF device type */
typedef enum {
  NDEF_DEV_NONE          = 0x00U,                            /*!< Device type undef                                  */
  NDEF_DEV_T1T           = 0x01U,                            /*!< Device type T1T                                    */
  NDEF_DEV_T2T           = 0x02U,                            /*!< Device type T2T                                    */
  NDEF_DEV_T3T           = 0x03U,                            /*!< Device type T3T                                    */
  NDEF_DEV_T4T           = 0x04U,                            /*!< Device type T4AT/T4BT                              */
  NDEF_DEV_T5T           = 0x05U,                            /*!< Device type T5T                                    */
} ndefDeviceType;

/*! NDEF states  */
typedef enum {
  NDEF_STATE_INVALID     = 0x00U,                            /*!< Invalid state (e.g. no CC)                         */
  NDEF_STATE_INITIALIZED = 0x01U,                            /*!< State Initialized (no NDEF message)                */
  NDEF_STATE_READWRITE   = 0x02U,                            /*!< Valid NDEF found. Read/Write capability            */
  NDEF_STATE_READONLY    = 0x03U,                            /*!< Valid NDEF found. Read only                        */
} ndefState;

/*! NDEF Information */
typedef struct {
  uint8_t                  majorVersion;                     /*!< Major version                                      */
  uint8_t                  minorVersion;                     /*!< Minor version                                      */
  uint32_t                 areaLen;                          /*!< Area Length for NDEF storage                       */
  uint32_t                 areaAvalableSpaceLen;             /*!< Remaining Space in case a propTLV is present       */
  uint32_t                 messageLen;                       /*!< NDEF message Length                                */
  ndefState                state;                            /*!< Tag state e.g. NDEF_STATE_INITIALIZED              */
} ndefInfo;

#if NDEF_FEATURE_T1T
/*! T1T Capability Container  */
typedef struct {
  uint8_t                  magicNumber;                      /*!< Magic number e.g. E1h                              */
  uint8_t                  majorVersion;                     /*!< Major version i.e. 1                               */
  uint8_t                  minorVersion;                     /*!< Minor version i.e. 2                               */
  uint16_t                 tagMemorySize;                    /*!< Tag Memory Size (TMS)                              */
  uint8_t                  readAccess;                       /*!< NDEF READ access condition                         */
  uint8_t                  writeAccess;                      /*!< NDEF WRITE access condition                        */
} ndefCapabilityContainerT1T;
#endif

#if NDEF_FEATURE_T2T
/*! T2T Capability Container  */
typedef struct {
  uint8_t                  magicNumber;                      /*!< Magic number e.g. E1h                              */
  uint8_t                  majorVersion;                     /*!< Major version i.e. 1                               */
  uint8_t                  minorVersion;                     /*!< Minor version i.e. 2                               */
  uint8_t                  size;                             /*!< Size. T2T_Area_Size = Size * 8                     */
  uint8_t                  readAccess;                       /*!< NDEF READ access condition                         */
  uint8_t                  writeAccess;                      /*!< NDEF WRITE access condition                        */
} ndefCapabilityContainerT2T;
#endif

#if NDEF_FEATURE_T3T
/*! T3T Attribute info block  */
typedef struct {
  uint8_t                  majorVersion;                     /*!< Major version i.e. 1                               */
  uint8_t                  minorVersion;                     /*!< Minor version i.e. 2                               */
  uint8_t                  nbR;                              /*!< Nbr: number of blocks read in one CHECK cmd        */
  uint8_t                  nbW;                              /*!< Nbr: number of blocks written in one UPDATE cmd    */
  uint16_t                 nMaxB;                            /*!< NmaxB: max number of blocks for NDEF data          */
  uint8_t                  writeFlag;                        /*!< WriteFlag indicates completion of previous NDEF    */
  uint8_t                  rwFlag;                           /*!< RWFlag indicates whether the NDEF can be updated   */
  uint32_t                 Ln;                               /*!< Ln size of the actual stored NDEF data in bytes    */
} ndefAttribInfoBlockT3T;
#endif

#if NDEF_FEATURE_T4T
/*! T4T Capability Container  */
typedef struct {
  uint16_t                 ccLen;                            /*!< CCFILE Length                                      */
  uint8_t                  vNo;                              /*!< Mapping version                                    */
  uint16_t                 mLe;                              /*!< Max data size that can be read using a ReadBinary  */
  uint16_t                 mLc;                              /*!< Max data size that can be sent using a single cmd  */
  uint8_t                  fileId[2];                        /*!< NDEF File Identifier                               */
  uint32_t                 fileSize;                         /*!< NDEF File Size                                     */
  uint8_t                  readAccess;                       /*!< NDEF File READ access condition                    */
  uint8_t                  writeAccess;                      /*!< NDEF File WRITE access condition                   */
} ndefCapabilityContainerT4T;
#endif

#if NDEF_FEATURE_T5T
/*! T5T Capability Container  */
typedef struct {
  uint8_t                  ccLen;                            /*!< CC Length                                          */
  uint8_t                  magicNumber;                      /*!< Magic number i.e. E1h or E2h                       */
  uint8_t                  majorVersion;                     /*!< Major version i.e. 1                               */
  uint8_t                  minorVersion;                     /*!< Minor version i.e. 0                               */
  uint8_t                  readAccess;                       /*!< NDEF READ access condition                         */
  uint8_t                  writeAccess;                      /*!< NDEF WRITE access condition                        */
  uint16_t                 memoryLen;                        /*!< MLEN (Memory Len). T5T_Area size = 8 * MLEN (bytes)*/
  bool                     specialFrame;                     /*!< Use Special Frames for Write-alike commands        */
  bool                     lockBlock;                        /*!< (EXTENDED_)LOCK_SINGLE_BLOCK supported             */
  bool                     mlenOverflow;                     /*!< memory size exceeds 2040 bytes (Android)           */
  bool                     multipleBlockRead;                /*!< (EXTENDED_)READ_MULTIPLE_BLOCK supported           */
} ndefCapabilityContainerT5T;
#endif

/*! Generic Capability Container  */
typedef union {
#if NDEF_FEATURE_T1T
  ndefCapabilityContainerT1T   t1t;                          /*!< T1T Capability Container                           */
#endif
#if NDEF_FEATURE_T2T
  ndefCapabilityContainerT2T   t2t;                          /*!< T2T Capability Container                           */
#endif
#if NDEF_FEATURE_T3T
  ndefAttribInfoBlockT3T       t3t;                          /*!< T3T Attribute Information Block                    */
#endif
#if NDEF_FEATURE_T4T
  ndefCapabilityContainerT4T   t4t;                          /*!< T4T Capability Container                           */
#endif
#if NDEF_FEATURE_T5T
  ndefCapabilityContainerT5T   t5t;                          /*!< T5T Capability Container                           */
#endif
} ndefCapabilityContainer;

#if NDEF_FEATURE_T1T
/*! NDEF T1T sub context structure */
typedef struct {
  void *rfu;                                                 /*!< RFU                                                */
} ndefT1TContext;
#endif

#if NDEF_FEATURE_T2T
/*! NDEF T2T sub context structure */
typedef struct {
  uint8_t                      currentSecNo;                                   /*!< Current sector number                          */
  uint8_t                      cacheBuf[NDEF_T2T_READ_RESP_SIZE];              /*!< Cache buffer                                   */
  uint8_t                      nbrRsvdAreas;                                   /*!< Number of reserved Areas                        */
  uint16_t                     dynLockNbrLockBits;                             /*!< Number of bits inside the DynLock_Area         */
  uint16_t                     dynLockBytesLockedPerBit;                       /*!< Number of bytes locked by one Dynamic Lock bit */
  uint16_t                     dynLockNbrBytes;                                /*!< Number of bytes inside the DynLock_Area        */
  uint16_t                     rsvdAreaSize[NDEF_T2T_MAX_RSVD_AREAS];          /*!< Sizes of reserved areas                        */
  uint32_t                     cacheAddr;                                      /*!< Address of cached data                         */
  uint32_t                     offsetNdefTLV;                                  /*!< NDEF TLV message offset                        */
  uint32_t                     dynLockFirstByteAddr;                           /*!< Address of the first byte of the DynLock_Area  */
  uint32_t                     rsvdAreaFirstByteAddr[NDEF_T2T_MAX_RSVD_AREAS]; /*!< Addresses of reserved areas                    */
} ndefT2TContext;
#endif

#if NDEF_FEATURE_T3T
/*! NDEF T3T sub context structure */
typedef struct {
  uint8_t                      NFCID2[RFAL_NFCF_NFCID2_LEN];        /*!< NFCID2                                                  */
  uint8_t                      txbuf[NDEF_T3T_MAX_TX_SIZE];         /*!< Tx buffer dedicated for T3T internal operations         */
  uint8_t                      rxbuf[NDEF_T3T_MAX_RX_SIZE];         /*!< Rx buffer dedicated for T3T internal operations         */
  rfalNfcfBlockListElem        listBlocks[NDEF_T3T_MAX_NB_BLOCKS];  /*!< block number list for T3T internal operations           */
} ndefT3TContext;
#endif

#if NDEF_FEATURE_T4T
/*! NDEF T4T sub context structure */
typedef struct {
  uint8_t                      curMLe;                       /*!< Current MLe. Default Fh until CC file is read      */
  uint8_t                      curMLc;                       /*!< Current MLc. Default Dh until CC file is read      */
  bool                         mv1Flag;                      /*!< Mapping version 1 flag                             */
  rfalIsoDepApduBufFormat      cApduBuf;                     /*!< Command-APDU buffer                                */
  rfalIsoDepApduBufFormat      rApduBuf;                     /*!< Response-APDU buffer                               */
  rfalT4tRApduParam            respAPDU;                     /*!< Response-APDU params                               */
  rfalIsoDepBufFormat          tmpBuf;                       /*!< I-Block temporary buffer                           */
  uint16_t                     rApduBodyLen;                 /*!< Response Body Length                               */
  uint32_t                     FWT;                          /*!< Frame Waiting Time (1/fc)                          */
  uint32_t                     dFWT;                         /*!< Delta Frame Waiting Time (1/fc)                    */
  uint16_t                     FSx;                          /*!< Frame Size Device/Card (FSD or FSC)                */
  uint8_t                      DID;                          /*!< Device ID                                          */
} ndefT4TContext;
#endif

#if NDEF_FEATURE_T5T
/*! NFCV (Extended) System Information  */
typedef struct {
  uint16_t    numberOfBlock;                    /*!< Number of block                                    */
  uint8_t     UID[NDEF_NFCV_UID_LEN];           /*!< UID Value                                          */
  uint8_t     supportedCmd[NDEF_NFCV_SUPPORTED_CMD_LEN];/*!< Supported Commands list                    */
  uint8_t     infoFlags;                        /*!< Information flags                                  */
  uint8_t     DFSID;                            /*!< DFSID Value                                        */
  uint8_t     AFI;                              /*!< AFI Value                                          */
  uint8_t     blockSize;                        /*!< Block Size Value                                   */
  uint8_t     ICRef;                            /*!< IC Reference                                       */
} ndefSystemInformation;

/*! NDEF T5T sub context structure */
typedef struct {
  const uint8_t               *uid;                          /*!< UID in Addressed mode, NULL: Non-addr/Selected mode*/
  uint8_t                      flags;                        /*!< Command flags                                      */
  uint32_t                     TlvNDEFOffset;                /*!< NDEF TLV message offset                            */
  uint8_t                      blockLen;                     /*!< T5T BLEN parameter                                 */
  ndefSystemInformation        sysInfo;                      /*!< System Information (when supported)                */
  bool                         sysInfoSupported;             /*!< System Information Supported flag                  */
  bool                         legacySTHighDensity;          /*!< Legacy ST High Density flag                        */
  uint8_t                      txrxBuf[NDEF_T5T_TxRx_BUFF_SIZE];/*!< Tx Rx Buffer                                    */
  uint8_t                      cacheBuf[NDEF_T5T_TxRx_BUFF_SIZE];/*!< Cache buffer                                   */
  uint32_t                     cacheBlock;                   /*!< Block number of cached buffer                      */
  bool                         useMultipleBlockRead;         /*!< Access multiple block read                         */
  bool                         stDevice;                     /*!< ST device                                          */
} ndefT5TContext;
#endif

/*! NDEF context structure */
typedef struct {
  ndefDeviceType               type;                         /*!< NDEF Device type                                   */
  ndefDevice                   device;                       /*!< NDEF Device                                        */
  ndefState                    state;                        /*!< Tag state e.g. NDEF_STATE_INITIALIZED              */
  ndefCapabilityContainer      cc;                           /*!< Capability Container                               */
  uint32_t                     messageLen;                   /*!< NDEF message length                                */
  uint32_t                     messageOffset;                /*!< NDEF message offset                                */
  uint32_t                     areaLen;                      /*!< Area Length for NDEF storage                       */
  uint8_t                      ccBuf[NDEF_CC_BUF_LEN];       /*!< buffer for CC                                      */
  const struct ndefPollerWrapperStruct *
    ndefPollWrapper;              /*!< pointer to array of function for wrapper           */
  union {
#if NDEF_FEATURE_T1T
    ndefT1TContext t1t;                                    /*!< T1T context                                        */
#endif
#if NDEF_FEATURE_T2T
    ndefT2TContext t2t;                                    /*!< T2T context                                        */
#endif
#if NDEF_FEATURE_T3T
    ndefT3TContext t3t;                                    /*!< T3T context                                        */
#endif
#if NDEF_FEATURE_T4T
    ndefT4TContext t4t;                                    /*!< T4T context                                        */
#endif
#if NDEF_FEATURE_T5T
    ndefT5TContext t5t;                                    /*!< T5T context                                        */
#endif
  } subCtx;                                                  /*!< Sub-context union                                  */

  void                        *ndef_class_instance;
} ndefContext;

/*! Wrapper structure to hold the function pointers on each tag type */
typedef struct ndefPollerWrapperStruct {
  ReturnCode(* pollerContextInitialization)(ndefContext *ctx, const ndefDevice *dev);                                                   /*!< ContextInitialization function pointer                 */
  ReturnCode(* pollerNdefDetect)(ndefContext *ctx, ndefInfo *info);                                                                     /*!< NdefDetect function pointer                            */
  ReturnCode(* pollerReadBytes)(ndefContext *ctx, uint32_t offset, uint32_t len, uint8_t *buf, uint32_t *rcvdLen);                      /*!< Read function pointer                                  */
  ReturnCode(* pollerReadRawMessage)(ndefContext *ctx, uint8_t *buf, uint32_t bufLen, uint32_t *rcvdLen, bool single);                  /*!< ReadRawMessage function pointer                        */
#if NDEF_FEATURE_FULL_API
  ReturnCode(* pollerWriteBytes)(ndefContext *ctx, uint32_t offset, const uint8_t *buf, uint32_t len, bool pad, bool writeTerminator);  /*!< Write function pointer                                 */
  ReturnCode(* pollerWriteRawMessage)(ndefContext *ctx, const uint8_t *buf, uint32_t bufLen);                                           /*!< WriteRawMessage function pointer                       */
  ReturnCode(* pollerTagFormat)(ndefContext *ctx, const ndefCapabilityContainer *cc, uint32_t options);                                 /*!< TagFormat function pointer                             */
  ReturnCode(* pollerWriteRawMessageLen)(ndefContext *ctx, uint32_t rawMessageLen, bool writeTerminator);                               /*!< WriteRawMessageLen function pointer                    */
  ReturnCode(* pollerCheckPresence)(ndefContext *ctx);                                                                                  /*!< CheckPresence function pointer                         */
  ReturnCode(* pollerCheckAvailableSpace)(const ndefContext *ctx, uint32_t messageLen);                                                 /*!< CheckAvailableSpace function pointer                   */
  ReturnCode(* pollerBeginWriteMessage)(ndefContext *ctx, uint32_t messageLen);                                                         /*!< BeginWriteMessage function pointer                     */
  ReturnCode(* pollerEndWriteMessage)(ndefContext *ctx, uint32_t messageLen, bool writeTerminator);                                     /*!< EndWriteMessage function pointer                       */
  ReturnCode(* pollerSetReadOnly)(ndefContext *ctx);                                                                                    /*!< SetReadOnly function pointer                           */
#endif /* NDEF_FEATURE_FULL_API */
} ndefPollerWrapper;


/*
 ******************************************************************************
 * GLOBAL FUNCTION PROTOTYPES
 ******************************************************************************
 */


/*!
 *****************************************************************************
 * \brief Return the device type
 *
 * This function returns the device type from the context
 *
 * \param[in] dev: ndef Device
 *
 * \return the device type
 *****************************************************************************
 */
ndefDeviceType ndefGetDeviceType(const ndefDevice *dev);


/*!
 *****************************************************************************
 * \brief Handle NDEF context activation
 *
 * This method performs the initialization of the NDEF context.
 * It must be called after a successful
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
ReturnCode ndefPollerContextInitialization(ndefContext *ctx, const ndefDevice *dev);


/*!
 *****************************************************************************
 * \brief NDEF Detection procedure
 *
 * This method performs the NDEF Detection procedure
 *
 * \param[in]   ctx    : ndef Context
 * \param[out]  info   : ndef Information (optional parameter, NULL may be used when no NDEF Information is needed)
 *
 * \return ERR_WRONG_STATE  : Library not initialized or mode not set
 * \return ERR_REQUEST      : Detection failed
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_PROTO        : Protocol error
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode ndefPollerNdefDetect(ndefContext *ctx, ndefInfo *info);


/*!
 *****************************************************************************
 * \brief Read data
 *
 * This method reads arbitrary length data
 *
 * \param[in]   ctx    : ndef Context
 * \param[in]   offset : file offset of where to start reading data
 * \param[in]   len    : requested length
 * \param[out]  buf    : buffer to place the data read from the tag
 * \param[out]  rcvdLen: received length
 *
 * \return ERR_WRONG_STATE  : Library not initialized or mode not set
 * \return ERR_REQUEST      : read failed
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_PROTO        : Protocol error
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode ndefPollerReadBytes(ndefContext *ctx, uint32_t offset, uint32_t len, uint8_t *buf, uint32_t *rcvdLen);


/*!
 *****************************************************************************
 * \brief  Write data
 *
 * This method writes arbitrary length data from the current selected file
 *
 * \param[in]   ctx    : ndef Context
 * \param[in]   offset : file offset of where to start writing data
 * \param[in]   buf    : data to write
 * \param[in]   len    : buf length
 *
 * \return ERR_WRONG_STATE  : Library not initialized or mode not set
 * \return ERR_REQUEST      : read failed
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_PROTO        : Protocol error
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode ndefPollerWriteBytes(ndefContext *ctx, uint32_t offset, const uint8_t *buf, uint32_t len);


/*!
 *****************************************************************************
 * \brief Read raw NDEF message
 *
 * This method reads a raw NDEF message.
 * Prior to NDEF Read procedure, a successful ndefPollerNdefDetect()
 * has to be performed.
 *
 *
 * \param[in]   ctx    : ndef Context
 * \param[out]  buf    : buffer to place the NDEF message
 * \param[in]   bufLen : buffer length
 * \param[out]  rcvdLen: received length
 * \param[in]   single : performs the procedure as part of a single NDEF read operation. "true" can be used when migrating from previous version of this API as only SINGLE NDEF READ was supported. "false" can be used to force the reading of the NDEF length (e.g. for TNEP).
 *
 * \return ERR_WRONG_STATE  : Library not initialized or mode not set
 * \return ERR_REQUEST      : read failed
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_PROTO        : Protocol error
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode ndefPollerReadRawMessage(ndefContext *ctx, uint8_t *buf, uint32_t bufLen, uint32_t *rcvdLen, bool single);


/*!
 *****************************************************************************
 * \brief Write raw NDEF message
 *
 * This method writes a raw NDEF message.
 * Prior to NDEF Write procedure, a successful ndefPollerNdefDetect()
 * has to be performed.
 *
 * \param[in]   ctx    : ndef Context
 * \param[in]   buf    : raw message buffer
 * \param[in]   bufLen : buffer length
 *
 * \return ERR_WRONG_STATE  : Library not initialized or mode not set
 * \return ERR_REQUEST      : write failed
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_PROTO        : Protocol error
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode ndefPollerWriteRawMessage(ndefContext *ctx, const uint8_t *buf, uint32_t bufLen);


/*!
 *****************************************************************************
 * \brief Format Tag
 *
 * This method formats a tag to make it ready for NDEF storage.
 * cc and options parameters usage is described in each technology method
 * (ndefT[2345]TPollerTagFormat)
 *
 * \param[in]   ctx     : ndef Context
 * \param[in]   cc      : Capability Container
 * \param[in]   options : specific flags
 *
 * \return ERR_WRONG_STATE  : Library not initialized or mode not set
 * \return ERR_REQUEST      : write failed
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_PROTO        : Protocol error
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode ndefPollerTagFormat(ndefContext *ctx, const ndefCapabilityContainer *cc, uint32_t options);


/*!
 *****************************************************************************
 * \brief Write NDEF message length
 *
 * This method writes the NLEN field
 *
 * \param[in]   ctx          : ndef Context
 * \param[in]   rawMessageLen: len
 *
 * \return ERR_WRONG_STATE  : Library not initialized or mode not set
 * \return ERR_REQUEST      : write failed
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_PROTO        : Protocol error
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode ndefPollerWriteRawMessageLen(ndefContext *ctx, uint32_t rawMessageLen);


/*!
*****************************************************************************
* \brief Write an NDEF message
*
* Write the NDEF message to the tag
*
* \param[in] ctx:     NDEF Context
* \param[in] message: Message to write
*
* \return ERR_WRONG_STATE  : Library not initialized or mode not set
* \return ERR_REQUEST      : write failed
* \return ERR_PARAM        : Invalid parameter
* \return ERR_PROTO        : Protocol error
* \return ERR_NONE         : No error
*****************************************************************************
*/
ReturnCode ndefPollerWriteMessage(ndefContext *ctx, const ndefMessage *message);


/*!
 *****************************************************************************
 * \brief Check Presence
 *
 * This method checks whether an NFC tag is still present in the operating field
 *
 * \param[in]   ctx    : ndef Context

 *
 * \return ERR_WRONG_STATE  : Library not initialized or mode not set
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_PROTO        : Protocol error
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode ndefPollerCheckPresence(ndefContext *ctx);


/*!
 *****************************************************************************
 * \brief Check Available Space
 *
 * This method checks whether a NFC tag has enough space to write a message of a given length
 *
 * \param[in]   ctx       : ndef Context
 * \param[in]   messageLen: message length
 *
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_NOMEM        : not enough space
 * \return ERR_NONE         : Enough space for message of messageLen length
 *****************************************************************************
 */
ReturnCode ndefPollerCheckAvailableSpace(const ndefContext *ctx, uint32_t messageLen);


/*!
 *****************************************************************************
 * \brief Begin Write Message
 *
 * This method sets the L-field to 0 (T1T, T2T, T4T, T5T) or set the WriteFlag (T3T) and sets the message offset to the proper value according to messageLen
 *
 * \param[in]   ctx       : ndef Context
 * \param[in]   messageLen: message length
 *
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_NOMEM        : not enough space
 * \return ERR_NONE         : Enough space for message of messageLen length
 *****************************************************************************
 */
ReturnCode ndefPollerBeginWriteMessage(ndefContext *ctx, uint32_t messageLen);


/*!
 *****************************************************************************
 * \brief End Write Message
 *
 * This method updates the L-field value after the message has been written and resets the WriteFlag (for T3T only)
 *
 * \param[in]   ctx       : ndef Context
 * \param[in]   messageLen: message length
 *
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_NOMEM        : not enough space
 * \return ERR_NONE         : Enough space for message of messageLen length
 *****************************************************************************
 */
ReturnCode ndefPollerEndWriteMessage(ndefContext *ctx, uint32_t messageLen);


/*!
 *****************************************************************************
 * \brief Set Read Only
 *
 * This method performs the transition from the READ/WRITE state to the READ-ONLY state
 *
 * \param[in]   ctx       : ndef Context
 *
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode ndefPollerSetReadOnly(ndefContext *ctx);



#endif /* NDEF_POLLER_H */

/**
  * @}
  *
  */
