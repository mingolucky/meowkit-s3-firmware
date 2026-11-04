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
 *  \brief Implementation of ISO-DEP protocol
 *
 *  This implementation was based on the following specs:
 *    - ISO/IEC 14443-4  2nd Edition 2008-07-15
 *    - NFC Forum Digital Protocol  1.1 2014-01-14
 *
 *
 * \addtogroup RFAL
 * @{
 *
 * \addtogroup RFAL-AL
 * \brief RFAL Abstraction Layer
 * @{
 *
 * \addtogroup ISO-DEP
 * \brief RFAL ISO-DEP Module
 * @{
 *
 */

#ifndef RFAL_ISODEP_H_
#define RFAL_ISODEP_H_
/*
 ******************************************************************************
 * INCLUDES
 ******************************************************************************
 */
#include "rfal_nfc.h"
#include "rfal_nfcb.h"
/*
 ******************************************************************************
 * ENABLE SWITCH
 ******************************************************************************
 */

/* If module is disabled remove the need for the user to set lengths */
#if !RFAL_FEATURE_ISO_DEP
  #undef RFAL_FEATURE_ISO_DEP_IBLOCK_MAX_LEN
  #undef RFAL_FEATURE_ISO_DEP_APDU_MAX_LEN

  #define RFAL_FEATURE_ISO_DEP_IBLOCK_MAX_LEN  (1U)    /*!< ISO-DEP I-Block max length, set to "none" */
  #define RFAL_FEATURE_ISO_DEP_APDU_MAX_LEN    (1U)    /*!< ISO-DEP APDU max length, set to "none"    */
#endif /* !RFAL_FEATURE_ISO_DEP */

/*
 ******************************************************************************
 * DEFINES
 ******************************************************************************
 */

#define RFAL_ISODEP_PROLOGUE_SIZE               (3U)     /*!< Length of Prologue Field for I-Block Format                       */

#define RFAL_ISODEP_PCB_LEN                     (1U)     /*!< PCB length                                                        */
#define RFAL_ISODEP_DID_LEN                     (1U)     /*!< DID length                                                        */
#define RFAL_ISODEP_NAD_LEN                     (1U)     /*!< NAD length                                                        */
#define RFAL_ISODEP_NO_DID                      (0x10U)  /*!< DID value indicating the ISO-DEP layer not to use DID             */
#define RFAL_ISODEP_NO_NAD                      (0xFFU)  /*!< NAD value indicating the ISO-DEP layer not to use NAD             */

#define RFAL_ISODEP_FWI_MASK                    (0xF0U)  /*!< Mask bits of FWI                                                  */
#define RFAL_ISODEP_FWI_SHIFT                   (4U)     /*!< Shift val of FWI                                                  */
#define RFAL_ISODEP_FWI_DEFAULT                 (4U)     /*!< Default value for FWI Digital 1.0 11.6.2.17                       */
#define RFAL_ISODEP_ADV_FEATURE                 (0x0FU)  /*!< Indicate 256 Bytes FSD and Advanc Proto Feature support:NAD & DID */

#define RFAL_ISODEP_DID_MAX                     (14U)    /*!< Maximum DID value                                                 */

#define RFAL_ISODEP_BRI_MASK                    (0x07U)  /*!< Mask bits for Poll to Listen Send bitrate                         */
#define RFAL_ISODEP_BSI_MASK                    (0x70U)  /*!< Mask bits for Listen to Poll Send bitrate                         */
#define RFAL_ISODEP_SAME_BITRATE_MASK           (0x80U)  /*!< Mask bit indicate only same bit rate D for both direction support */
#define RFAL_ISODEP_BITRATE_RFU_MASK            (0x08U)  /*!< Mask bit for RFU                                                  */

/*! Maximum Frame Waiting Time = ((256 * 16/fc) * 2^FWImax) = ((256*16/fc)*2^14) = (67108864)/fc = 2^26 (1/fc)                  */
#define RFAL_ISODEP_MAX_FWT                     ((uint32_t)1U<<26)
#define ISODEP_SWTX_PARAM_LEN (1U) /*!< SWTX parameter length         */
/*! Maximum length of control message (no INF) */
#define ISODEP_CONTROLMSG_BUF_LEN (RFAL_ISODEP_PCB_LEN + RFAL_ISODEP_DID_LEN + RFAL_ISODEP_NAD_LEN + ISODEP_SWTX_PARAM_LEN)

#define RFAL_ISODEP_FSDI_DEFAULT                RFAL_ISODEP_FSXI_256  /*!< Default Frame Size Integer in Poll mode              */
#define RFAL_ISODEP_FSX_KEEP                    (0xFFU)               /*!< Flag to keep FSX from activation                     */
#define RFAL_ISODEP_DEFAULT_FSCI                RFAL_ISODEP_FSXI_256  /*!< FSCI default value to be used  in Listen Mode        */
#define RFAL_ISODEP_DEFAULT_FSC                 RFAL_ISODEP_FSX_256   /*!< FSC default value (aligned RFAL_ISODEP_DEFAULT_FSCI) */
#define RFAL_ISODEP_DEFAULT_SFGI                (0U)                  /*!< SFGI Default value to be used  in Listen Mode        */
#define RFAL_ISODEP_DEFAULT_FWI                 (8U)                  /*!< Default Listener FWI (Max)      Digital 2.0  B7 & B3 */

#define RFAL_ISODEP_APDU_MAX_LEN                RFAL_ISODEP_FSX_1024  /*!< Max APDU length                                      */

#define RFAL_ISODEP_ATTRIB_RES_MBLI_NO_INFO     (0x00U)  /*!< MBLI indicating no information on its internal input buffer size  */
#define RFAL_ISODEP_ATTRIB_REQ_PARAM1_DEFAULT   (0x00U)  /*!< Default values of Param 1 of ATTRIB_REQ Digital 1.0  12.6.1.3-5   */
#define RFAL_ISODEP_ATTRIB_HLINFO_LEN           (32U)    /*!< Maximum Size of Higher Layer Information                          */
#define RFAL_ISODEP_ATS_HB_MAX_LEN              (15U)    /*!< Maximum length of Historical Bytes  Digital 1.1  13.6.2.23        */
#define RFAL_ISODEP_ATTRIB_REQ_MIN_LEN          (9U)     /*!< Minimum Length of ATTRIB_REQ command                              */
#define RFAL_ISODEP_ATTRIB_RES_MIN_LEN          (1U)     /*!< Minimum Length of ATTRIB_RES response                             */

#define RFAL_ISODEP_SPARAM_VALUES_MAX_LEN       (16U)    /*!< Maximum Length of the value field on S(PARAMETERS)                */
#define RFAL_ISODEP_SPARAM_TAG_BLOCKINFO        (0xA0U)  /*!< S(PARAMETERS) tag Block information                               */
#define RFAL_ISODEP_SPARAM_TAG_BRREQ            (0xA1U)  /*!< S(PARAMETERS) tag Bit rates Request                               */
#define RFAL_ISODEP_SPARAM_TAG_BRIND            (0xA2U)  /*!< S(PARAMETERS) tag Bit rates Indication                            */
#define RFAL_ISODEP_SPARAM_TAG_BRACT            (0xA3U)  /*!< S(PARAMETERS) tag Bit rates Activation                            */
#define RFAL_ISODEP_SPARAM_TAG_BRACK            (0xA4U)  /*!< S(PARAMETERS) tag Bit rates Acknowledgement                       */

#define RFAL_ISODEP_SPARAM_TAG_SUP_PCD2PICC     (0x80U)  /*!< S(PARAMETERS) tag Supported bit rates from PCD to PICC            */
#define RFAL_ISODEP_SPARAM_TAG_SUP_PICC2PCD     (0x81U)  /*!< S(PARAMETERS) tag Supported bit rates from PICC to PCD            */
#define RFAL_ISODEP_SPARAM_TAG_SUP_FRAME        (0x82U)  /*!< S(PARAMETERS) tag Supported framing options PICC to PCD           */
#define RFAL_ISODEP_SPARAM_TAG_SEL_PCD2PICC     (0x83U)  /*!< S(PARAMETERS) tag Selected bit rate from PCD to PICC              */
#define RFAL_ISODEP_SPARAM_TAG_SEL_PICC2PCD     (0x84U)  /*!< S(PARAMETERS) tag Selected bit rate from PICC to PCD              */
#define RFAL_ISODEP_SPARAM_TAG_SEL_FRAME        (0x85U)  /*!< S(PARAMETERS) tag Selected framing options PICC to PCD            */

#define RFAL_ISODEP_SPARAM_TAG_LEN              (1)      /*!< S(PARAMETERS) Tag Length                                          */
#define RFAL_ISODEP_SPARAM_TAG_BRREQ_LEN        (0U)     /*!< S(PARAMETERS) tag Bit rates Request Length                        */
#define RFAL_ISODEP_SPARAM_TAG_PICC2PCD_LEN     (2U)     /*!< S(PARAMETERS) bit rates from PCD to PICC Length                   */
#define RFAL_ISODEP_SPARAM_TAG_PCD2PICC_LEN     (2U)     /*!< S(PARAMETERS) bit rates from PICC to PCD Length                   */
#define RFAL_ISODEP_SPARAM_TAG_BRACK_LEN        (0U)     /*!< S(PARAMETERS) tag Bit rates Acknowledgement Length                */

#define RFAL_ISODEP_ATS_TA_DPL_212              (0x01U)  /*!< ATS TA DSI 212 kbps support bit mask                              */
#define RFAL_ISODEP_ATS_TA_DPL_424              (0x02U)  /*!< ATS TA DSI 424 kbps support bit mask                              */
#define RFAL_ISODEP_ATS_TA_DPL_848              (0x04U)  /*!< ATS TA DSI 848 kbps support bit mask                              */
#define RFAL_ISODEP_ATS_TA_DLP_212              (0x10U)  /*!< ATS TA DSI 212 kbps support bit mask                              */
#define RFAL_ISODEP_ATS_TA_DLP_424              (0x20U)  /*!< ATS TA DRI 424 kbps support bit mask                              */
#define RFAL_ISODEP_ATS_TA_DLP_848              (0x40U)  /*!< ATS TA DRI 848 kbps support bit mask                              */
#define RFAL_ISODEP_ATS_TA_SAME_D               (0x80U)  /*!< ATS TA same bit both directions bit mask                          */
#define RFAL_ISODEP_ATS_TB_FWI_MASK             (0xF0U)  /*!< Mask bits for FWI (Frame Waiting Integer) in TB byte              */
#define RFAL_ISODEP_ATS_TB_SFGI_MASK            (0x0FU)  /*!< Mask bits for SFGI (Start-Up Frame Guard Integer) in TB byte      */

#define RFAL_ISODEP_ATS_T0_TA_PRESENCE_MASK     (0x10U)  /*!< Mask bit for TA presence                                          */
#define RFAL_ISODEP_ATS_T0_TB_PRESENCE_MASK     (0x20U)  /*!< Mask bit for TB presence                                          */
#define RFAL_ISODEP_ATS_T0_TC_PRESENCE_MASK     (0x40U)  /*!< Mask bit for TC presence                                          */
#define RFAL_ISODEP_ATS_T0_FSCI_MASK            (0x0FU)  /*!< Mask bit for FSCI presence                                        */
#define RFAL_ISODEP_ATS_T0_OFFSET               (0x01U)  /*!< Offset of T0 in ATS Response                                      */


#define RFAL_ISODEP_MAX_I_RETRYS                (2U)     /*!< Number of retries for a I-Block     Digital 2.0   16.2.5.4                  */
#define RFAL_ISODEP_MAX_R_RETRYS                (3U)     /*!< Number of retries for a R-Block     Digital 2.0 B9 - nRETRY ACK/NAK: [2,5]  */
#define RFAL_ISODEP_MAX_WTX_NACK_RETRYS         (3U)     /*!< Number of S(WTX) replied with NACK  Digital 2.0 B9 - nRETRY WTX[2,5]        */
#define RFAL_ISODEP_MAX_WTX_RETRYS              (20U)    /*!< Number of overall S(WTX) retries    Digital 2.0  16.2.5.2                   */
#define RFAL_ISODEP_MAX_WTX_RETRYS_ULTD         (255U)   /*!< Use unlimited number of overall S(WTX)                                      */
#define RFAL_ISODEP_MAX_DSL_RETRYS              (0U)     /*!< Number of retries for a S(DESELECT) Digital 2.0 B9 - nRETRY DESELECT: [0,5] */
#define RFAL_ISODEP_RATS_RETRIES                (1U)     /*!< RATS retries upon fail              Digital 2.0 B7 - nRETRY RATS [0,1]      */




/*! Frame Size for Proximity Card Integer definitions                                                               */
typedef enum {
  RFAL_ISODEP_FSXI_16   =  0,  /*!< Frame Size for Proximity Card Integer with 16 bytes                         */
  RFAL_ISODEP_FSXI_24   =  1,  /*!< Frame Size for Proximity Card Integer with 24 bytes                         */
  RFAL_ISODEP_FSXI_32   =  2,  /*!< Frame Size for Proximity Card Integer with 32 bytes                         */
  RFAL_ISODEP_FSXI_40   =  3,  /*!< Frame Size for Proximity Card Integer with 40 bytes                         */
  RFAL_ISODEP_FSXI_48   =  4,  /*!< Frame Size for Proximity Card Integer with 48 bytes                         */
  RFAL_ISODEP_FSXI_64   =  5,  /*!< Frame Size for Proximity Card Integer with 64 bytes                         */
  RFAL_ISODEP_FSXI_96   =  6,  /*!< Frame Size for Proximity Card Integer with 96 bytes                         */
  RFAL_ISODEP_FSXI_128  =  7,  /*!< Frame Size for Proximity Card Integer with 128 bytes                        */
  RFAL_ISODEP_FSXI_256  =  8,  /*!< Frame Size for Proximity Card Integer with 256 bytes                        */
  RFAL_ISODEP_FSXI_512  =  9,  /*!< Frame Size for Proximity Card Integer with 512 bytes   ISO14443-3 Amd2 2012 */
  RFAL_ISODEP_FSXI_1024 = 10,  /*!< Frame Size for Proximity Card Integer with 1024 bytes  ISO14443-3 Amd2 2012 */
  RFAL_ISODEP_FSXI_2048 = 11,  /*!< Frame Size for Proximity Card Integer with 2048 bytes  ISO14443-3 Amd2 2012 */
  RFAL_ISODEP_FSXI_4096 = 12   /*!< Frame Size for Proximity Card Integer with 4096 bytes  ISO14443-3 Amd2 2012 */
} rfalIsoDepFSxI;

/*! Frame Size for Proximity Card  definitions                                                             */
typedef enum {
  RFAL_ISODEP_FSX_16   = 16,    /*!< Frame Size for Proximity Card with 16 bytes                         */
  RFAL_ISODEP_FSX_24   = 24,    /*!< Frame Size for Proximity Card with 24 bytes                         */
  RFAL_ISODEP_FSX_32   = 32,    /*!< Frame Size for Proximity Card with 32 bytes                         */
  RFAL_ISODEP_FSX_40   = 40,    /*!< Frame Size for Proximity Card with 40 bytes                         */
  RFAL_ISODEP_FSX_48   = 48,    /*!< Frame Size for Proximity Card with 48 bytes                         */
  RFAL_ISODEP_FSX_64   = 64,    /*!< Frame Size for Proximity Card with 64 bytes                         */
  RFAL_ISODEP_FSX_96   = 96,    /*!< Frame Size for Proximity Card with 96 bytes                         */
  RFAL_ISODEP_FSX_128  = 128,   /*!< Frame Size for Proximity Card with 128 bytes                        */
  RFAL_ISODEP_FSX_256  = 256,   /*!< Frame Size for Proximity Card with 256 bytes                        */
  RFAL_ISODEP_FSX_512  = 512,   /*!< Frame Size for Proximity Card with 512 bytes   ISO14443-3 Amd2 2012 */
  RFAL_ISODEP_FSX_1024 = 1024,  /*!< Frame Size for Proximity Card with 1024 bytes  ISO14443-3 Amd2 2012 */
  RFAL_ISODEP_FSX_2048 = 2048,  /*!< Frame Size for Proximity Card with 2048 bytes  ISO14443-3 Amd2 2012 */
  RFAL_ISODEP_FSX_4096 = 4096,  /*!< Frame Size for Proximity Card with 4096 bytes  ISO14443-3 Amd2 2012 */
} rfalIsoDepFSx;

/*
 ******************************************************************************
 * GLOBAL MACROS
 ******************************************************************************
 */

/*
 ******************************************************************************
 * GLOBAL DATA TYPES
 ******************************************************************************
 */

/*! RATS format  Digital 1.1 13.6.1                                                               */
typedef struct {
  uint8_t      CMD;                               /*!< RATS command byte: 0xE0                  */
  uint8_t      PARAM;                             /*!< Param indicating FSDI and DID            */
} rfalIsoDepRats;


/*! ATS response format  Digital 1.1 13.6.2                                                       */
typedef struct {
  uint8_t        TL;                                /*!< Length Byte, including TL byte itself    */
  uint8_t        T0;                                /*!< Format Byte T0 indicating if TA, TB, TC  */
  uint8_t        TA;                                /*!< Interface Byte TA(1)                     */
  uint8_t        TB;                                /*!< Interface Byte TB(1)                     */
  uint8_t        TC;                                /*!< Interface Byte TC(1)                     */
  uint8_t        HB[RFAL_ISODEP_ATS_HB_MAX_LEN];    /*!< Historical Bytes                         */
} rfalIsoDepAts;


/*! PPS Request format (Protocol and Parameter Selection) ISO14443-4  5.3                         */
typedef struct {
  uint8_t      PPSS;                              /*!< Start Byte: [ 1101b | CID[4b] ]          */
  uint8_t      PPS0;                              /*!< Parameter 0:[ 000b | PPS1[1n] | 0001b ]  */
  uint8_t      PPS1;                              /*!< Parameter 1:[ 0000b | DSI[2b] | DRI[2b] ]*/
} rfalIsoDepPpsReq;


/*! PPS Response format (Protocol and Parameter Selection) ISO14443-4  5.4                        */
typedef struct {
  uint8_t      PPSS;                              /*!< Start Byte:  [ 1101b | CID[4b] ]         */
} rfalIsoDepPpsRes;


/*! ATTRIB Command Format  Digital 1.1  15.6.1 */
typedef struct {
  uint8_t         cmd;                                   /*!< ATTRIB_REQ command byte           */
  uint8_t         nfcid0[RFAL_NFCB_NFCID0_LEN];          /*!< NFCID0 of the card to be selected */
  struct {
    uint8_t PARAM1;                                /*!< PARAM1 of ATTRIB command          */
    uint8_t PARAM2;                                /*!< PARAM2 of ATTRIB command          */
    uint8_t PARAM3;                                /*!< PARAM3 of ATTRIB command          */
    uint8_t PARAM4;                                /*!< PARAM4 of ATTRIB command          */
  } Param;                                               /*!< Parameter of ATTRIB command       */
  uint8_t         HLInfo[RFAL_ISODEP_ATTRIB_HLINFO_LEN]; /*!< Higher Layer Information          */
} rfalIsoDepAttribCmd;


/*! ATTRIB Response Format  Digital 1.1  15.6.2 */
typedef struct {
  uint8_t         mbliDid;                               /*!< Contains MBLI and DID             */
  uint8_t         HLInfo[RFAL_ISODEP_ATTRIB_HLINFO_LEN]; /*!< Higher Layer Information          */
} rfalIsoDepAttribRes;

/*! S(Parameters) Command Format  ISO14443-4 (2016) Table 4 */
typedef struct {
  uint8_t         tag;                                      /*!< S(PARAMETERS) Tag field        */
  uint8_t         length;                                   /*!< S(PARAMETERS) Length field     */
  uint8_t         value[RFAL_ISODEP_SPARAM_VALUES_MAX_LEN]; /*!< S(PARAMETERS) Value field      */
} rfalIsoDepSParameter;


/*! Activation info as Poller and Listener for NFC-A and NFC-B                                    */
typedef union {/*  PRQA S 0750 # MISRA 19.2 - Both members of the union will not be used concurrently, device is only of type A or B at a time. Thus no problem can occur. */

  /*! NFC-A information                                                                         */
  union {
    struct {
      rfalIsoDepAts        ATS;               /*!< ATS response            (Poller mode)    */
      uint8_t              ATSLen;            /*!< ATS response length     (Poller mode)    */
    } Listener;
    struct {
      rfalIsoDepRats      RATS;               /*!< RATS request          (Listener mode)    */
    } Poller;
  } A;

  /*! NFC-B information                                                                         */
  union {
    struct {
      rfalIsoDepAttribRes  ATTRIB_RES;        /*!< ATTRIB_RES              (Poller mode)    */
      uint8_t              ATTRIB_RESLen;     /*!< ATTRIB_RES length       (Poller mode)    */
    } Listener;
    struct {
      rfalIsoDepAttribCmd  ATTRIB;            /*!< ATTRIB request        (Listener mode)    */
      uint8_t              ATTRIBLen;         /*!< ATTRIB request length (Listener mode)    */
    } Poller;
  } B;
} rfalIsoDepActivation;


/*! ISO-DEP device Info */
typedef struct {
  uint8_t            FWI;             /*!< Frame Waiting Integer                                */
  uint32_t           FWT;             /*!< Frame Waiting Time (1/fc)                            */
  uint32_t           dFWT;            /*!< Delta Frame Waiting Time (1/fc)                      */
  uint32_t           SFGI;            /*!< Start-up Frame Guard time Integer                    */
  uint32_t           SFGT;            /*!< Start-up Frame Guard Time (ms)                       */
  uint8_t            FSxI;            /*!< Frame Size Device/Card Integer (FSDI or FSCI)        */
  uint16_t           FSx;             /*!< Frame Size Device/Card (FSD or FSC)                  */
  uint32_t           MBL;             /*!< Maximum Buffer Length (optional for NFC-B)           */
  rfalBitRate        DSI;             /*!< Bit Rate coding from Listener (PICC) to Poller (PCD) */
  rfalBitRate        DRI;             /*!< Bit Rate coding from Poller (PCD) to Listener (PICC) */
  uint8_t            DID;             /*!< Device ID                                            */
  uint8_t            NAD;             /*!< Node ADdress                                         */
  bool               supDID;          /*!< DID supported flag                                   */
  bool               supNAD;          /*!< NAD supported flag                                   */
  bool               supAdFt;         /*!< Advanced Features supported flag                     */
} rfalIsoDepInfo;


/*! ISO-DEP Device structure */
typedef struct {
  rfalIsoDepActivation    activation; /*!< Activation Info                                      */
  rfalIsoDepInfo          info;       /*!< ISO-DEP (ISO14443-4) device Info                     */
} rfalIsoDepDevice;


/*! ATTRIB Response parameters */
typedef struct {
  uint8_t  mbli;                                     /*!< MBLI                                     */
  uint8_t  HLInfo[RFAL_ISODEP_ATTRIB_HLINFO_LEN];    /*!< Hi Layer Information                     */
  uint8_t  HLInfoLen;                                /*!< Hi Layer Information Length              */
} rfalIsoDepAttribResParam;


/*! ATS Response parameter */
typedef struct {
  uint8_t     fsci;                                  /*!< Frame Size of Proximity Card Integer     */
  uint8_t     fwi;                                   /*!< Frame Waiting Time Integer               */
  uint8_t     sfgi;                                  /*!< Start-Up Frame Guard Time Integer        */
  bool        didSupport;                            /*!< DID Supported                            */
  uint8_t     ta;                                    /*!< Max supported bitrate both direction     */
  uint8_t     *hb;                                   /*!< Historical Bytes data                    */
  uint8_t     hbLen;                                 /*!< Historical Bytes Length                  */
} rfalIsoDepAtsParam;


/*! Structure of I-Block Buffer format from caller */
typedef struct {
  uint8_t  prologue[RFAL_ISODEP_PROLOGUE_SIZE];      /*!< Prologue/SoD buffer                      */
  uint8_t  inf[RFAL_FEATURE_ISO_DEP_IBLOCK_MAX_LEN]; /*!< INF/Payload buffer                       */
} rfalIsoDepBufFormat;


/*! Structure of APDU Buffer format from caller */
typedef struct {
  uint8_t  prologue[RFAL_ISODEP_PROLOGUE_SIZE];      /*!< Prologue/SoD buffer                      */
  uint8_t  apdu[RFAL_FEATURE_ISO_DEP_APDU_MAX_LEN];  /*!< APDU/Payload buffer                      */
} rfalIsoDepApduBufFormat;


/*! Listen Activation Parameters Structure */
typedef struct {
  rfalIsoDepBufFormat  *rxBuf;                       /*!< Receive Buffer struct reference          */
  uint16_t             *rxLen;                       /*!< Received INF data length in Bytes        */
  bool                 *isRxChaining;                /*!< Received data is not complete            */
  rfalIsoDepDevice     *isoDepDev;                   /*!< ISO-DEP device info                      */
} rfalIsoDepListenActvParam;


/*! Structure of parameters used on ISO DEP Transceive */
typedef struct {
  rfalIsoDepBufFormat  *txBuf;                       /*!< Transmit Buffer struct reference         */
  uint16_t             txBufLen;                     /*!< Transmit Buffer INF field length in Bytes*/
  bool                 isTxChaining;                 /*!< Transmit data is not complete            */
  rfalIsoDepBufFormat  *rxBuf;                       /*!< Receive Buffer struct reference in Bytes */
  uint16_t             *rxLen;                       /*!< Received INF data length in Bytes        */
  bool                 *isRxChaining;                /*!< Received data is not complete            */
  uint32_t             FWT;                          /*!< FWT to be used (ignored in Listen Mode)  */
  uint32_t             dFWT;                         /*!< Delta FWT to be used                     */
  uint16_t             FSx;                          /*!< Other device Frame Size (FSD or FSC)     */
  uint16_t             ourFSx;                       /*!< Our device Frame Size (FSD or FSC)       */
  uint8_t              DID;                          /*!< Device ID (RFAL_ISODEP_NO_DID if no DID) */
} rfalIsoDepTxRxParam;


/*! Structure of parameters used on ISO DEP APDU Transceive */
typedef struct {
  rfalIsoDepApduBufFormat  *txBuf;                   /*!< Transmit Buffer struct reference         */
  uint16_t                 txBufLen;                 /*!< Transmit Buffer INF field length in Bytes*/
  rfalIsoDepApduBufFormat  *rxBuf;                   /*!< Receive Buffer struct reference in Bytes */
  uint16_t                 *rxLen;                   /*!< Received INF data length in Bytes        */
  rfalIsoDepBufFormat      *tmpBuf;                  /*!< Temp buffer for Rx I-Blocks (internal)   */
  uint32_t                 FWT;                      /*!< FWT to be used (ignored in Listen Mode)  */
  uint32_t                 dFWT;                     /*!< Delta FWT to be used                     */
  uint16_t                 FSx;                      /*!< Other device Frame Size (FSD or FSC)     */
  uint16_t                 ourFSx;                   /*!< Our device Frame Size (FSD or FSC)       */
  uint8_t                  DID;                      /*!< Device ID (RFAL_ISODEP_NO_DID if no DID) */
} rfalIsoDepApduTxRxParam;

/*! Enumeration of the possible control message types */
typedef enum {
  ISODEP_R_ACK,                    /*!< R-ACK  Acknowledge            */
  ISODEP_R_NAK,                    /*!< R-NACK Negative acknowledge   */
  ISODEP_S_WTX,                    /*!< S-WTX  Waiting Time Extension */
  ISODEP_S_DSL                     /*!< S-DSL  Deselect               */
} rfalIsoDepControlMsg;

/*! Internal structure to be used in handling of S(PARAMETERS) only */
typedef struct {
  uint8_t               pcb;       /*!< PCB byte                      */
  rfalIsoDepSParameter  sParam;    /*!< S(PARAMETERS)                 */
} rfalIsoDepControlMsgSParam;


/*! Enumeration of the IsoDep roles */
typedef enum {
  ISODEP_ROLE_PCD,                /*!< Perform as Reader/PCD          */
  ISODEP_ROLE_PICC                /*!< Perform as Card/PICC           */
} rfalIsoDepRole;

/*! ISO-DEP layer states */
typedef enum {
  ISODEP_ST_IDLE,                 /*!< Idle State                     */
  ISODEP_ST_PCD_TX,               /*!< PCD Transmission State         */
  ISODEP_ST_PCD_RX,               /*!< PCD Reception State            */
  ISODEP_ST_PCD_WAIT_DSL,         /*!< PCD Wait for DSL response      */

  ISODEP_ST_PICC_ACT_ATS,         /*!< PICC has replied to RATS (ATS) */
  ISODEP_ST_PICC_ACT_ATTRIB,      /*!< PICC has replied to ATTRIB     */
  ISODEP_ST_PICC_RX,              /*!< PICC REception State           */
  ISODEP_ST_PICC_SWTX,            /*!< PICC Waiting Time eXtension    */
  ISODEP_ST_PICC_SDSL,            /*!< PICC S(DSL) response ongoing   */
  ISODEP_ST_PICC_TX,              /*!< PICC Transmission State        */

  ISODEP_ST_PCD_ACT_RATS,         /*!< PCD activation (RATS)          */
  ISODEP_ST_PCD_ACT_PPS,          /*!< PCD activation (PPS)           */
} rfalIsoDepState;

/*! Holds all ISO-DEP data(counters, buffers, ID, timeouts, frame size)         */
typedef struct {
  rfalIsoDepState state;         /*!< ISO-DEP module state                      */
  rfalIsoDepRole  role;          /*!< Current ISO-DEP role                      */

  uint8_t         blockNumber;   /*!< Current block number                      */
  uint8_t         did;           /*!< Current DID                               */
  uint8_t         nad;           /*!< Current DID                               */
  uint8_t         cntIRetrys;    /*!< I-Block retry counter                     */
  uint8_t         cntRRetrys;    /*!< R-Block retry counter                     */
  uint8_t         cntSDslRetrys; /*!< S(DESELECT) retry counter                 */
  uint8_t         cntSWtxRetrys; /*!< Overall S(WTX) retry counter              */
  uint8_t         cntSWtxNack;   /*!< R(NACK) answered with S(WTX) counter      */
  uint32_t        fwt;           /*!< Current FWT (Frame Waiting Time)          */
  uint32_t        dFwt;          /*!< Current delta FWT                         */
  uint16_t        fsx;           /*!< Current FSx FSC or FSD (max Frame size)   */
  bool            isTxChaining;  /*!< Flag for chaining on Tx                   */
  bool            isRxChaining;  /*!< Flag for chaining on Rx                   */
  uint8_t        *txBuf;         /*!< Tx buffer pointer                         */
  uint8_t        *rxBuf;         /*!< Rx buffer pointer                         */
  uint16_t        txBufLen;      /*!< Tx buffer length                          */
  uint16_t        rxBufLen;      /*!< Rx buffer length                          */
  uint8_t         txBufInfPos;   /*!< Start of payload in txBuf                 */
  uint8_t         rxBufInfPos;   /*!< Start of payload in rxBuf                 */


  uint16_t        ourFsx;        /*!< Our current FSx FSC or FSD (Frame size)   */
  uint8_t         lastPCB;       /*!< Last PCB sent                             */
  uint8_t         lastWTXM;      /*!< Last WTXM sent                            */
  uint8_t         atsTA;         /*!< TA on ATS                                 */
  uint8_t         hdrLen;        /*!< Current ISO-DEP length                    */
  rfalBitRate     txBR;          /*!< Current Tx Bit Rate                       */
  rfalBitRate     rxBR;          /*!< Current Rx Bit Rate                       */
  uint16_t        *rxLen;        /*!< Output parameter ptr to Rx length         */
  bool            *rxChaining;   /*!< Output parameter ptr to Rx chaining flag  */
  uint32_t        WTXTimer;      /*!< Timer used for WTX                        */
  bool            lastDID00;     /*!< Last PCD block had DID flag (for DID = 0) */

  bool            isTxPending;   /*!< Flag pending Block while waiting WTX Ack  */
  bool            isWait4WTX;    /*!< Flag for waiting WTX Ack                  */

  uint8_t         maxRetriesI;   /*!< Number of retries for a I-Block           */
  uint8_t         maxRetriesR;   /*!< Number of retries for a R-Block           */
  uint8_t         maxRetriesSDSL;/*!< Number of retries for S(DESELECT) errors  */
  uint8_t         maxRetriesSWTX;/*!< Number of retries for S(WTX) errors       */
  uint8_t         maxRetriesSnWTX;/*!< Number of retries S(WTX) replied w NACK  */
  uint8_t         maxRetriesRATS;/*!< Number of retries for RATS                */

  rfalComplianceMode compMode;   /*!< Compliance mode                           */

  uint8_t         ctrlBuf[ISODEP_CONTROLMSG_BUF_LEN];    /*!< Control msg buf   */
  uint16_t        ctrlRxLen;  /*!< Control msg rcvd len                         */

  union {  /*  PRQA S 0750 # MISRA 19.2 - Members of the union will not be used concurrently, only one frame at a time */
#if RFAL_FEATURE_NFCA
    rfalIsoDepRats       ratsReq;
    rfalIsoDepPpsReq     ppsReq;
#endif /* RFAL_FEATURE_NFCA */

#if RFAL_FEATURE_NFCB
    rfalIsoDepAttribCmd  attribReq;
#endif /* RFAL_FEATURE_NFCB */
  } actv;                                   /*!< Activation buffer              */

  uint8_t                  *rxLen8;         /*!< Receive length (8-bit)         */
  rfalIsoDepDevice         *actvDev;        /*!< Activation Device Info         */
  rfalIsoDepListenActvParam actvParam;      /*!< Listen Activation context      */


  rfalIsoDepApduTxRxParam APDUParam;        /*!< APDU TxRx params               */
  uint16_t                APDUTxPos;        /*!< APDU Tx position               */
  uint16_t                APDURxPos;        /*!< APDU Rx position               */
  bool                    isAPDURxChaining; /*!< APDU Transceive chaining flag  */

} rfalIsoDep;

#endif /* RFAL_ISODEP_H_ */

/**
  * @}
  *
  * @}
  *
  * @}
  */
