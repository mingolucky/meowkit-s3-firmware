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
 *  \brief Implementation of NFC-B (ISO14443B) helpers
 *
 *  It provides a NFC-B Poller (ISO14443B PCD) interface and
 *  also provides some NFC-B Listener (ISO14443B PICC) helpers
 *
 *  The definitions and helpers methods provided by this module are only
 *  up to ISO14443-3 layer (excluding ATTRIB)
 *
 *
 * \addtogroup RFAL
 * @{
 *
 * \addtogroup RFAL-AL
 * \brief RFAL Abstraction Layer
 * @{
 *
 * \addtogroup NFC-B
 * \brief RFAL NFC-B Module
 * @{
 *
 */


#ifndef RFAL_NFCB_H
#define RFAL_NFCB_H

/*
 ******************************************************************************
 * INCLUDES
 ******************************************************************************
 */
#include "st_errno.h"
#include "rfal_rf.h"

/*
******************************************************************************
* ENABLE SWITCH
******************************************************************************
*/

#ifndef RFAL_FEATURE_NFCB
  #define RFAL_FEATURE_NFCB   false    /* NFC-B module configuration missing. Disabled by default */
#endif

/*
 ******************************************************************************
 * GLOBAL DEFINES
 ******************************************************************************
 */

#define RFAL_NFCB_FWTSENSB                       7680U                /*!< NFC-B FWT(SENSB)  Digital 2.0  B.3        */
#define RFAL_NFCB_DFWT                           49152U               /*!< NFC-B dFWT Delta 2.0  7.9.1.3 & B.3       */
#define RFAL_NFCB_DTPOLL_10                      rfalConvMsTo1fc(20)  /*!< NFC-B Delta Tb Poll  Digital 1.0  A.2     */
#define RFAL_NFCB_DTPOLL_20                      rfalConvMsTo1fc(17)  /*!< NFC-B Delta Tb Poll  Digital 2.1  B.3     */

#define RFAL_NFCB_AFI                            0x00U   /*!< NFC-B default Application Family   Digital 1.1 7.6.1.1 */
#define RFAL_NFCB_PARAM                          0x00U   /*!< NFC-B default SENSB_REQ PARAM                          */
#define RFAL_NFCB_CRC_LEN                        2U      /*!< NFC-B CRC length and CRC_B(AID)   Digital 1.1 Table 28 */
#define RFAL_NFCB_NFCID0_LEN                     4U      /*!< Length of NFC-B NFCID0                                 */
#define RFAL_NFCB_CMD_LEN                        1U      /*!< Length of NFC-B Command                                */

#define RFAL_NFCB_SENSB_RES_LEN                  12U     /*!< Standard length of SENSB_RES without SFGI byte         */
#define RFAL_NFCB_SENSB_RES_EXT_LEN              13U     /*!< Extended length of SENSB_RES with SFGI byte            */

#define RFAL_NFCB_SENSB_REQ_ADV_FEATURE          0x20U   /*!< Bit mask for Advance Feature in SENSB_REQ              */
#define RFAL_NFCB_SENSB_RES_FSCI_MASK            0x0FU   /*!< Bit mask for FSCI value in SENSB_RES                   */
#define RFAL_NFCB_SENSB_RES_FSCI_SHIFT           4U      /*!< Shift for FSCI value in SENSB_RES                      */
#define RFAL_NFCB_SENSB_RES_PROTO_RFU_MASK       0x08U   /*!< Bit mask for Protocol Type RFU in SENSB_RES            */
#define RFAL_NFCB_SENSB_RES_PROTO_TR2_MASK       0x03U   /*!< Bit mask for Protocol Type TR2 in SENSB_RES            */
#define RFAL_NFCB_SENSB_RES_PROTO_TR2_SHIFT      1U      /*!< Shift for Protocol Type TR2 in SENSB_RES               */
#define RFAL_NFCB_SENSB_RES_PROTO_ISO_MASK       0x01U   /*!< Bit mask Protocol Type ISO14443 Compliant in SENSB_RES */
#define RFAL_NFCB_SENSB_RES_FWI_MASK             0x0FU   /*!< Bit mask for FWI value in SENSB_RES                    */
#define RFAL_NFCB_SENSB_RES_FWI_SHIFT            4U      /*!< Bit mask for FWI value in SENSB_RES                    */
#define RFAL_NFCB_SENSB_RES_ADC_MASK             0x0CU   /*!< Bit mask for ADC value in SENSB_RES                    */
#define RFAL_NFCB_SENSB_RES_ADC_ADV_FEATURE_MASK 0x08U   /*!< Bit mask for ADC.Advanced Proto Features in SENSB_RES  */
#define RFAL_NFCB_SENSB_RES_ADC_PROPRIETARY_MASK 0x04U   /*!< Bit mask for ADC.Proprietary Application in SENSB_RES  */
#define RFAL_NFCB_SENSB_RES_FO_DID_MASK          0x01U   /*!< Bit mask for DID in SENSB_RES                          */
#define RFAL_NFCB_SENSB_RES_FO_NAD_MASK          0x02U   /*!< Bit mask for DID in SENSB_RES                          */
#define RFAL_NFCB_SENSB_RES_FO_MASK              0x03U   /*!< Bit mask for FO value in SENSB_RES (NAD and DID)       */
#define RFAL_NFCB_SENSB_RES_SFGI_MASK            0x0FU   /*!< Bit mask for SFGI in SENSB_RES                         */
#define RFAL_NFCB_SENSB_RES_SFGI_SHIFT           4U      /*!< Shift for SFGI in SENSB_RES                            */

/*
******************************************************************************
* GLOBAL MACROS
******************************************************************************
*/

/*! Get device's FSCI given its SENSB_RES  Digital 1.1 7.6.2  */
#define rfalNfcbGetFSCI( sensbRes )        ((((rfalNfcbSensbRes*)(sensbRes))->protInfo.FsciProType >> RFAL_NFCB_SENSB_RES_FSCI_SHIFT) & RFAL_NFCB_SENSB_RES_FSCI_MASK )

/*! Checks if the given NFC-B device indicates ISO-DEP support */
#define rfalNfcbIsIsoDepSupported( dev )  ( (((rfalNfcbListenDevice*)(dev))->sensbRes.protInfo.FsciProType & RFAL_NFCB_SENSB_RES_PROTO_ISO_MASK) != 0U )

/*
******************************************************************************
* GLOBAL TYPES
******************************************************************************
*/

/*! SENSB_REQ and ALLB_REQ param   Digital 1.1 7.6.1  */
typedef enum {
  RFAL_NFCB_SENS_CMD_ALLB_REQ  = 0x08,  /*!< ALLB_REQ  (WUPB)  */
  RFAL_NFCB_SENS_CMD_SENSB_REQ = 0x00   /*!< SENSB_REQ (REQB)  */
} rfalNfcbSensCmd;


/*! Number of Slots (NI) codes used for NFC-B anti collision  Digital 1.1 Table 26 */
typedef enum {
  RFAL_NFCB_SLOT_NUM_1  = 0,         /*!< N=0 :  1 slot   */
  RFAL_NFCB_SLOT_NUM_2  = 1,         /*!< N=1 :  2 slots  */
  RFAL_NFCB_SLOT_NUM_4  = 2,         /*!< N=2 :  4 slots  */
  RFAL_NFCB_SLOT_NUM_8  = 3,         /*!< N=3 :  8 slots  */
  RFAL_NFCB_SLOT_NUM_16 = 4          /*!< N=4 : 16 slots  */
} rfalNfcbSlots;


/*! SENSB_RES (ATQB) Application Data Format   Digital 1.1 Table 28 */
typedef struct {
  uint8_t  AFI;                      /*!< Application Family Identifier */
  uint8_t  CRC_B[RFAL_NFCB_CRC_LEN]; /*!< CRC_B of AID                  */
  uint8_t  numApps;                  /*!< Number of Applications        */
} rfalNfcbSensbResAppData;


/*! SENSB_RES Protocol Info format Digital 1.1 Table 29 */
typedef struct {
  uint8_t  BRC;                      /*!< Bit Rate Capability                                                            */
  uint8_t  FsciProType;              /*!< Frame Size Card Integer [4b] | Protocol Type[4 bits]                           */
  uint8_t  FwiAdcFo;                 /*!< Frame Waiting Integer [4b] | Application Data Coding [2b] | Frame Options [2b] */
  uint8_t  SFGI;                     /*!< Optional: Start-Up Frame Guard Time Integer[4b] | RFU [4b]                     */
} rfalNfcbSensbResProtocolInfo;


/*! SENSB_RES format   Digital 1.1  7.6.2 */
typedef struct {
  uint8_t                      cmd;                           /*!< SENSB_RES: 50h       */
  uint8_t                      nfcid0[RFAL_NFCB_NFCID0_LEN];  /*!< NFC Identifier (PUPI)*/
  rfalNfcbSensbResAppData      appData;                       /*!< Application Data     */
  rfalNfcbSensbResProtocolInfo protInfo;                      /*!< Protocol Information */
} rfalNfcbSensbRes;


/*! NFC-B listener device (PICC) struct  */
typedef struct {
  uint8_t           sensbResLen;                              /*!< SENSB_RES length      */
  rfalNfcbSensbRes  sensbRes;                                 /*!< SENSB_RES             */
  bool              isSleep;                                  /*!< Device sleeping flag  */
} rfalNfcbListenDevice;

/*! NFC-B Technology Detection context                                                              */
typedef struct {
  rfalNfcbSensbRes *sensbRes;            /*!< Location of SENSB_RES                               */
  uint8_t          *sensbResLen;         /*!< Location of SENSB_RES length                        */
  uint16_t          rxLen;               /*!< Reception length (16bits)                           */
} rfalNfcbTechDetParams;

/*! NFC-B Collision Resolution states                                                               */
typedef enum {
  RFAL_NFCB_CR_SLOTS_TX,                 /*!< State where slots are open and slot markers issued  */
  RFAL_NFCB_CR_SLOTS,                    /*!< State where slots are open and slot markers issued  */
  RFAL_NFCB_CR_SLEEP,                    /*!< State between slotted loop                          */
  RFAL_NFCB_CR_END                       /*!< State for terminating the collision resolution      */
} rfalNfcbColResState;


/*! NFC-B Collision Resolution context                                                              */
typedef struct {
  rfalComplianceMode    compMode;        /*!< Compliance mode to be used                          */
  uint8_t               devLimit;        /*!< Device limit to be used                             */
  rfalNfcbListenDevice *nfcbDevList;     /*!< Location of the device list                         */
  uint8_t               *devCnt;         /*!< Location of the device counter                      */
  bool                  *colPending;     /*!< Location of the Collision pending flag              */

  uint8_t               curSlots;        /*!< Current number of slots                             */
  uint8_t               curSlotNum;      /*!< Current Slot number (within slotted loop)          */
  uint8_t               endSlots;        /*!< Maximum number of slots allowed                     */
  uint8_t               curDevCnt;       /*!< Current device counter (per slotted loop)           */
  bool                  colPend;         /*!< Internal Collision pending flag                     */
  uint32_t              tmr;             /*!< Collision Resolution timer                          */
  rfalNfcbColResState   state;           /*!< Collision Resolution state                          */
} rfalNfcbColResParams;

/*! ALLB_REQ (WUPB) and SENSB_REQ (REQB) Command Format   Digital 1.1  7.6.1 */
typedef struct {
  uint8_t  cmd;                            /*!< xxxxB_REQ: 05h       */
  uint8_t  AFI;                            /*!< NFC Identifier       */
  uint8_t  PARAM;                          /*!< Application Data     */
} rfalNfcbSensbReq;

/*! SLOT_MARKER Command format  Digital 1.1  7.7.1 */
typedef struct {
  uint8_t  APn;    /*!< Slot number 2..16 | 0101b */
} rfalNfcbSlotMarker;

/*! SLPB_REQ (HLTB) Command Format   Digital 1.1  7.8.1 */
typedef struct {
  uint8_t  cmd;                            /*!< SLPB_REQ: 50h        */
  uint8_t  nfcid0[RFAL_NFCB_NFCID0_LEN];   /*!< NFC Identifier (PUPI)*/
} rfalNfcbSlpbReq;


/*! SLPB_RES (HLTB) Response Format   Digital 1.1  7.8.2 */
typedef struct {
  uint8_t  cmd;                            /*!< SLPB_RES: 00h        */
} rfalNfcbSlpbRes;


/*! RFAL NFC-B instance */
typedef struct {
  uint8_t               AFI;               /*!< AFI to be used       */
  uint8_t               PARAM;             /*!< PARAM to be used     */
  rfalNfcbColResParams  CR;                /*!< Collision Resolution */
  rfalNfcbTechDetParams DT;
} rfalNfcb;

/*
******************************************************************************
* GLOBAL FUNCTION PROTOTYPES
******************************************************************************
*/


#endif /* RFAL_NFCB_H */

/**
  * @}
  *
  * @}
  *
  * @}
  */
