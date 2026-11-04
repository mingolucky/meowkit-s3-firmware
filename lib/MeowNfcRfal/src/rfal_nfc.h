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
 *  \brief RFAL NFC device
 *
 *  This module provides the required features to behave as an NFC Poller
 *  or Listener device. It grants an easy to use interface for the following
 *  activities: Technology Detection, Collision Resolution, Activation,
 *  Data Exchange, and Deactivation
 *
 *  This layer is influenced by (but not fully aligned with) the NFC Forum
 *  specifications, in particular: Activity 2.0 and NCI 2.0
 *
 *
 *
 * \addtogroup RFAL
 * @{
 *
 * \addtogroup RFAL-AL
 * \brief RFAL Abstraction Layer
 * @{
 *
 * \addtogroup RF
 * \brief RFAL RF Abstraction Layer
 * @{
 *
 */

/*
******************************************************************************
* Some MCU have a DSI peripheral and define 'DSI' in CMSIS device header
* so undefine DSI definition as it is a structure name member of nfc rfal
******************************************************************************
*/
#ifdef DSI
  #undef DSI
#endif

#ifndef RFAL_NFC_H
#define RFAL_NFC_H

/*
******************************************************************************
* INCLUDES
******************************************************************************
*/
#include "st_errno.h"
#include "rfal_config.h"
#include "rfal_rf.h"
#include "rfal_isoDep.h"
#include "rfal_nfca.h"
#include "rfal_nfcb.h"
#include "rfal_nfcf.h"
#include "rfal_nfcv.h"
#include "rfal_st25tb.h"
#include "rfal_nfcDep.h"
#include "rfal_t4t.h"

/*
******************************************************************************
* GLOBAL DEFINES
******************************************************************************
*/
#define RFAL_NFC_MAX_DEVICES          5U    /* Max number of devices supported */

#define RFAL_NFC_TECH_NONE               0x0000U  /*!< No technology                     */
#define RFAL_NFC_POLL_TECH_A             0x0001U  /*!< Poll NFC-A technology Flag        */
#define RFAL_NFC_POLL_TECH_B             0x0002U  /*!< Poll NFC-B technology Flag        */
#define RFAL_NFC_POLL_TECH_F             0x0004U  /*!< Poll NFC-F technology Flag        */
#define RFAL_NFC_POLL_TECH_V             0x0008U  /*!< Poll NFC-V technology Flag        */
#define RFAL_NFC_POLL_TECH_AP2P          0x0010U  /*!< Poll AP2P technology Flag         */
#define RFAL_NFC_POLL_TECH_ST25TB        0x0020U  /*!< Poll ST25TB technology Flag       */
#define RFAL_NFC_POLL_TECH_PROP          0x0040U  /*!< Poll Proprietary technology Flag  */
#define RFAL_NFC_LISTEN_TECH_A           0x1000U  /*!< Listen NFC-A technology Flag      */
#define RFAL_NFC_LISTEN_TECH_B           0x2000U  /*!< Listen NFC-B technology Flag      */
#define RFAL_NFC_LISTEN_TECH_F           0x4000U  /*!< Listen NFC-F technology Flag      */
#define RFAL_NFC_LISTEN_TECH_AP2P        0x8000U  /*!< Listen AP2P technology Flag       */



/*
******************************************************************************
* GLOBAL MACROS
******************************************************************************
*/

/*! Checks if a device is currently activated */
#define rfalNfcIsDevActivated( st )        ( ((st)>= RFAL_NFC_STATE_ACTIVATED) && ((st)<RFAL_NFC_STATE_DEACTIVATION) )

/*! Checks if a device is in discovery */
#define rfalNfcIsInDiscovery( st )         ( ((st)>= RFAL_NFC_STATE_START_DISCOVERY) && ((st)<RFAL_NFC_STATE_ACTIVATED) )

/*! Checks if remote device is in Poll mode */
#define rfalNfcIsRemDevPoller( tp )    ( ((tp)>= RFAL_NFC_POLL_TYPE_NFCA) && ((tp)<=RFAL_NFC_POLL_TYPE_AP2P ) )

/*! Checks if remote device is in Listen mode */
#define rfalNfcIsRemDevListener( tp )  ( ((int16_t)(tp)>= (int16_t)RFAL_NFC_LISTEN_TYPE_NFCA) && ((tp)<=RFAL_NFC_LISTEN_TYPE_AP2P) )

/*! Sets the discover parameters to its default values */
#define rfalNfcDefaultDiscParams(dp)                       \
  if ((dp) != NULL)                                        \
  {                                                        \
    ST_MEMSET((dp), 0x00, sizeof(rfalNfcDiscoverParam)); \
    ((dp))->compMode = RFAL_COMPLIANCE_MODE_NFC;           \
    ((dp))->devLimit = 1U;                                 \
    ((dp))->nfcfBR = RFAL_BR_212;                          \
    ((dp))->ap2pBR = RFAL_BR_424;                          \
    ((dp))->maxBR = RFAL_BR_KEEP;                          \
    ((dp))->isoDepFS = RFAL_ISODEP_FSXI_256;               \
    ((dp))->nfcDepLR = RFAL_NFCDEP_LR_254;                 \
    ((dp))->GBLen = 0U;                                    \
    ((dp))->p2pNfcaPrio = false;                           \
    ((dp))->wakeupEnabled = false;                         \
    ((dp))->wakeupConfigDefault = true;                    \
    ((dp))->wakeupPollBefore = false;                      \
    ((dp))->wakeupNPolls = 1U;                             \
    ((dp))->totalDuration = 1000U;                         \
    ((dp))->techs2Find = RFAL_NFC_TECH_NONE;               \
    ((dp))->techs2Bail = RFAL_NFC_TECH_NONE;               \
  }

/*
******************************************************************************
* GLOBAL ENUMS
******************************************************************************
*/

/*
******************************************************************************
* GLOBAL TYPES
******************************************************************************
*/

/*! Main state                                                                       */
typedef enum {
  RFAL_NFC_STATE_NOTINIT                  =  0,   /*!< Not Initialized state       */
  RFAL_NFC_STATE_IDLE                     =  1,   /*!< Initialize state            */
  RFAL_NFC_STATE_START_DISCOVERY          =  2,   /*!< Start Discovery loop state  */
  RFAL_NFC_STATE_WAKEUP_MODE              =  3,   /*!< Wake-Up state               */
  RFAL_NFC_STATE_POLL_TECHDETECT          =  10,  /*!< Technology Detection state  */
  RFAL_NFC_STATE_POLL_COLAVOIDANCE        =  11,  /*!< Collision Avoidance state   */
  RFAL_NFC_STATE_POLL_SELECT              =  12,  /*!< Wait for Selection state    */
  RFAL_NFC_STATE_POLL_ACTIVATION          =  13,  /*!< Activation state            */
  RFAL_NFC_STATE_LISTEN_TECHDETECT        =  20,  /*!< Listen Tech Detect          */
  RFAL_NFC_STATE_LISTEN_COLAVOIDANCE      =  21,  /*!< Listen Collision Avoidance  */
  RFAL_NFC_STATE_LISTEN_ACTIVATION        =  22,  /*!< Listen Activation state     */
  RFAL_NFC_STATE_LISTEN_SLEEP             =  23,  /*!< Listen Sleep state          */
  RFAL_NFC_STATE_ACTIVATED                =  30,  /*!< Activated state             */
  RFAL_NFC_STATE_DATAEXCHANGE             =  31,  /*!< Data Exchange Start state   */
  RFAL_NFC_STATE_DATAEXCHANGE_DONE        =  33,  /*!< Data Exchange terminated    */
  RFAL_NFC_STATE_DEACTIVATION             =  34   /*!< Deactivation state          */
} rfalNfcState;


/*! Device type                                                                       */
typedef enum {
  RFAL_NFC_LISTEN_TYPE_NFCA               =  0,   /*!< NFC-A Listener device type  */
  RFAL_NFC_LISTEN_TYPE_NFCB               =  1,   /*!< NFC-B Listener device type  */
  RFAL_NFC_LISTEN_TYPE_NFCF               =  2,   /*!< NFC-F Listener device type  */
  RFAL_NFC_LISTEN_TYPE_NFCV               =  3,   /*!< NFC-V Listener device type  */
  RFAL_NFC_LISTEN_TYPE_ST25TB             =  4,   /*!< ST25TB Listener device type */
  RFAL_NFC_LISTEN_TYPE_AP2P               =  5,   /*!< AP2P Listener device type   */
  RFAL_NFC_LISTEN_TYPE_PROP               =  6,   /*!< Proprietary Listen dev type */
  RFAL_NFC_POLL_TYPE_NFCA                 =  10,  /*!< NFC-A Poller device type    */
  RFAL_NFC_POLL_TYPE_NFCB                 =  11,  /*!< NFC-B Poller device type    */
  RFAL_NFC_POLL_TYPE_NFCF                 =  12,  /*!< NFC-F Poller device type    */
  RFAL_NFC_POLL_TYPE_NFCV                 =  13,  /*!< NFC-V Poller device type    */
  RFAL_NFC_POLL_TYPE_AP2P                 =  15   /*!< AP2P Poller device type     */
} rfalNfcDevType;


/*! Device interface                                                                 */
typedef enum {
  RFAL_NFC_INTERFACE_RF                   = 0,    /*!< RF Frame interface          */
  RFAL_NFC_INTERFACE_ISODEP               = 1,    /*!< ISO-DEP interface           */
  RFAL_NFC_INTERFACE_NFCDEP               = 2     /*!< NFC-DEP interface           */
} rfalNfcRfInterface;

/*! Deactivation type                                                                     */
typedef enum {
  RFAL_NFC_DEACTIVATE_IDLE                = 0,    /*!< Deactivate and go to IDLE        */
  RFAL_NFC_DEACTIVATE_SLEEP               = 1,    /*!< Deactivate and go to SELECT      */
  RFAL_NFC_DEACTIVATE_DISCOVERY           = 2     /*!< Deactivate and restart DISCOVERY */
} rfalNfcDeactivateType;

/*! Device struct containing all its details                                          */
typedef struct {
  rfalNfcDevType type;                            /*!< Device's type                */
  union {                             /*  PRQA S 0750 # MISRA 19.2 - Members of the union will not be used concurrently, only one technology at a time */
    rfalNfcaListenDevice   nfca;                /*!< NFC-A Listen Device instance */
    rfalNfcbListenDevice   nfcb;                /*!< NFC-B Listen Device instance */
    rfalNfcfListenDevice   nfcf;                /*!< NFC-F Listen Device instance */
    rfalNfcvListenDevice   nfcv;                /*!< NFC-V Listen Device instance */
    rfalSt25tbListenDevice st25tb;              /*!< ST25TB Listen Device instance*/
  } dev;                                          /*!< Device's instance            */

  uint8_t                    *nfcid;              /*!< Device's NFCID               */
  uint8_t                    nfcidLen;            /*!< Device's NFCID length        */
  rfalNfcRfInterface         rfInterface;         /*!< Device's interface           */

  union {                             /*  PRQA S 0750 # MISRA 19.2 - Members of the union will not be used concurrently, only one protocol at a time */
    rfalIsoDepDevice       isoDep;              /*!< ISO-DEP instance             */
    rfalNfcDepDevice       nfcDep;              /*!< NFC-DEP instance             */
  } proto;                                        /*!< Device's protocol            */
} rfalNfcDevice;

/*! Callbacks for Proprietary|Other Technology      Activity 2.1   &   EMVCo 3.0  9.2 */
typedef ReturnCode(* rfalNfcPropCallback)(void);


/*! Struct that holds the Proprietary NFC callbacks                                                                                  */
typedef struct {
  rfalNfcPropCallback    rfalNfcpPollerInitialize;                    /*!< Prorietary NFC Initialization callback                  */
  rfalNfcPropCallback    rfalNfcpPollerTechnologyDetection;           /*!< Prorietary NFC Technoly Detection callback              */
  rfalNfcPropCallback    rfalNfcpPollerStartCollisionResolution;      /*!< Prorietary NFC Start Collision Resolution callback      */
  rfalNfcPropCallback    rfalNfcpPollerGetCollisionResolutionStatus;  /*!< Prorietary NFC Get Collision Resolution status callback */
  rfalNfcPropCallback    rfalNfcpStartActivation;                     /*!< Prorietary NFC Start Activation callback                */
  rfalNfcPropCallback    rfalNfcpGetActivationStatus;                 /*!< Prorietary NFC Get Activation status callback           */
} rfalNfcPropCallbacks;

/*! Discovery parameters                                                                                           */
typedef struct {
  rfalComplianceMode compMode;                        /*!< Compliance mode to be used                            */
  uint16_t           techs2Find;                      /*!< Technologies to search for                            */
  uint16_t           techs2Bail;                       /*!< Bail-out after certain NFC technologies                            */
  uint16_t               totalDuration;                    /*!< Duration of a whole Poll + Listen cycle        NCI 2.1 Table 46    */
  uint8_t                devLimit;                         /*!< Max number of devices                      Activity 2.1  Table 11  */
  rfalBitRate            maxBR;                            /*!< Max Bit rate to be used                        NCI 2.1  Table 28   */

  rfalBitRate            nfcfBR;                           /*!< Bit rate to poll for NFC-F                     NCI 2.1  Table 27   */
  uint8_t                nfcid3[RFAL_NFCDEP_NFCID3_LEN];   /*!< NFCID3 to be used on the ATR_REQ/ATR_RES                           */
  uint8_t                GB[RFAL_NFCDEP_GB_MAX_LEN];       /*!< General bytes to be used on the ATR-REQ        NCI 2.1  Table 29   */
  uint8_t                GBLen;                            /*!< Length of the General Bytes                    NCI 2.1  Table 29   */
  rfalBitRate            ap2pBR;                           /*!< Bit rate to poll for AP2P                      NCI 2.1  Table 31   */
  bool                   p2pNfcaPrio;                      /*!< NFC-A P2P (true) or ISO14443-4/T4T (false) priority                */
  rfalNfcPropCallbacks   propNfc;                          /*!< Proprietary Technology callbacks                                    */


  rfalIsoDepFSxI         isoDepFS;                         /*!< ISO-DEP Poller announced maximum frame size   Digital 2.2 Table 60 */
  uint8_t                nfcDepLR;                         /*!< NFC-DEP Poller & Listener maximum frame size  Digital 2.2 Table 90 */

  rfalLmConfPA           lmConfigPA;                       /*!< Configuration for Passive Listen mode NFC-A                        */
  rfalLmConfPF           lmConfigPF;                       /*!< Configuration for Passive Listen mode NFC-A                        */

  void (*notifyCb)(rfalNfcState st);                       /*!< Callback to Notify upper layer                                     */

  bool                   wakeupEnabled;                    /*!< Enable Wake-Up mode before polling                                 */
  bool                   wakeupConfigDefault;              /*!< Wake-Up mode default configuration                                 */
  rfalWakeUpConfig       wakeupConfig;                     /*!< Wake-Up mode configuration                                         */
  bool                   wakeupPollBefore;                 /*!< Flag to Poll wakeupNPolls times before entering Wake-up            */
  uint16_t               wakeupNPolls;                     /*!< Number of polling cycles before|after entering Wake-up             */
} rfalNfcDiscoverParam;


/*! Buffer union, only one interface is used at a time                                                             */
typedef union { /*  PRQA S 0750 # MISRA 19.2 - Members of the union will not be used concurrently, only one interface at a time */
  uint8_t                  rfBuf[RFAL_FEATURE_NFC_RF_BUF_LEN]; /*!< RF buffer                                    */
  rfalIsoDepApduBufFormat  isoDepBuf;                          /*!< ISO-DEP buffer format (with header/prologue) */
  rfalNfcDepPduBufFormat   nfcDepBuf;                          /*!< NFC-DEP buffer format (with header/prologue) */
} rfalNfcBuffer;

/*! Buffer union, only one interface is used at a time                                                        */
typedef union { /*PRQA S 0750 # MISRA 19.2 - Members of the union will not be used concurrently, only one interface at a time */
  rfalIsoDepBufFormat   isoDepBuf;                  /*!< ISO-DEP buffer format (with header/prologue)       */
  rfalNfcDepBufFormat   nfcDepBuf;                  /*!< NFC-DEP buffer format (with header/prologue)       */
} rfalNfcTmpBuffer;


/*! RFAL NFC instance                                                                                */
typedef struct {
  rfalNfcState            state;              /*!< Main state                                      */
  uint16_t                techsFound;         /*!< Technologies found bitmask                      */
  uint16_t                techs2do;           /*!< Technologies still to be performed              */
  uint16_t                techDctCnt;         /*!< Technologies detection counter (before WU)      */
  rfalBitRate             ap2pBR;             /*!< Bit rate to poll for AP2P                       */
  uint8_t                 selDevIdx;          /*!< Selected device index                           */
  rfalNfcDevice           *activeDev;         /*!< Active device pointer                           */
  rfalNfcDiscoverParam    disc;               /*!< Discovery parameters                            */
  rfalNfcDevice           devList[RFAL_NFC_MAX_DEVICES];   /*!< Location of device list            */
  uint8_t                 devCnt;             /*!< Devices found counter                           */
  uint32_t                discTmr;            /*!< Discovery Total duration timer                  */
  ReturnCode              dataExErr;          /*!< Last Data Exchange error                        */
  bool                    discRestart;        /* Restart discover after deactivation flag        */
  rfalNfcDeactivateType   deactType;          /*!< Deactivation type                               */
  bool                    isRxChaining;       /*!< Flag indicating Other device is chaining        */
  uint32_t                lmMask;             /*!< Listen Mode mask                                */
  bool                    isFieldOn;          /*!< Flag indicating Fieldon for Passive Poll        */
  bool                    isTechInit;         /*!< Flag indicating technology has been set         */
  bool                    isOperOngoing;      /*!< Flag indicating operation is ongoing            */
  bool                    isDeactivating;     /*!< Flag indicating deactivation is ongoing         */

  rfalNfcaSensRes         sensRes;            /*!< SENS_RES during card detection and activation   */
  rfalNfcbSensbRes        sensbRes;           /*!< SENSB_RES during card detection and activation  */
  uint8_t                 sensbResLen;        /*!< SENSB_RES length                                */

  rfalNfcBuffer           txBuf;              /*!< Tx buffer for Data Exchange                     */
  rfalNfcBuffer           rxBuf;              /*!< Rx buffer for Data Exchange                     */
  uint16_t                rxLen;              /*!< Length of received data on Data Exchange        */

#if RFAL_FEATURE_NFC_DEP || RFAL_FEATURE_ISO_DEP
  rfalNfcTmpBuffer        tmpBuf;             /*!< Tmp buffer for Data Exchange                    */
#endif /* RFAL_FEATURE_NFC_DEP || RFAL_FEATURE_ISO_DEP */

} rfalNfc;

/*******************************************************************************/


/* CLASS */
class RfalNfcClass {

  public:

    /*
    ******************************************************************************
    * GLOBAL FUNCTION PROTOTYPES
    ******************************************************************************
    */

    /*!
     *****************************************************************************
     * \brief  RFAL NFC Constructor
     *
     * It generates the RFAL NFC object.
     *****************************************************************************
     */
    RfalNfcClass(RfalRfClass *rfal_rf); // Set the hardware component to be used

    /*!
     *****************************************************************************
     * \brief  RFAL NFC Worker
     *
     * It runs the internal state machine and runs the RFAL RF worker.
     *****************************************************************************
     */
    void rfalNfcWorker(void);

    /*!
     *****************************************************************************
     * \brief  RFAL NFC Initialize
     *
     * It initializes this module and its dependencies
     *
     * \return ERR_WRONG_STATE  : Incorrect state for this operation
     * \return ERR_IO           : Generic internal error
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalNfcInitialize(void);

    /*!
     *****************************************************************************
     * \brief  RFAL NFC Discovery
     *
     * It set the device in Discovery state.
     * In discovery it will Poll and/or Listen for the technologies configured,
     * and perform Wake-up mode if configured to do so.
     *
     * The device list passed on disParams must not be empty.
     * The number of devices on the list is indicated by the devLimit and shall
     * be at >= 1.
     *
     * \param[in]  disParams    : discovery configuration parameters
     *
     * \return ERR_WRONG_STATE  : Incorrect state for this operation
     * \return ERR_PARAM        : Invalid parameters
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalNfcDiscover(const rfalNfcDiscoverParam *disParams);

    /*!
     *****************************************************************************
     * \brief  RFAL NFC Get State
     *
     * It returns the current state
     *
     * \return rfalNfcState : the current state
     *****************************************************************************
     */
    rfalNfcState rfalNfcGetState(void);

    /*!
     *****************************************************************************
     * \brief  RFAL NFC Get Devices Found
     *
     * It returns the location of the device list and the number of
     * devices found.
     *
     * \param[out]  devList     : device list location
     * \param[out]  devCnt      : number of devices found
     *
     * \return ERR_WRONG_STATE  : Incorrect state for this operation
     *                            Discovery still ongoing
     * \return ERR_PARAM        : Invalid parameters
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalNfcGetDevicesFound(rfalNfcDevice **devList, uint8_t *devCnt);

    /*!
     *****************************************************************************
     * \brief  RFAL NFC Get Active Device
     *
     * It returns the location of the device current Active device
     *
     * \param[out]  dev           : device info location
     *
     * \return ERR_WRONG_STATE    : Incorrect state for this operation
     *                              No device activated
     * \return ERR_PARAM          : Invalid parameters
     * \return ERR_NONE           : No error
     *****************************************************************************
     */
    ReturnCode rfalNfcGetActiveDevice(rfalNfcDevice **dev);

    /*!
     *****************************************************************************
     * \brief  RFAL NFC Select Device
     *
     * It selects the device to be activated.
     * It shall be called when more than one device has been identified to
     * indicate which device shall be active
     *
     * \param[in]  devIdx       : device index to be activated
     *
     * \return ERR_WRONG_STATE  : Incorrect state for this operation
     *                            Not in select state
     * \return ERR_PARAM        : Invalid parameters
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalNfcSelect(uint8_t devIdx);

    /*!
     *****************************************************************************
     * \brief  RFAL NFC Start Data Exchange
     *
     * After a device has been activated, it starts a data exchange.
     * It handles automatically which interface/protocol to be used and acts accordingly.
     *
     * In Listen mode the first frame/data shall be sent by the Reader/Initiator
     * therefore this method must be called first with txDataLen set to zero
     * to retrieve the rxData and rcvLen locations.
     *
     *
     * \param[in]  txData       : data to be transmitted
     * \param[in]  txDataLen    : size of the data to be transmitted (in bits or bytes - see below)
     * \param[out] rxData       : location of the received data after operation is completed
     * \param[out] rvdLen       : location of the length of the received data (in bits or bytes - see below)
     * \param[in]  fwt          : FWT to be used in case of RF interface.
     *                            If ISO-DEP or NFC-DEP interface is used, this will be ignored
     *
     * \warning In order to support a wider range of protocols, when RF interface is used the lengths
     *          are in number of bits (not bytes). Therefore both input txDataLen and output rvdLen refer to
     *          bits. If ISO-DEP or NFC-DEP interface is used those are expressed in number of bytes.
     *
     *
     * \return ERR_WRONG_STATE  : Incorrect state for this operation
     * \return ERR_PARAM        : Invalid parameters
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalNfcDataExchangeStart(uint8_t *txData, uint16_t txDataLen, uint8_t **rxData, uint16_t **rvdLen, uint32_t fwt);

    /*!
     *****************************************************************************
     * \brief  RFAL NFC Get Data Exchange Status
     *
     * Gets current Data Exchange status
     *
     * \return  ERR_NONE         : Transceive done with no error
     * \return  ERR_BUSY         : Transceive ongoing
     *  \return ERR_AGAIN        : received one chaining block, copy received data
     *                             and continue to call this method to retrieve the
     *                             remaining blocks
     * \return  ERR_XXXX         : Error occurred
     * \return  ERR_TIMEOUT      : No response
     * \return  ERR_FRAMING      : Framing error detected
     * \return  ERR_PAR          : Parity error detected
     * \return  ERR_CRC          : CRC error detected
     * \return  ERR_LINK_LOSS    : Link Loss - External Field is Off
     * \return  ERR_RF_COLLISION : Collision detected
     * \return  ERR_IO           : Internal error
     *****************************************************************************
     */
    ReturnCode rfalNfcDataExchangeGetStatus(void);

    /*!
    *****************************************************************************
    * \brief  RFAL NFC Deactivate
    *
    * It triggers the deactivation procedure to terminate communications with
    * remote device.
    * In case the deactivation type is RFAL_NFC_DEACTIVATE_SLEEP the field is
    * kept On and device selection shall follow. Otherwise the field will
    * be turned Off.
    *
    * \warning In case the deactivation type is RFAL_NFC_DEACTIVATE_IDLE the
    *  deactivation procedure is executed immediately and in a blocking manner
    *
    * \param[in]  deactType         : Type of deactivation to be performed
    *
    * \return ERR_WRONG_STATE  : Incorrect state for this operation
    * \return ERR_NONE         : No error
    *****************************************************************************
    */
    ReturnCode rfalNfcDeactivate(rfalNfcDeactivateType deactType);


    /*
    ******************************************************************************
    * RFAL ISO DEP FUNCTION PROTOTYPES
    ******************************************************************************
    */

    /*!
     ******************************************************************************
     * \brief Initialize the ISO-DEP protocol
     *
     * Initialize the ISO-DEP protocol layer with default config
     ******************************************************************************
     */
    void rfalIsoDepInitialize(void);


    /*!
     ******************************************************************************
     * \brief Initialize the ISO-DEP protocol
     *
     * Initialize the ISO-DEP protocol layer with additional parameters allowing
     * to customise the protocol layer for specific behaviours
     *
     *  \param[in] compMode       : compliance mode to be performed
     *  \param[in] maxRetriesR    :  Number of retries for a R-Block
     *  \param[in] maxRetriesS    :  Number of retries for a S-Block
     *  \param[in] maxRetriesI    :  Number of retries for a I-Block
     *  \param[in] maxRetriesRATS :  Number of retries for RATS
     *
     ******************************************************************************
     */
    void rfalIsoDepInitializeWithParams(rfalComplianceMode compMode, uint8_t maxRetriesR, uint8_t maxRetriesSnWTX, uint8_t maxRetriesSWTX, uint8_t maxRetriesSDSL, uint8_t maxRetriesI, uint8_t maxRetriesRATS);


    /*!
     *****************************************************************************
     *  \brief  FSxI to FSx
     *
     *  Convert Frame Size for proximity coupling Device Integer (FSxI) to
     *  Frame Size for proximity coupling Device (FSx)
     *
     *  FSD - maximum frame size for NFC Forum Device in Poll Mode
     *  FSC - maximum frame size for NFC Forum Device in Listen Mode
     *
     *  FSxI = FSDI or FSCI
     *  FSx  = FSD or FSC
     *
     *  The FSD/FSC value includes the header and CRC
     *
     *  \param[in] FSxI :  Frame Size for proximity coupling Device Integer
     *
     *  \return fsx : Frame Size for proximity coupling Device (FSD or FSC)
     *
     *****************************************************************************
     */
    uint16_t rfalIsoDepFSxI2FSx(uint8_t FSxI);


    /*!
     *****************************************************************************
     *  \brief  FWI to FWT
     *
     *  Convert Frame Waiting time Integer (FWI) to Frame Waiting Time (FWT) in
     *  1/fc units
     *
     *  \param[in] fwi : Frame Waiting time Integer
     *
     *  \return fwt : Frame Waiting Time in 1/fc units
     *
     *****************************************************************************
     */
    uint32_t rfalIsoDepFWI2FWT(uint8_t fwi);


    /*!
     *****************************************************************************
     *  \brief  Check if the buffer data contains a valid RATS command
     *
     *  Check if it is a  well formed RATS command with 2 bytes
     *  This function does not check the validity of FSDI and DID
     *
     *  \param[in] buf    : reference to buffer containing the data to be checked
     *  \param[in] bufLen : length of data in the buffer in bytes
     *
     *  \return true if the data indicates a RATS command; false otherwise
     *****************************************************************************
     */
    bool rfalIsoDepIsRats(const uint8_t *buf, uint8_t bufLen);


    /*!
     *****************************************************************************
     *  \brief  Check if the buffer data contains a valid ATTRIB command
     *
     *  Check if it is a well formed ATTRIB command, but does not check the
     *  validity of the information inside
     *
     *  \param[in] buf    : reference to buffer containing the data to be checked
     *  \param[in] bufLen : length of data in the buffer in bytes
     *
     *  \return true if the data indicates a ATTRIB command; false otherwise
     *****************************************************************************
     */
    bool rfalIsoDepIsAttrib(const uint8_t *buf, uint8_t bufLen);


    /*!
     *****************************************************************************
     * \brief Start Listen Activation Handling
     *
     * Start Listen Activation Handling and setup to receive first I-block which may
     * contain complete or partial APDU after activation is completed
     *
     *  Pass in RATS for T4AT, or ATTRIB for T4BT, to handle ATS or ATTRIB Response respectively
     *  The Activation Handling handles ATS and ATTRIB Response; and additionally PPS Response
     *  if a PPS is received for T4AT.
     *  The method uses the current RFAL state machine to determine if it is expecting RATS or ATTRIB
     *
     *  Activation is completed if PPS Response is sent or if first PDU is received in T4T-A
     *  Activation is completed if ATTRIB Response is sent in T4T-B
     *
     *  \ref rfalIsoDepListenGetActivationStatus provide status if activation is completed.
     *  \ref rfalIsoDepStartTransceive shall be called right after activation is completed
     *
     *  \param[in] atsParam       : reference to ATS parameters
     *  \param[in] attribResParam : reference to ATTRIB_RES parameters
     *  \param[in] buf            : reference to buffer containing RATS or ATTRIB
     *  \param[in] bufLen         : length in bytes of the given buffer
     *  \param[in] actParam       : reference to incoming reception information will be placed
     *
     *
     *  \warning Once the Activation has been completed the method
     *  rfalIsoDepGetTransceiveStatus() must be called.
     *  If activation has completed due to reception of a data block (not PPS) the
     *  buffer owned by the caller and passed on actParam must still contain this data.
     *  The first data will be processed (I-Block or S-DSL) by rfalIsoDepGetTransceiveStatus()
     *  inform the caller and then for the next transaction use rfalIsoDepStartTransceive()
     *
     *  \return ERR_NONE    : RATS/ATTRIB is valid and activation has started
     *  \return ERR_PARAM   : Invalid parameters
     *  \return ERR_PROTO   : Invalid request
     *  \return ERR_NOTSUPP : Feature not supported
     *****************************************************************************
     */
    ReturnCode rfalIsoDepListenStartActivation(rfalIsoDepAtsParam *atsParam, const rfalIsoDepAttribResParam *attribResParam, const uint8_t *buf, uint16_t bufLen, rfalIsoDepListenActvParam actParam);


    /*!
     *****************************************************************************
     *  \brief Get the current Activation Status
     *
     *  \return ERR_NONE if Activation is already completed
     *  \return ERR_BUSY if Activation is ongoing
     *  \return ERR_LINK_LOSS if Remote Field is turned off
     *****************************************************************************
     */
    ReturnCode rfalIsoDepListenGetActivationStatus(void);


    /*!
     *****************************************************************************
     *  \brief Get the ISO-DEP Communication Information
     *
     *  Gets the maximum INF length in bytes based on current Frame Size
     *  for proximity coupling Device (FSD or FSC) excluding the header and CRC
     *
     *  \return maximum INF length in bytes
     *****************************************************************************
     */
    uint16_t rfalIsoDepGetMaxInfLen(void);


    /*!
     *****************************************************************************
     *  \brief ISO-DEP Start Transceive
     *
     *  This method triggers a ISO-DEP Transceive containing a complete or
     *  partial APDU
     *  It transmits the given message and handles all protocol retransmitions,
     *  error handling and control messages
     *
     *  The txBuf  contains a complete or partial APDU (INF) to be transmitted
     *  The Prologue field will be manipulated by the Transceive
     *
     *  If the buffer contains a partial APDU and is not the last block,
     *  then isTxChaining must be set to true
     *
     *  \param[in] param: reference parameters to be used for the Transceive
     *
     *  \return ERR_PARAM       : Bad request
     *  \return ERR_WRONG_STATE : The module is not in a proper state
     *  \return ERR_NONE        : The Transceive request has been started
     *****************************************************************************
     */
    ReturnCode rfalIsoDepStartTransceive(rfalIsoDepTxRxParam param);


    /*!
     *****************************************************************************
     *  \brief Get the Transceive status
     *
     *  Returns the status of the ISO-DEP Transceive
     *
     *  \warning  When the other device is performing chaining once a chained
     *            block is received the error ERR_AGAIN is sent. At this point
     *            caller must handle the received data immediately.
     *            When ERR_AGAIN is returned an ACK has already been sent to
     *            the other device and the next block might be incoming.
     *            If rfalWorker() is called frequently it will place the next
     *            block on the given buffer
     *
     *
     *  \return ERR_NONE      : Transceive has been completed successfully
     *  \return ERR_BUSY      : Transceive is ongoing
     *  \return ERR_PROTO     : Protocol error occurred
     *  \return ERR_TIMEOUT   : Timeout error occurred
     *  \return ERR_SLEEP_REQ : Deselect has been received and responded
     *  \return ERR_NOMEM     : The received INF does not fit into the
     *                            receive buffer
     *  \return ERR_LINK_LOSS : Communication is lost because Reader/Writer
     *                            has turned off its field
     *  \return ERR_AGAIN     : received one chaining block, continue to call
     *                            this method to retrieve the remaining blocks
     *****************************************************************************
     */
    ReturnCode rfalIsoDepGetTransceiveStatus(void);


    /*!
     *****************************************************************************
     *  \brief ISO-DEP Start APDU Transceive
     *
     *  This method triggers a ISO-DEP Transceive containing a complete APDU
     *  It transmits the given message and handles all protocol retransmitions,
     *  error handling and control messages
     *
     *  The txBuf  contains a complete APDU to be transmitted
     *  The Prologue field will be manipulated by the Transceive
     *
     *  \warning the txBuf will be modified during the transmission
     *  \warning the maximum RF frame which can be received is limited by param.tmpBuf
     *
     *  \param[in] param: reference parameters to be used for the Transceive
     *
     *  \return ERR_PARAM       : Bad request
     *  \return ERR_WRONG_STATE : The module is not in a proper state
     *  \return ERR_NONE        : The Transceive request has been started
     *****************************************************************************
     */
    ReturnCode rfalIsoDepStartApduTransceive(rfalIsoDepApduTxRxParam param);


    /*!
     *****************************************************************************
     *  \brief Get the APDU Transceive status
     *
     *  \return ERR_NONE      : if Transceive has been completed successfully
     *  \return ERR_BUSY      : if Transceive is ongoing
     *  \return ERR_PROTO     : if a protocol error occurred
     *  \return ERR_TIMEOUT   : if a timeout error occurred
     *  \return ERR_SLEEP_REQ : if Deselect is received and responded
     *  \return ERR_NOMEM     : if the received INF does not fit into the
     *                            receive buffer
     *  \return ERR_LINK_LOSS : if communication is lost because Reader/Writer
     *                            has turned off its field
     *****************************************************************************
     */
    ReturnCode rfalIsoDepGetApduTransceiveStatus(void);

    /*!
     *****************************************************************************
     *  \brief  ISO-DEP Send RATS
     *
     *  This sends a RATS to make a NFC-A Listen Device to enter
     *  ISO-DEP layer (ISO14443-4) and checks if the received ATS is valid
     *
     *  \param[in]  FSDI   : Frame Size Device Integer to be used
     *  \param[in]  DID    : Device ID to be used or RFAL_ISODEP_NO_DID for not use DID
     *  \param[out] ats    : pointer to place the ATS Response
     *  \param[out] atsLen : pointer to place the ATS length
     *
     *  \return ERR_WRONG_STATE  : RFAL not initialized or incorrect mode
     *  \return ERR_PARAM        : Invalid parameters
     *  \return ERR_IO           : Generic internal error
     *  \return ERR_TIMEOUT      : Timeout error
     *  \return ERR_PAR          : Parity error detected
     *  \return ERR_CRC          : CRC error detected
     *  \return ERR_FRAMING      : Framing error detected
     *  \return ERR_PROTO        : Protocol error detected
     *  \return ERR_NONE         : No error, ATS received
     *****************************************************************************
     */
    ReturnCode rfalIsoDepRATS(rfalIsoDepFSxI FSDI, uint8_t DID, rfalIsoDepAts *ats, uint8_t *atsLen);


    /*!
     *****************************************************************************
     *  \brief  ISO-DEP Send PPS
     *
     *  This sends a PPS to make a NFC-A Listen Device change the communications
     *  bit rate from 106kbps to one of the supported bit rates
     *  Additionally checks if the received PPS response is valid
     *
     *  \param[in]  DID    : Device ID
     *  \param[in]  DSI    : DSI code the divisor from Listener (PICC) to Poller (PCD)
     *  \param[in]  DRI    : DRI code the divisor from Poller (PCD) to Listener (PICC)
     *  \param[out] ppsRes : pointer to place the PPS Response
     *
     *  \return ERR_WRONG_STATE  : RFAL not initialized or incorrect mode
     *  \return ERR_PARAM        : Invalid parameters
     *  \return ERR_IO           : Generic internal error
     *  \return ERR_TIMEOUT      : Timeout error
     *  \return ERR_PAR          : Parity error detected
     *  \return ERR_CRC          : CRC error detected
     *  \return ERR_FRAMING      : Framing error detected
     *  \return ERR_PROTO        : Protocol error detected
     *  \return ERR_NONE         : No error, PPS Response received
     *****************************************************************************
     */
    ReturnCode rfalIsoDepPPS(uint8_t DID, rfalBitRate DSI, rfalBitRate DRI, rfalIsoDepPpsRes *ppsRes);


    /*!
     *****************************************************************************
     *  \brief  ISO-DEP Send ATTRIB
     *
     *  This sends a ATTRIB to make a NFC-B Listen Device to enter
     *  ISO-DEP layer (ISO14443-4) and checks if the received ATTRIB Response is valid
     *
     *  \param[in]  nfcid0    : NFCID0 to be used for the ATTRIB
     *  \param[in]  PARAM1    : ATTRIB PARAM1 byte (communication parameters)
     *  \param[in]  DSI       : DSI code the divisor from Listener (PICC) to Poller (PCD)
     *  \param[in]  DRI       : DRI code the divisor from Poller (PCD) to Listener (PICC)
     *  \param[in]  FSDI      : PCD's Frame Size to be announced on the ATTRIB
     *  \param[in]  PARAM3    : ATTRIB PARAM1 byte (protocol type)
     *  \param[in]  DID       : Device ID to be used or RFAL_ISODEP_NO_DID for not use DID
     *  \param[in]  HLInfo    : pointer to Higher layer INF (NULL if none)
     *  \param[in]  HLInfoLen : Length HLInfo
     *  \param[in]  fwt       : Frame Waiting Time to be used (from SENSB_RES)
     *  \param[out] attribRes    : pointer to place the ATTRIB Response
     *  \param[out] attribResLen : pointer to place the ATTRIB Response length
     *
     *  \return ERR_WRONG_STATE  : RFAL not initialized or incorrect mode
     *  \return ERR_PARAM        : Invalid parameters
     *  \return ERR_IO           : Generic internal error
     *  \return ERR_TIMEOUT      : Timeout error
     *  \return ERR_CRC          : CRC error detected
     *  \return ERR_FRAMING      : Framing error detected
     *  \return ERR_PROTO        : Protocol error detected
     *  \return ERR_NONE         : No error, ATTRIB Response received
     *****************************************************************************
     */
    ReturnCode rfalIsoDepATTRIB(const uint8_t *nfcid0, uint8_t PARAM1, rfalBitRate DSI, rfalBitRate DRI, rfalIsoDepFSxI FSDI, uint8_t PARAM3, uint8_t DID, const uint8_t *HLInfo, uint8_t HLInfoLen, uint32_t fwt, rfalIsoDepAttribRes *attribRes, uint8_t *attribResLen);


    /*!
     *****************************************************************************
     *  \brief  Deselects PICC
     *
     *  This function sends a deselect command to PICC and waits for it`s
     *  response in a blocking way
     *
     *  \return ERR_NONE   : Deselect successfully sent and acknowledged by PICC
     *  \return ERR_TIMEOUT: No response rcvd from PICC
     *
     *****************************************************************************
     */
    ReturnCode rfalIsoDepDeselect(void);


    /*!
     *****************************************************************************
     *  \brief  ISO-DEP Poller Handle NFC-A Activation
     *
     *  This performs a NFC-A Activation into ISO-DEP layer (ISO14443-4) with the given
     *  parameters. It sends RATS and if the higher bit rates are supported by
     *  both devices it additionally sends PPS
     *  Once Activated all details of the device are provided on isoDepDev
     *
     *  \param[in]  FSDI      : Frame Size Device Integer to be used
     *  \param[in]  DID       : Device ID to be used or RFAL_ISODEP_NO_DID for not use DID
     *  \param[in]  maxBR     : Max bit rate supported by the Poller
     *  \param[out] isoDepDev : ISO-DEP information of the activated Listen device
     *
     *  \return ERR_WRONG_STATE  : RFAL not initialized or incorrect mode
     *  \return ERR_PARAM        : Invalid parameters
     *  \return ERR_IO           : Generic internal error
     *  \return ERR_TIMEOUT      : Timeout error
     *  \return ERR_PAR          : Parity error detected
     *  \return ERR_CRC          : CRC error detected
     *  \return ERR_FRAMING      : Framing error detected
     *  \return ERR_PROTO        : Protocol error detected
     *  \return ERR_NONE         : No error, activation successful
     *****************************************************************************
     */
    ReturnCode rfalIsoDepPollAHandleActivation(rfalIsoDepFSxI FSDI, uint8_t DID, rfalBitRate maxBR, rfalIsoDepDevice *isoDepDev);


    /*!
     *****************************************************************************
     *  \brief  ISO-DEP Poller Handle NFC-B Activation
     *
     *  This performs a NFC-B Activation into ISO-DEP layer (ISO14443-4) with the given
     *  parameters. It sends ATTRIB and calculates supported higher bit rates of both
     *  devices and performs activation.
     *  Once Activated all details of the device are provided on isoDepDev
     *
     *  \param[in]  FSDI         : Frame Size Device Integer to be used
     *  \param[in]  DID          : Device ID to be used or RFAL_ISODEP_NO_DID for not use DID
     *  \param[in]  maxBR        : Max bit rate supported by the Poller
     *  \param[in]  PARAM1       : ATTRIB PARAM1 byte (communication parameters)
     *  \param[in]  nfcbDev      : pointer to the NFC-B Device containing the SENSB_RES
     *  \param[in]  HLInfo       : pointer to Higher layer INF (NULL if none)
     *  \param[in]  HLInfoLen    : Length HLInfo
     *  \param[out] isoDepDev    : ISO-DEP information of the activated Listen device
     *
     *  \return ERR_WRONG_STATE  : RFAL not initialized or incorrect mode
     *  \return ERR_PARAM        : Invalid parameters
     *  \return ERR_IO           : Generic internal error
     *  \return ERR_TIMEOUT      : Timeout error
     *  \return ERR_PAR          : Parity error detected
     *  \return ERR_CRC          : CRC error detected
     *  \return ERR_FRAMING      : Framing error detected
     *  \return ERR_PROTO        : Protocol error detected
     *  \return ERR_NONE         : No error, activation successful
     *****************************************************************************
     */
    ReturnCode rfalIsoDepPollBHandleActivation(rfalIsoDepFSxI FSDI, uint8_t DID, rfalBitRate maxBR, uint8_t PARAM1, const rfalNfcbListenDevice *nfcbDev, const uint8_t *HLInfo, uint8_t HLInfoLen, rfalIsoDepDevice *isoDepDev);


    /*!
     *****************************************************************************
     *  \brief  ISO-DEP Poller Handle S(Parameters)
     *
     *  This checks if PICC supports S(PARAMETERS), retrieves PICC's
     *  capabilities and sets the Bit Rate at the highest supported by both
     *  devices
     *
     *  \param[out] isoDepDev    : ISO-DEP information of the activated Listen device
     *  \param[in]  maxTxBR      : Maximum Tx bit rate supported by PCD
     *  \param[in]  maxRxBR      : Maximum Rx bit rate supported by PCD
     *
     *  \return ERR_WRONG_STATE  : RFAL not initialized or incorrect mode
     *  \return ERR_PARAM        : Invalid parameters
     *  \return ERR_IO           : Generic internal error
     *  \return ERR_TIMEOUT      : Timeout error
     *  \return ERR_FRAMING      : Framing error detected
     *  \return ERR_PROTO        : Protocol error detected
     *  \return ERR_NONE         : No error, S(PARAMETERS) selection successful
     *****************************************************************************
     */
    ReturnCode rfalIsoDepPollHandleSParameters(rfalIsoDepDevice *isoDepDev, rfalBitRate maxTxBR, rfalBitRate maxRxBR);


    /*
    ******************************************************************************
    * RFAL NFC-A FUNCTION PROTOTYPES
    ******************************************************************************
    */

    /*!
     *****************************************************************************
     * \brief  Initialize NFC-A Poller mode
     *
     * This methods configures RFAL RF layer to perform as a
     * NFC-A Poller/RW (ISO14443A PCD) including all default timings and bit rate
     * to 106 kbps

     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalNfcaPollerInitialize(void);


    /*!
     *****************************************************************************
     * \brief  NFC-A Poller Check Presence
     *
     * This method checks if a NFC-A Listen device (PICC) is present on the field
     * by sending an ALL_REQ (WUPA) or SENS_REQ (REQA)
     *
     * \param[in]  cmd     : Indicate if to send an ALL_REQ or a SENS_REQ
     * \param[out] sensRes : If received, the SENS_RES
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or incorrect mode
     * \return ERR_PARAM        : Invalid parameters
     * \return ERR_IO           : Generic internal error
     * \return ERR_RF_COLLISION : Collision detected one or more device in the field
     * \return ERR_PAR          : Parity error detected, one or more device in the field
     * \return ERR_CRC          : CRC error detected, one or more device in the field
     * \return ERR_FRAMING      : Framing error detected, one or more device in the field
     * \return ERR_PROTO        : Protocol error detected, one or more device in the field
     * \return ERR_TIMEOUT      : Timeout error, no listener device detected
     * \return ERR_NONE         : No error, one or more device in the field
     *****************************************************************************
     */
    ReturnCode rfalNfcaPollerCheckPresence(rfal14443AShortFrameCmd cmd, rfalNfcaSensRes *sensRes);


    /*!
     *****************************************************************************
     * \brief  NFC-A Poller Select
     *
     * This method selects a NFC-A Listener device (PICC)
     *
     * \param[in]  nfcid1   : Listener device NFCID1 to be selected
     * \param[in]  nfcidLen : Length of the NFCID1 to be selected
     * \param[out] selRes   : pointer to place the SEL_RES
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or incorrect mode
     * \return ERR_PARAM        : Invalid parameters
     * \return ERR_IO           : Generic internal error
     * \return ERR_TIMEOUT      : Timeout error
     * \return ERR_PAR          : Parity error detected
     * \return ERR_CRC          : CRC error detected
     * \return ERR_FRAMING      : Framing error detected
     * \return ERR_PROTO        : Protocol error detected
     * \return ERR_NONE         : No error, SEL_RES received
     *****************************************************************************
     */
    ReturnCode rfalNfcaPollerSelect(const uint8_t *nfcid1, uint8_t nfcidLen, rfalNfcaSelRes *selRes);


    /*!
     *****************************************************************************
     * \brief  NFC-A Poller Sleep
     *
     * This method sends a SLP_REQ (HLTA)
     * No response is expected afterwards   Digital 1.1  6.9.2.1
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or incorrect mode
     * \return ERR_PARAM        : Invalid parameters
     * \return ERR_IO           : Generic internal error
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalNfcaPollerSleep(void);


    /*!
     *****************************************************************************
     * \brief  NFC-A Technology Detection
     *
     * This method performs NFC-A Technology Detection as defined in the spec
     * given in the compliance mode
     *
     * \param[in]  compMode  : compliance mode to be performed
     * \param[out] sensRes   : location to store the SENS_RES, if received
     *
     * When compMode is set to ISO compliance a SLP_REQ (HLTA) is not sent
     * after detection. When set to EMV a ALL_REQ (WUPA) is sent instead of
     * a SENS_REQ (REQA)
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or incorrect mode
     * \return ERR_PARAM        : Invalid parameters
     * \return ERR_IO           : Generic internal error
     * \return ERR_NONE         : No error, one or more device in the field
     *****************************************************************************
     */
    ReturnCode rfalNfcaPollerTechnologyDetection(rfalComplianceMode compMode, rfalNfcaSensRes *sensRes);

    /*!
    *****************************************************************************
    * \brief  NFC-A Start Technology Detection
    *
    * This method starts NFC-A Technology Detection as defined in the spec
    * given in the compliance mode
    *
    * \param[in]  compMode  : compliance mode to be performed
    * \param[out] sensRes   : location to store the SENS_RES, if received
    *
    * When compMode is set to ISO compliance a SLP_REQ (HLTA) is not sent
    * after detection. When set to EMV a ALL_REQ (WUPA) is sent instead of
    * a SENS_REQ (REQA)
    *
    * \return ERR_WRONG_STATE  : RFAL not initialized or incorrect mode
    * \return ERR_PARAM        : Invalid parameters
    * \return ERR_IO           : Generic internal error
    * \return ERR_NONE         : No error, one or more device in the field
    *****************************************************************************
    */
    ReturnCode rfalNfcaPollerStartTechnologyDetection(rfalComplianceMode compMode, rfalNfcaSensRes *sensRes);

    /*!
     *****************************************************************************
     * \brief  NFC-A Poller Collision Resolution
     *
     * Collision resolution for one NFC-A Listener device/card (PICC) as
     * defined in Activity 1.1  9.3.4
     *
     * This method executes anti collision loop and select the device with higher NFCID1
     *
     * When devLimit = 0 it is configured to perform collision detection only. Once a collision
     * is detected the collision resolution is aborted immediately. If only one device is found
     * with no collisions, it will properly resolved.
     *
     * \param[in]  devLimit    : device limit value (CON_DEVICES_LIMIT)
     * \param[out] collPending : pointer to collision pending flag (INT_COLL_PEND)
     * \param[out] selRes      : location to store the last Select Response from listener device (PICC)
     * \param[out] nfcId1      : location to store the NFCID1 (UID), ensure RFAL_NFCA_CASCADE_3_UID_LEN
     * \param[out] nfcId1Len   : pointer to length of NFCID1 (UID)
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
     * \return ERR_PARAM        : Invalid parameters
     * \return ERR_IO           : Generic internal error
     * \return ERR_PROTO        : Card length invalid
     * \return ERR_IGNORE       : conDevLimit is 0 and there is a collision
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalNfcaPollerSingleCollisionResolution(uint8_t devLimit, bool *collPending, rfalNfcaSelRes *selRes, uint8_t *nfcId1, uint8_t *nfcId1Len);


    /*!
     *****************************************************************************
     * \brief  NFC-A Poller Full Collision Resolution
     *
     * Performs a full Collision resolution as defined in Activity 1.0 or 1.1  9.3.4
     *
     * \param[in]  compMode    : compliance mode to be performed
     * \param[in]  devLimit    : device limit value, and size nfcaDevList
     * \param[out] nfcaDevList : NFC-A listener device info
     * \param[out] devCnt      : Devices found counter
     *
     * When compMode is set to ISO compliance it assumes that the device is
     * not sleeping and therefore no ALL_REQ (WUPA) is sent at the beginning.
     *
     * When compMode is set to NFC compliance an additional ALL_REQ (WUPA) is sent at
     * the beginning and a proprietary behaviour also takes place. Once a device has been
     * resolved an additional SLP_REQ (HLTA) is sent regardless if there was a collision
     * (except if the number of devices found already equals the limit).
     * This proprietary behaviour ensures proper activation of certain devices that suffer
     * from influence of Type B commands as foreseen in ISO14443-3 5.2.3
     *
     *
     * When devLimit = 0 it is configured to perform collision detection only. Once a collision
     * is detected the collision resolution is aborted immediately. If only one device is found
     * with no collisions, it will properly resolved.
     *
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
     * \return ERR_PARAM        : Invalid parameters
     * \return ERR_IO           : Generic internal error
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalNfcaPollerFullCollisionResolution(rfalComplianceMode compMode, uint8_t devLimit, rfalNfcaListenDevice *nfcaDevList, uint8_t *devCnt);


    /*!
     *****************************************************************************
     * \brief NFC-A Listener is SLP_REQ
     *
     * Checks if the given buffer contains valid NFC-A SLP_REQ (HALT)
     *
     * \param[in] buf: buffer containing data
     * \param[in] bufLen: length of the data in buffer to be checked
     *
     * \return true if data in buf contains a SLP_REQ ; false otherwise
     *****************************************************************************
     */
    bool rfalNfcaListenerIsSleepReq(const uint8_t *buf, uint16_t bufLen);


    /*
    ******************************************************************************
    * RFAL NFC-B FUNCTION PROTOTYPES
    ******************************************************************************
    */

    /*!
     *****************************************************************************
     * \brief  Initialize NFC-B Poller mode
     *
     * This methods configures RFAL RF layer to perform as a
     * NFC-B Poller/RW (ISO14443B PCD) including all default timings
     *
     * It sets NFC-B parameters (AFI, PARAM) to default values
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalNfcbPollerInitialize(void);


    /*!
     *****************************************************************************
     * \brief  Set NFC-B Poller parameters
     *
     * This methods configures RFAL RF layer to perform as a
     * NFCA Poller/RW (ISO14443A PCD) including all default timings
     *
     * Additionally configures NFC-B specific parameters to be used on the
     * following communications
     *
     * \param[in]  AFI   : Application Family Identifier to be used
     * \param[in]  PARAM : PARAM to be used, it announces whether Advanced
     *                     Features or Extended SENSB_RES is supported
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalNfcbPollerInitializeWithParams(uint8_t AFI, uint8_t PARAM);


    /*!
     *****************************************************************************
     * \brief  NFC-B Poller Check Presence
     *
     * This method checks if a NFC-B Listen device (PICC) is present on the field
     * by sending an ALLB_REQ (WUPB) or SENSB_REQ (REQB)
     *
     * \param[in]  cmd         : Indicate if to send an ALL_REQ or a SENS_REQ
     * \param[in]  slots       : The number of slots to be announced
     * \param[out] sensbRes    : If received, the SENSB_RES
     * \param[out] sensbResLen : If received, the SENSB_RES length
     *
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or incorrect mode
     * \return ERR_PARAM        : Invalid parameters
     * \return ERR_IO           : Generic internal error
     * \return ERR_TIMEOUT      : Timeout error, no listener device detected
     * \return ERR_RF_COLLISION : Collision detected one or more device in the field
     * \return ERR_PAR          : Parity error detected, one or more device in the field
     * \return ERR_CRC          : CRC error detected, one or more device in the field
     * \return ERR_FRAMING      : Framing error detected, one or more device in the field
     * \return ERR_PROTO        : Protocol error detected, invalid SENSB_RES received
     * \return ERR_NONE         : No error, SENSB_RES received
     *****************************************************************************
     */
    ReturnCode rfalNfcbPollerCheckPresence(rfalNfcbSensCmd cmd, rfalNfcbSlots slots, rfalNfcbSensbRes *sensbRes, uint8_t *sensbResLen);


    /*!
     *****************************************************************************
     * \brief  NFC-B Poller Sleep
     *
     * This function is used to send the SLPB_REQ (HLTB) command to put the PICC with
     * the given NFCID0 to state HALT so that they do not reply to further SENSB_REQ
     * commands (only to ALLB_REQ)
     *
     * \param[in]  nfcid0       : NFCID of the device to be put to Sleep
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or incorrect mode
     * \return ERR_PARAM        : Invalid parameters
     * \return ERR_IO           : Generic internal error
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalNfcbPollerSleep(const uint8_t *nfcid0);


    /*!
     *****************************************************************************
     * \brief  NFC-B Poller Slot Marker
     *
     * This method selects a NFC-B Slot marker frame
     *
     * \param[in]  slotCode     : Slot Code [1-15]
     * \param[out] sensbRes     : If received, the SENSB_RES
     * \param[out] sensbResLen  : If received, the SENSB_RES length
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or incorrect mode
     * \return ERR_PARAM        : Invalid parameters
     * \return ERR_IO           : Generic internal error
     * \return ERR_TIMEOUT      : Timeout error
     * \return ERR_PAR          : Parity error detected
     * \return ERR_CRC          : CRC error detected
     * \return ERR_FRAMING      : Framing error detected
     * \return ERR_PROTO        : Protocol error detected
     * \return ERR_NONE         : No error, SEL_RES received
     *****************************************************************************
     */
    ReturnCode rfalNfcbPollerSlotMarker(uint8_t slotCode, rfalNfcbSensbRes *sensbRes, uint8_t *sensbResLen);

    /*!
     *****************************************************************************
     * \brief  NFC-B Technology Detection
     *
     * This method performs NFC-B Technology Detection as defined in the spec
     * given in the compliance mode
     *
     * \param[in]  compMode    : compliance mode to be performed
     * \param[out] sensbRes    : location to store the SENSB_RES, if received
     * \param[out] sensbResLen : length of the SENSB_RES, if received
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or incorrect mode
     * \return ERR_PARAM        : Invalid parameters
     * \return ERR_IO           : Generic internal error
     * \return ERR_NONE         : No error, one or more device in the field
     *****************************************************************************
     */
    ReturnCode rfalNfcbPollerTechnologyDetection(rfalComplianceMode compMode, rfalNfcbSensbRes *sensbRes, uint8_t *sensbResLen);

    /*!
     *****************************************************************************
     * \brief  NFC-B Poller Collision Resolution
     *
     * NFC-B Collision resolution  Listener device/card (PICC) as
     * defined in Activity 1.1  9.3.5
     *
     * This function is used to perform collision resolution for detection in case
     * of multiple NFC Forum Devices with Technology B detected.
     * Target with valid SENSB_RES will be stored in devInfo and nfcbDevCount incremented.
     *
     * \param[in]  compMode    : compliance mode to be performed
     * \param[in]  devLimit    : device limit value, and size nfcbDevList
     * \param[out] nfcbDevList : NFC-B listener device info
     * \param[out] devCnt      : devices found counter
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
     * \return ERR_PARAM        : Invalid parameters
     * \return ERR_IO           : Generic internal error
     * \return ERR_PROTO        : Protocol error detected
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalNfcbPollerCollisionResolution(rfalComplianceMode compMode, uint8_t devLimit, rfalNfcbListenDevice *nfcbDevList, uint8_t *devCnt);

    /*!
     *****************************************************************************
     * \brief  NFC-B Poller Collision Resolution Slotted
     *
     * NFC-B Collision resolution  Listener device/card (PICC). The sequence can
     * be configured to be according to NFC Forum Activity 1.1  9.3.5, ISO10373
     * or EMVCo
     *
     * This function is used to perform collision resolution for detection in case
     * of multiple NFC Forum Devices with Technology B are detected.
     * Target with valid SENSB_RES will be stored in devInfo and nfcbDevCount incremented.
     *
     * This method provides the means to perform a collision resolution loop with specific
     * initial and end number of slots. This allows to user to start the loop already with
     * greater number of slots, and or limit the end number of slots. At the end a flag
     * indicating whether there were collisions pending is returned.
     *
     * If RFAL_COMPLIANCE_MODE_ISO is used \a initSlots must be set to RFAL_NFCB_SLOT_NUM_1
     *
     *
     * \param[in]  compMode    : compliance mode to be performed
     * \param[in]  devLimit    : device limit value, and size nfcbDevList
     * \param[in]  initSlots   : number of slots to open initially
     * \param[in]  endSlots    : number of slots when to stop collision resolution
     * \param[out] nfcbDevList : NFC-B listener device info
     * \param[out] devCnt      : devices found counter
     * \param[out] colPending  : flag indicating whether collision are still pending
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
     * \return ERR_PARAM        : Invalid parameters
     * \return ERR_IO           : Generic internal error
     * \return ERR_PROTO        : Protocol error detected
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalNfcbPollerSlottedCollisionResolution(rfalComplianceMode compMode, uint8_t devLimit, rfalNfcbSlots initSlots, rfalNfcbSlots endSlots, rfalNfcbListenDevice *nfcbDevList, uint8_t *devCnt, bool *colPending);


    /*!
     *****************************************************************************
     * \brief  NFC-B TR2 code to FDT
     *
     *  Converts the TR2 code as defined in Digital 1.1 Table 33 Minimum
     *  TR2 Coding to Frame Delay Time (FDT) in 1/Fc
     *
     * \param[in]  tr2Code : TR2 code as defined in Digital 1.1 Table 33
     *
     * \return FDT in 1/Fc
     *****************************************************************************
     */
    uint32_t rfalNfcbTR2ToFDT(uint8_t tr2Code);

    /*
    ******************************************************************************
    * RFAL NFC-DEP FUNCTION PROTOTYPES
    ******************************************************************************
    */

    /*!
     ******************************************************************************
     * \brief NFCIP Initialize
     *
     * This method resets all NFC-DEP inner states, counters and context and sets
     * default values
     *
     ******************************************************************************
     */
    void rfalNfcDepInitialize(void);


    /*!
     ******************************************************************************
     * \brief Set deactivating callback
     *
     * Sets the deactivating callback so that nfcip layer can check if upper layer
     * has a deactivation pending, and not perform error recovery upon specific
     * errors
     *
     * \param[in] pFunc : method pointer to deactivation flag check
     ******************************************************************************
     */
    void rfalNfcDepSetDeactivatingCallback(rfalNfcDepDeactCallback pFunc);


    /*!
     ******************************************************************************
     * \brief Calculate Response Waiting Time
     *
     * Calculates the Response Waiting Time (RWT) from the given Waiting Time (WT)
     *
     * \param[in]  wt : the WT value to calculate RWT
     *
     * \return RWT value in 1/fc
     ******************************************************************************
     */
    uint32_t rfalNfcDepCalculateRWT(uint8_t wt);


    /*!
     ******************************************************************************
     * \brief NFC-DEP Initiator ATR (Attribute Request)
     *
     * This method configures the NFC-DEP layer with given parameters and then
     * sends an ATR to the Target with and checks for a valid response response
     *
     * \param[in]   param     : parameters to initialize and compose the ATR
     * \param[out]  atrRes    : location to store the ATR_RES
     * \param[out]  atrResLen : length of the ATR_RES received
     *
     * \return ERR_NONE    : No error
     * \return ERR_TIMEOUT : Timeout occurred
     * \return ERR_PROTO   : Protocol error occurred
     ******************************************************************************
     */
    ReturnCode rfalNfcDepATR(const rfalNfcDepAtrParam *param, rfalNfcDepAtrRes *atrRes, uint8_t *atrResLen);


    /*!
     ******************************************************************************
     * \brief NFC-DEP Initiator PSL (Parameter Selection)
     *
     * This method sends a PSL to the Target with the given parameters and checks
     * for a valid response response
     *
     * The parameters must be coded according to  Digital 1.1  16.7.1
     *
     * \param[in] BRS : the selected Bit Rates for Initiator and Target
     * \param[in] FSL : the maximum length of Commands and Responses
     *
     * \return ERR_NONE    : No error
     * \return ERR_TIMEOUT : Timeout occurred
     * \return ERR_PROTO   : Protocol error occurred
     ******************************************************************************
     */
    ReturnCode rfalNfcDepPSL(uint8_t BRS, uint8_t FSL);


    /*!
     ******************************************************************************
     * \brief NFC-DEP Initiator DSL (Deselect)
     *
     * This method checks if the NFCIP module is configured as initiator and if
     * so sends a DSL REQ, waits  the target's response and checks it
     *
     * In case of performing as target no action is taken
     *
     * \return ERR_NONE       : No error
     * \return ERR_TIMEOUT    : Timeout occurred
     * \return ERR_MAX_RERUNS : Timeout occurred
     * \return ERR_PROTO      : Protocol error occurred
     ******************************************************************************
     */
    ReturnCode rfalNfcDepDSL(void);


    /*!
     ******************************************************************************
     * \brief NFC-DEP Initiator RLS (Release)
     *
     * This method checks if the NFCIP module is configured as initiator and if
     * so sends a RLS REQ, waits target's response and checks it
     *
     * In case of performing as target no action is taken
     *
     * \return ERR_NONE       : No error
     * \return ERR_TIMEOUT    : Timeout occurred
     * \return ERR_MAX_RERUNS : Timeout occurred
     * \return ERR_PROTO      : Protocol error occurred
     ******************************************************************************
     */
    ReturnCode rfalNfcDepRLS(void);


    /*!
     *****************************************************************************
     *  \brief  NFC-DEP Initiator Handle  Activation
     *
     *  This performs a Activation into NFC-DEP layer with the given
     *  parameters. It sends ATR_REQ and if the higher bit rates are supported by
     *  both devices it additionally sends PSL
     *  Once Activated all details of the device are provided on nfcDepDev
     *
     *  \param[in]  param     : required parameters to initialize and send ATR_REQ
     *  \param[in]  desiredBR : Desired bit rate supported by the Poller
     *  \param[out] nfcDepDev : NFC-DEP information of the activated Listen device
     *
     *  \return ERR_WRONG_STATE  : RFAL not initialized or incorrect mode
     *  \return ERR_PARAM        : Invalid parameters
     *  \return ERR_IO           : Generic internal error
     *  \return ERR_TIMEOUT      : Timeout error
     *  \return ERR_PAR          : Parity error detected
     *  \return ERR_CRC          : CRC error detected
     *  \return ERR_FRAMING      : Framing error detected
     *  \return ERR_PROTO        : Protocol error detected
     *  \return ERR_NONE         : No error, activation successful
     *****************************************************************************
     */
    ReturnCode rfalNfcDepInitiatorHandleActivation(rfalNfcDepAtrParam *param, rfalBitRate desiredBR, rfalNfcDepDevice *nfcDepDev);


    /*!
     ******************************************************************************
     * \brief Check if buffer contains valid ATR_REQ
     *
     * This method checks if the given ATR_REQ is valid
     *
     *
     * \param[in]  buf    : buffer holding Initiator's received request
     * \param[in]  bufLen : size of the msg contained on the buf in Bytes
     * \param[out] nfcid3 : pointer to where the NFCID3 may be outputted,
     *                       nfcid3 has NFCF_SENSF_NFCID3_LEN as length
     *                       Pass NULL if output parameter not desired
     *
     * \return true  : Valid ATR_REQ received, the ATR_RES has been computed in txBuf
     * \return false : Invalid protocol request
     *
     ******************************************************************************
     */
    bool rfalNfcDepIsAtrReq(const uint8_t *buf, uint16_t bufLen, uint8_t *nfcid3);


    /*!
     ******************************************************************************
     * \brief Check is Target has received ATR
     *
     * This method checks if the NFCIP module is configured as target and if a
     * ATR REQ has been received ( whether is in activation or in data exchange)
     *
     * \return true  : a ATR has already been received
     * \return false : no ATR has been received
     ******************************************************************************
     */
    bool rfalNfcDepTargetRcvdATR(void);

    /*!
     *****************************************************************************
     * \brief NFCDEP Start Listen Activation Handling
     *
     * Start Activation Handling and setup to receive first frame which may
     * contain complete or partial DEP-REQ after activation is completed
     *
     * Pass in ATR_REQ for NFC-DEP to handle ATR_RES. The Activation Handling
     * handles ATR_RES and PSL_RES if a PSL_REQ is received
     *
     * Activation is completed if PSL_RES is sent or if first I-PDU is received
     *
     * \ref rfalNfcDepListenGetActivationStatus() provide status of the
     *       ongoing activation
     *
     * \warning nfcDepGetTransceiveStatus() shall be called right after activation
     * is completed (i.e. rfalNfcDepListenGetActivationStatus() return ERR_NONE)
     * to check for first received frame.
     *
     * \param[in]  param       : Target parameters to be used
     * \param[in]  atrReq      : reference to buffer containing ATR_REQ
     * \param[in]  atrReqLength: Length of ATR_REQ
     * \param[out] rxParam     : references to buffer, length and chaining indication
     *                           for first complete LLCP to be received
     *
     * \return ERR_NONE      : ATR_REQ is valid and activation ongoing
     * \return ERR_PARAM     : ATR_REQ or other params are invalid
     * \return ERR_LINK_LOSS : Remote Field is turned off
     *****************************************************************************
     */
    ReturnCode rfalNfcDepListenStartActivation(const rfalNfcDepTargetParam *param, const uint8_t *atrReq, uint16_t atrReqLength, rfalNfcDepListenActvParam rxParam);


    /*!
     *****************************************************************************
     * \brief Get the current NFC-DEP Activation Status
     *
     * \return ERR_NONE      : Activation has completed successfully
     * \return ERR_BUSY      : Activation is ongoing
     * \return ERR_LINK_LOSS : Remote Field was turned off
     *****************************************************************************
     */
    ReturnCode rfalNfcDepListenGetActivationStatus(void);

    /*!
     *****************************************************************************
     * \brief Start Transceive
     *
     * Transceives a complete or partial DEP block
     *
     * The txBuf contains complete or partial of DEP to be transmitted.
     * The Prologue field of the I-PDU is handled internally
     *
     * If the buffer contains partial LLCP and is not the last block, then
     * isTxChaining must be set to true
     *
     * \param[in] param: reference parameters to be used for the Transceive
     *
     * \return ERR_PARAM       : Bad request
     * \return ERR_WRONG_STATE : The module is not in a proper state
     * \return ERR_NONE        : The Transceive request has been started
     *****************************************************************************
     */
    ReturnCode rfalNfcDepStartTransceive(const rfalNfcDepTxRxParam *param);


    /*!
     *****************************************************************************
     * \brief Return the Transceive status
     *
     * Returns the status of the NFC-DEP Transceive
     *
     * \warning  When the other device is performing chaining once a chained
     *            block is received the error ERR_AGAIN is sent. At this point
     *            caller must handle the received data immediately.
     *            When ERR_AGAIN is returned an ACK has already been sent to
     *            the other device and the next block might be incoming.
     *            If rfalWorker() is called frequently it will place the next
     *            block on the given buffer
     *
     * \return ERR_NONE      : Transceive has been completed successfully
     * \return ERR_BUSY      : Transceive is ongoing
     * \return ERR_PROTO     : Protocol error occurred
     * \return ERR_TIMEOUT   : Timeout error occurred
     * \return ERR_SLEEP_REQ : Deselect has been received and responded
     * \return ERR_NOMEM     : The received I-PDU does not fit into the
     *                            receive buffer
     * \return ERR_LINK_LOSS : Communication is lost because Reader/Writer
     *                            has turned off its field
     * \return ERR_AGAIN     : received one chaining block, continue to call
     *                            this method to retrieve the remaining blocks
     *****************************************************************************
     */
    ReturnCode rfalNfcDepGetTransceiveStatus(void);


    /*!
    *****************************************************************************
    * \brief Start PDU Transceive
    *
    * This method triggers a NFC-DEP Transceive containing a complete PDU
    * It transmits the given message and handles all protocol retransmitions,
    * error handling and control messages
    *
    * The txBuf  contains a complete PDU to be transmitted
    * The Prologue field will be manipulated by the Transceive
    *
    * \warning the txBuf will be modified during the transmission
    * \warning the maximum RF frame which can be received is limited by param.tmpBuf
    *
    * \param[in] param: reference parameters to be used for the Transceive
    *
    * \return ERR_PARAM       : Bad request
    * \return ERR_WRONG_STATE : The module is not in a proper state
    * \return ERR_NONE        : The Transceive request has been started
    *****************************************************************************
    */
    ReturnCode rfalNfcDepStartPduTransceive(rfalNfcDepPduTxRxParam param);


    /*!
    *****************************************************************************
    * \brief Return the PDU Transceive status
    *
    * Returns the status of the NFC-DEP PDU Transceive
    *
    *
    * \return ERR_NONE      : Transceive has been completed successfully
    * \return ERR_BUSY      : Transceive is ongoing
    * \return ERR_PROTO     : Protocol error occurred
    * \return ERR_TIMEOUT   : Timeout error occurred
    * \return ERR_SLEEP_REQ : Deselect has been received and responded
    * \return ERR_NOMEM     : The received I-PDU does not fit into the
    *                            receive buffer
    * \return ERR_LINK_LOSS : Communication is lost because Reader/Writer
    *                            has turned off its field
    *****************************************************************************
    */
    ReturnCode rfalNfcDepGetPduTransceiveStatus(void);

    /*
    ******************************************************************************
    * RFAL NFC-F FUNCTION PROTOTYPES
    ******************************************************************************
    */


    /*!
     *****************************************************************************
     * \brief  Initialize NFC-F Poller mode
     *
     * This methods configures RFAL RF layer to perform as a
     * NFC-F Poller/RW (FeliCa PCD) including all default timings
     *
     * \param[in]  bitRate      : NFC-F bitrate to be initialize (212 or 424)
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
     * \return ERR_PARAM        : Incorrect bitrate
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalNfcfPollerInitialize(rfalBitRate bitRate);


    /*!
     *****************************************************************************
     *  \brief NFC-F Poller Check Presence
     *
     *  This function sends a Poll/SENSF command according to NFC Activity spec
     *  It detects if a NCF-F device is within range
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or incorrect mode
     * \return ERR_PARAM        : Invalid parameters
     * \return ERR_IO           : Generic internal error
     * \return ERR_CRC          : CRC error detected
     * \return ERR_FRAMING      : Framing error detected
     * \return ERR_PROTO        : Protocol error detected
     * \return ERR_TIMEOUT      : Timeout error, no listener device detected
     * \return ERR_NONE         : No error and some NFC-F device was detected
     *
     *****************************************************************************
     */
    ReturnCode rfalNfcfPollerCheckPresence(void);



    /*!
     *****************************************************************************
     * \brief NFC-F Poller Poll
     *
     * This function sends to all PICCs in field the POLL command with the given
     * number of slots.
     *
     * \param[in]  slots      : the number of slots to be performed
     * \param[in]  sysCode    : as given in FeliCa poll command
     * \param[in]  reqCode    : FeliCa communication parameters
     * \param[out] cardList   : Parameter of type rfalFeliCaPollRes which will hold the cards found
     * \param[out] devCnt     : actual number of cards found
     * \param[out] collisions : number of collisions encountered
     *
     * \warning the list cardList has to be as big as the number of slots for the Poll
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or incorrect mode
     * \return ERR_PARAM        : Invalid parameters
     * \return ERR_IO           : Generic internal error
     * \return ERR_CRC          : CRC error detected
     * \return ERR_FRAMING      : Framing error detected
     * \return ERR_PROTO        : Protocol error detected
     * \return ERR_TIMEOUT      : Timeout error, no listener device detected
     * \return ERR_NONE         : No error and some NFC-F device was detected
     *
     *****************************************************************************
     */
    ReturnCode rfalNfcfPollerPoll(rfalFeliCaPollSlots slots, uint16_t sysCode, uint8_t reqCode, rfalFeliCaPollRes *cardList, uint8_t *devCnt, uint8_t *collisions);


    /*!
     *****************************************************************************
     * \brief  NFC-F Poller Full Collision Resolution
     *
     * Performs a full Collision resolution as defined in Activity 1.1  9.3.4
     *
     * \param[in]  compMode    : compliance mode to be performed
     * \param[in]  devLimit    : device limit value, and size nfcaDevList
     * \param[out] nfcfDevList : NFC-F listener devices list
     * \param[out] devCnt      : Devices found counter
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
     * \return ERR_PARAM        : Invalid parameters
     * \return ERR_IO           : Generic internal error
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalNfcfPollerCollisionResolution(rfalComplianceMode compMode, uint8_t devLimit, rfalNfcfListenDevice *nfcfDevList, uint8_t *devCnt);


    /*!
     *****************************************************************************
     * \brief  NFC-F Poller Check/Read
     *
     * It computes a Check / Read command according to T3T 1.0 and JIS X6319-4 and
     * sends it to PICC. If successfully, the rxBuf will contain the the number of
     * blocks in the first byte followed by the blocks data.
     *
     * \param[in]  nfcid2      : nfcid2 of the device
     * \param[in]  servBlock   : parameter containing the list of Services and
     *                           Blocks to be addressed by this command
     * \param[out] rxBuf       : buffer to place check/read data
     * \param[in]  rxBufLen    : size of the rxBuf
     * \param[out] rcvdLen     : length of data placed in rxBuf
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
     * \return ERR_PARAM        : Invalid parameters
     * \return ERR_IO           : Generic internal error
     * \return ERR_REQUEST      : The request was executed with error
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalNfcfPollerCheck(const uint8_t *nfcid2, const rfalNfcfServBlockListParam *servBlock, uint8_t *rxBuf, uint16_t rxBufLen, uint16_t *rcvdLen);


    /*!
     *****************************************************************************
     * \brief  NFC-F Poller Update/Write
     *
     * It computes a Update / Write command according to T3T 1.0 and JIS X6319-4 and
     * sends it to PICC.
     *
     * \param[in]  nfcid2      : nfcid2 of the device
     * \param[in]  servBlock   : parameter containing the list of Services and
     *                           Blocks to be addressed by this command
     * \param[in]  txBuf       : buffer where the request will be composed
     * \param[in]  txBufLen    : size of txBuf
     * \param[in]  blockData   : data to written on the given block(s)
     * \param[out] rxBuf       : buffer to place check/read data
     * \param[in]  rxBufLen    : size of the rxBuf
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
     * \return ERR_PARAM        : Invalid parameters
     * \return ERR_IO           : Generic internal error
     * \return ERR_REQUEST      : The request was executed with error
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalNfcfPollerUpdate(const uint8_t *nfcid2, const rfalNfcfServBlockListParam *servBlock, uint8_t *txBuf, uint16_t txBufLen, const uint8_t *blockData, uint8_t *rxBuf, uint16_t rxBufLen);

    /*!
     *****************************************************************************
     * \brief NFC-F Listener is T3T Request
     *
     * This method checks if the given data is a valid T3T command (Read or Write)
     * and in case a valid request has been received it may output the request's NFCID2
     *
     * \param[in]   buf : buffer holding Initiator's received command
     * \param[in]   bufLen : length of received command in bytes
     * \param[out]  nfcid2 : pointer to where the NFCID2 may be outputted,
     *                       nfcid2 has NFCF_SENSF_NFCID2_LEN as length
     *                       Pass NULL if output parameter not desired
     *
     * \return true  : Valid T3T command (Read or Write) received
     * \return false : Invalid protocol request
     *
     *****************************************************************************
     */
    bool rfalNfcfListenerIsT3TReq(const uint8_t *buf, uint16_t bufLen, uint8_t *nfcid2);


    /*
    ******************************************************************************
    * RFAL NFC-V FUNCTION PROTOTYPES
    ******************************************************************************
    */


    /*!
     *****************************************************************************
     * \brief  Initialize NFC-V Poller mode
     *
     * This methods configures RFAL RF layer to perform as a
     * NFC-F Poller/RW (ISO15693) including all default timings
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
     * \return ERR_PARAM        : Incorrect bitrate
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalNfcvPollerInitialize(void);

    /*!
     *****************************************************************************
     * \brief  NFC-V Poller Check Presence
     *
     * This method checks if a NFC-V Listen device (VICC) is present on the field
     * by sending an Inventory (INVENTORY_REQ)
     *
     * \param[out] invRes : If received, the INVENTORY_RES
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or incorrect mode
     * \return ERR_PARAM        : Invalid parameters
     * \return ERR_IO           : Generic internal error
     * \return ERR_TIMEOUT      : Timeout error, no listener device detectedd
     * \return ERR_NONE         : No error, one or more device in the field
     *****************************************************************************
     */
    ReturnCode rfalNfcvPollerCheckPresence(rfalNfcvInventoryRes *invRes);

    /*!
     *****************************************************************************
     * \brief NFC-F Poller Poll
     *
     * This function sends to all VICCs in field the INVENTORY command with the
     * given number of slots
     *
     * If more than one slot is used the following EOF need to be handled
     * by the caller using rfalISO15693TransceiveEOFAnticollision()
     *
     * \param[in]  nSlots  : Number of Slots to be sent (1 or 16)
     * \param[in]  maskLen : Number bits on the Mask value
     * \param[in]  maskVal : location of the Mask value
     * \param[out] invRes  : location to place the INVENTORY_RES
     * \param[out] rcvdLen : number of bits received (without collision)
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or incorrect mode
     * \return ERR_PARAM        : Invalid parameters
     * \return ERR_IO           : Generic internal error
     * \return ERR_RF_COLLISION : Collision detected
     * \return ERR_CRC          : CRC error detected
     * \return ERR_PROTO        : Protocol error detected
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalNfcvPollerInventory(rfalNfcvNumSlots nSlots, uint8_t maskLen, const uint8_t *maskVal, rfalNfcvInventoryRes *invRes, uint16_t *rcvdLen);

    /*!
     *****************************************************************************
     * \brief  NFC-V Poller Full Collision Resolution
     *
     * Performs a full Collision resolution as defined in Activity 2.0   9.3.7
     * Once done, the devCnt will indicate how many (if any) devices have
     * been identified and their details are contained on nfcvDevList
     *
     * \param[in]  compMode     : compliance mode to be performed
     * \param[in]  devLimit     : device limit value, and size nfcaDevList
     * \param[out] nfcvDevList  : NFC-v listener devices list
     * \param[out] devCnt       : Devices found counter
     *
     * When compMode is set to ISO the function immediately goes to 16 slots improving
     * chances to detect more than only one strong card.
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
     * \return ERR_PARAM        : Invalid parameters
     * \return ERR_IO           : Generic internal error
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalNfcvPollerCollisionResolution(rfalComplianceMode compMode, uint8_t devLimit, rfalNfcvListenDevice *nfcvDevList, uint8_t *devCnt);

    /*!
     *****************************************************************************
     * \brief  NFC-V Poller Full Collision Resolution With Sleep
     *
     * Performs a full Collision resolution which is different from Activity 2.0 9.3.7.
     * The implementation uses SLPV (StayQuiet) command to make sure all cards are found.
     * Once done, the devCnt will indicate how many (if any) devices have
     * been identified and their details are contained on nfcvDevList
     *
     * \param[in]  devLimit     : device limit value, and size nfcaDevList
     * \param[out] nfcvDevList  : NFC-v listener devices list
     * \param[out] devCnt       : Devices found counter
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
     * \return ERR_PARAM        : Invalid parameters
     * \return ERR_IO           : Generic internal error
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalNfcvPollerSleepCollisionResolution(uint8_t devLimit, rfalNfcvListenDevice *nfcvDevList, uint8_t *devCnt);

    /*!
     *****************************************************************************
     * \brief  NFC-V Poller Sleep
     *
     * This function is used to send the SLPV_REQ (Stay Quiet) command to put the VICC
     * with the given UID to state QUIET so that they do not reply to more Inventory
     *
     * \param[in]  flags        : Flags to be used: Sub-carrier; Data_rate; Option
     *                            for NFC-Forum use: RFAL_NFCV_REQ_FLAG_DEFAULT
     * \param[in]  uid          : UID of the device to be put to Sleep
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or incorrect mode
     * \return ERR_PARAM        : Invalid parameters
     * \return ERR_IO           : Generic internal error
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalNfcvPollerSleep(uint8_t flags, const uint8_t *uid);

    /*!
     *****************************************************************************
     * \brief  NFC-V Poller Select
     *
     * Selects a device (VICC) by its UID
     *
     * \param[in]  flags        : Flags to be used: Sub-carrier; Data_rate; Option
     *                            for NFC-Forum use: RFAL_NFCV_REQ_FLAG_DEFAULT
     * \param[in]  uid          : UID of the device to be put to be Selected
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or incorrect mode
     * \return ERR_PARAM        : Invalid parameters
     * \return ERR_IO           : Generic internal error
     * \return ERR_CRC          : CRC error detected
     * \return ERR_FRAMING      : Framing error detected
     * \return ERR_PROTO        : Protocol error detected
     * \return ERR_TIMEOUT      : Timeout error
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalNfcvPollerSelect(uint8_t flags, const uint8_t *uid);

    /*!
     *****************************************************************************
     * \brief  NFC-V Poller Read Single Block
     *
     * Reads a Single Block from a device (VICC)
     *
     * \param[in]  flags        : Flags to be used: Sub-carrier; Data_rate; Option
     *                            for NFC-Forum use: RFAL_NFCV_REQ_FLAG_DEFAULT
     * \param[in]  uid          : UID of the device to be put to be read
     *                             if not provided Select mode will be used
     * \param[in]  blockNum     : Number of the block to read
     * \param[out] rxBuf        : buffer to store response (also with RES_FLAGS)
     * \param[in]  rxBufLen     : length of rxBuf
     * \param[out] rcvLen       : number of bytes received
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or incorrect mode
     * \return ERR_PARAM        : Invalid parameters
     * \return ERR_IO           : Generic internal error
     * \return ERR_CRC          : CRC error detected
     * \return ERR_FRAMING      : Framing error detected
     * \return ERR_PROTO        : Protocol error detected
     * \return ERR_TIMEOUT      : Timeout error
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalNfcvPollerReadSingleBlock(uint8_t flags, const uint8_t *uid, uint8_t blockNum, uint8_t *rxBuf, uint16_t rxBufLen, uint16_t *rcvLen);

    /*!
     *****************************************************************************
     * \brief  NFC-V Poller Write Single Block
     *
     * Writes a Single Block from a device (VICC)
     *
     * \param[in]  flags        : Flags to be used: Sub-carrier; Data_rate; Option
     *                            for NFC-Forum use: RFAL_NFCV_REQ_FLAG_DEFAULT
     * \param[in]  uid          : UID of the device to be put to be written
     *                             if not provided Select mode will be used
     * \param[in]  blockNum     : Number of the block to write
     * \param[in]  wrData       : data to be written on the given block
     * \param[in]  blockLen     : number of bytes of a block
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or incorrect mode
     * \return ERR_PARAM        : Invalid parameters
     * \return ERR_IO           : Generic internal error
     * \return ERR_CRC          : CRC error detected
     * \return ERR_FRAMING      : Framing error detected
     * \return ERR_PROTO        : Protocol error detected
     * \return ERR_TIMEOUT      : Timeout error
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalNfcvPollerWriteSingleBlock(uint8_t flags, const uint8_t *uid, uint8_t blockNum, const uint8_t *wrData, uint8_t blockLen);

    /*!
     *****************************************************************************
     * \brief  NFC-V Poller Read Multiple Blocks
     *
     * Reads Multiple Blocks from a device (VICC)
     *
     * \param[in]  flags          : Flags to be used: Sub-carrier; Data_rate; Option
     *                              for NFC-Forum use: RFAL_NFCV_REQ_FLAG_DEFAULT
     * \param[in]  uid            : UID of the device to be put to be read
     *                               if not provided Select mode will be used
     * \param[in]  firstBlockNum  : first block to be read
     * \param[in]  numOfBlocks    : number of block to read
     * \param[out] rxBuf          : buffer to store response (also with RES_FLAGS)
     * \param[in]  rxBufLen       : length of rxBuf
     * \param[out] rcvLen         : number of bytes received
     *
     * \return ERR_WRONG_STATE    : RFAL not initialized or incorrect mode
     * \return ERR_PARAM          : Invalid parameters
     * \return ERR_IO             : Generic internal error
     * \return ERR_CRC            : CRC error detected
     * \return ERR_FRAMING        : Framing error detected
     * \return ERR_PROTO          : Protocol error detected
     * \return ERR_TIMEOUT        : Timeout error
     * \return ERR_NONE           : No error
     *****************************************************************************
     */
    ReturnCode rfalNfcvPollerReadMultipleBlocks(uint8_t flags, const uint8_t *uid, uint8_t firstBlockNum, uint8_t numOfBlocks, uint8_t *rxBuf, uint16_t rxBufLen, uint16_t *rcvLen);

    /*!
     *****************************************************************************
     * \brief  NFC-V Poller Write Multiple Blocks
     *
     * Reads Multiple Blocks from a device (VICC)
     * In order to not limit the length of the Write multiple command, a buffer
     * must be provided where the request will be composed and then sent.
     *
     * \param[in]  flags          : Flags to be used: Sub-carrier; Data_rate; Option
     *                              for NFC-Forum use: RFAL_NFCV_REQ_FLAG_DEFAULT
     * \param[in]  uid            : UID of the device to be put to be read
     *                               if not provided Select mode will be used
     * \param[in]  firstBlockNum  : first block to be write
     * \param[in]  numOfBlocks    : number of consecutive blocks to write
     * \param[in]  txBuf          : buffer where the request will be composed
     * \param[in]  txBufLen       : length of txBuf
     * \param[in]  blockLen       : number of bytes of a block
     * \param[in]  wrData         : data to be written
     * \param[in]  wrDataLen      : length of the data do be written. Must be
     *                              aligned with number of blocks to write and
     *                              the size of a block
     *
     * \return ERR_WRONG_STATE    : RFAL not initialized or incorrect mode
     * \return ERR_PARAM          : Invalid parameters
     * \return ERR_IO             : Generic internal error
     * \return ERR_CRC            : CRC error detected
     * \return ERR_FRAMING        : Framing error detected
     * \return ERR_PROTO          : Protocol error detected
     * \return ERR_TIMEOUT        : Timeout error
     * \return ERR_NONE           : No error
     *****************************************************************************
     */
    ReturnCode rfalNfcvPollerWriteMultipleBlocks(uint8_t flags, const uint8_t *uid, uint8_t firstBlockNum, uint8_t numOfBlocks, uint8_t *txBuf, uint16_t txBufLen, uint8_t blockLen, const uint8_t *wrData, uint16_t wrDataLen);

    /*!
     *****************************************************************************
     * \brief  NFC-V Poller Extended Lock Single Block
     *
     * Blocks a Single Block from a device (VICC) supporting extended commands
     *
     * \param[in]  flags        : Flags to be used: Sub-carrier; Data_rate; Option
     *                            for NFC-Forum use: RFAL_NFCV_REQ_FLAG_DEFAULT
     * \param[in]  uid          : UID of the device
     *                             if not provided Select mode will be used
     * \param[in]  blockNum     : Number of the block to be locked
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or incorrect mode
     * \return ERR_PARAM        : Invalid parameters
     * \return ERR_IO           : Generic internal error
     * \return ERR_CRC          : CRC error detected
     * \return ERR_FRAMING      : Framing error detected
     * \return ERR_PROTO        : Protocol error detected
     * \return ERR_TIMEOUT      : Timeout error
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalNfcvPollerLockBlock(uint8_t flags, const uint8_t *uid, uint8_t blockNum);

    /*!
     *****************************************************************************
     * \brief  NFC-V Poller Extended Lock Single Block
     *
     * Blocks a Single Block from a device (VICC) supporting extended commands
     *
     * \param[in]  flags        : Flags to be used: Sub-carrier; Data_rate; Option
     *                            for NFC-Forum use: RFAL_NFCV_REQ_FLAG_DEFAULT
     * \param[in]  uid          : UID of the device
     *                             if not provided Select mode will be used
     * \param[in]  blockNum     : Number of the block to be locked (16 bits)
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or incorrect mode
     * \return ERR_PARAM        : Invalid parameters
     * \return ERR_IO           : Generic internal error
     * \return ERR_CRC          : CRC error detected
     * \return ERR_FRAMING      : Framing error detected
     * \return ERR_PROTO        : Protocol error detected
     * \return ERR_TIMEOUT      : Timeout error
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalNfcvPollerExtendedLockSingleBlock(uint8_t flags, const uint8_t *uid, uint16_t blockNum);

    /*!
     *****************************************************************************
     * \brief  NFC-V Poller Extended Read Single Block
     *
     * Reads a Single Block from a device (VICC) supporting extended commands
     *
     * \param[in]  flags        : Flags to be used: Sub-carrier; Data_rate; Option
     *                            for NFC-Forum use: RFAL_NFCV_REQ_FLAG_DEFAULT
     * \param[in]  uid          : UID of the device to be put to be read
     *                             if not provided Select mode will be used
     * \param[in]  blockNum     : Number of the block to read (16 bits)
     * \param[out] rxBuf        : buffer to store response (also with RES_FLAGS)
     * \param[in]  rxBufLen     : length of rxBuf
     * \param[out] rcvLen       : number of bytes received
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or incorrect mode
     * \return ERR_PARAM        : Invalid parameters
     * \return ERR_IO           : Generic internal error
     * \return ERR_CRC          : CRC error detected
     * \return ERR_FRAMING      : Framing error detected
     * \return ERR_PROTO        : Protocol error detected
     * \return ERR_TIMEOUT      : Timeout error
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalNfcvPollerExtendedReadSingleBlock(uint8_t flags, const uint8_t *uid, uint16_t blockNum, uint8_t *rxBuf, uint16_t rxBufLen, uint16_t *rcvLen);

    /*!
     *****************************************************************************
     * \brief  NFC-V Poller Extended Write Single Block
     *
     * Writes a Single Block from a device (VICC) supporting extended commands
     *
     * \param[in]  flags        : Flags to be used: Sub-carrier; Data_rate; Option
     *                            for NFC-Forum use: RFAL_NFCV_REQ_FLAG_DEFAULT
     * \param[in]  uid          : UID of the device
     *                             if not provided Select mode will be used
     * \param[in]  blockNum     : Number of the block to write (16 bits)
     * \param[in]  wrData       : data to be written on the given block
     * \param[in]  blockLen     : number of bytes of a block
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or incorrect mode
     * \return ERR_PARAM        : Invalid parameters
     * \return ERR_IO           : Generic internal error
     * \return ERR_CRC          : CRC error detected
     * \return ERR_FRAMING      : Framing error detected
     * \return ERR_PROTO        : Protocol error detected
     * \return ERR_TIMEOUT      : Timeout error
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalNfcvPollerExtendedWriteSingleBlock(uint8_t flags, const uint8_t *uid, uint16_t blockNum, const uint8_t *wrData, uint8_t blockLen);

    /*!
     *****************************************************************************
     * \brief  NFC-V Poller Extended Read Multiple Blocks
     *
     * Reads Multiple Blocks from a device (VICC) supporting extended commands
     *
     * \param[in]  flags          : Flags to be used: Sub-carrier; Data_rate; Option
     *                              for NFC-Forum use: RFAL_NFCV_REQ_FLAG_DEFAULT
     * \param[in]  uid            : UID of the device to be put to be read
     *                               if not provided Select mode will be used
     * \param[in]  firstBlockNum  : first block to be read (16 bits)
     * \param[in]  numOfBlocks    : number of consecutive blocks to read (16 bits)
     * \param[out] rxBuf          : buffer to store response (also with RES_FLAGS)
     * \param[in]  rxBufLen       : length of rxBuf
     * \param[out] rcvLen         : number of bytes received
     *
     * \return ERR_WRONG_STATE    : RFAL not initialized or incorrect mode
     * \return ERR_PARAM          : Invalid parameters
     * \return ERR_IO             : Generic internal error
     * \return ERR_CRC            : CRC error detected
     * \return ERR_FRAMING        : Framing error detected
     * \return ERR_PROTO          : Protocol error detected
     * \return ERR_TIMEOUT        : Timeout error
     * \return ERR_NONE           : No error
     *****************************************************************************
     */
    ReturnCode rfalNfcvPollerExtendedReadMultipleBlocks(uint8_t flags, const uint8_t *uid, uint16_t firstBlockNum, uint16_t numOfBlocks, uint8_t *rxBuf, uint16_t rxBufLen, uint16_t *rcvLen);

    /*!
     *****************************************************************************
     * \brief  NFC-V Poller Extended Write Multiple Blocks
     *
     * Writes Multiple Blocks from a device (VICC) supporting extended commands
     * In order to not limit the length of the Write multiple command, a buffer
     * must be provided where the request will be composed and then sent.
     *
     * \param[in]  flags          : Flags to be used: Sub-carrier; Data_rate; Option
     *                              for NFC-Forum use: RFAL_NFCV_REQ_FLAG_DEFAULT
     * \param[in]  uid            : UID of the device to be put to be read
     *                               if not provided Select mode will be used
     * \param[in]  firstBlockNum  : first block to be write (16 bits)
     * \param[in]  numOfBlocks    : number of consecutive blocks to write (16 bits)
     * \param[in]  txBuf          : buffer where the request will be composed
     * \param[in]  txBufLen       : length of txBuf
     * \param[in]  blockLen       : number of bytes of a block
     * \param[in]  wrData         : data to be written
     * \param[in]  wrDataLen      : length of the data do be written. Must be
     *                              aligned with number of blocks to write and
     *                              the size of a block
     *
     * \return ERR_WRONG_STATE    : RFAL not initialized or incorrect mode
     * \return ERR_PARAM          : Invalid parameters
     * \return ERR_IO             : Generic internal error
     * \return ERR_CRC            : CRC error detected
     * \return ERR_FRAMING        : Framing error detected
     * \return ERR_PROTO          : Protocol error detected
     * \return ERR_TIMEOUT        : Timeout error
     * \return ERR_NONE           : No error
     *****************************************************************************
     */
    ReturnCode rfalNfcvPollerExtendedWriteMultipleBlocks(uint8_t flags, const uint8_t *uid, uint16_t firstBlockNum, uint16_t numOfBlocks, uint8_t *txBuf, uint16_t txBufLen, uint8_t blockLen, const uint8_t *wrData, uint16_t wrDataLen);

    /*!
     *****************************************************************************
     * \brief  NFC-V Get System Information
     *
     * Sends Get System Information command
     *
     * \param[in]  flags          : Flags to be used: Sub-carrier; Data_rate; Option
     *                              for NFC-Forum use: RFAL_NFCV_REQ_FLAG_DEFAULT
     * \param[in]  uid            : UID of the device to be put to be read
     *                               if not provided Select mode will be used
     * \param[out] rxBuf          : buffer to store response (also with RES_FLAGS)
     * \param[in]  rxBufLen       : length of rxBuf
     * \param[out] rcvLen         : number of bytes received
     *
     * \return ERR_WRONG_STATE    : RFAL not initialized or incorrect mode
     * \return ERR_PARAM          : Invalid parameters
     * \return ERR_IO             : Generic internal error
     * \return ERR_CRC            : CRC error detected
     * \return ERR_FRAMING        : Framing error detected
     * \return ERR_PROTO          : Protocol error detected
     * \return ERR_TIMEOUT        : Timeout error
     * \return ERR_NONE           : No error
     *****************************************************************************
     */
    ReturnCode rfalNfcvPollerGetSystemInformation(uint8_t flags, const uint8_t *uid, uint8_t *rxBuf, uint16_t rxBufLen, uint16_t *rcvLen);

    /*!
     *****************************************************************************
     * \brief  NFC-V Extended Get System Information
     *
     * Sends Extended Get System Information command
     *
     * \param[in]  flags          : Flags to be used: Sub-carrier; Data_rate; Option
     *                              for NFC-Forum use: RFAL_NFCV_REQ_FLAG_DEFAULT
     * \param[in]  uid            : UID of the device to be put to be read
     *                               if not provided Select mode will be used
     * \param[in]  requestField   : Get System info parameter request field
     * \param[out] rxBuf          : buffer to store response (also with RES_FLAGS)
     * \param[in]  rxBufLen       : length of rxBuf
     * \param[out] rcvLen         : number of bytes received
     *
     * \return ERR_WRONG_STATE    : RFAL not initialized or incorrect mode
     * \return ERR_PARAM          : Invalid parameters
     * \return ERR_IO             : Generic internal error
     * \return ERR_CRC            : CRC error detected
     * \return ERR_FRAMING        : Framing error detected
     * \return ERR_PROTO          : Protocol error detected
     * \return ERR_TIMEOUT        : Timeout error
     * \return ERR_NONE           : No error
     *****************************************************************************
     */
    ReturnCode rfalNfcvPollerExtendedGetSystemInformation(uint8_t flags, const uint8_t *uid, uint8_t requestField, uint8_t *rxBuf, uint16_t rxBufLen, uint16_t *rcvLen);


    /*!
     *****************************************************************************
     * \brief  NFC-V Transceive Request
     *
     * Performs a generic transceive with an ISO15693 tag
     *
     * \param[in]  cmd            : NFC-V command
     * \param[in]  flags          : Flags to be used: Sub-carrier; Data_rate; Option
     *                              for NFC-Forum use: RFAL_NFCV_REQ_FLAG_DEFAULT
     * \param[in]  param          : Prepend parameter on certain proprietary requests
     *                              For default commands skip: RFAL_NFCV_PARAM_SKIP
     * \param[in]  uid            : UID of the device to be put to be read
     *                               if not provided Select mode will be used
     * \param[in]  data           : command parameters append after UID
     * \param[in]  dataLen        : command parameters Len
     * \param[out] rxBuf          : buffer to store response (also with RES_FLAGS)
     * \param[in]  rxBufLen       : length of rxBuf
     * \param[out] rcvLen         : number of bytes received
     *
     * \return ERR_WRONG_STATE    : RFAL not initialized or incorrect mode
     * \return ERR_PARAM          : Invalid parameters
     * \return ERR_IO             : Generic internal error
     * \return ERR_CRC            : CRC error detected
     * \return ERR_FRAMING        : Framing error detected
     * \return ERR_PROTO          : Protocol error detected
     * \return ERR_TIMEOUT        : Timeout error
     * \return ERR_NONE           : No error
     *****************************************************************************
     */
    ReturnCode rfalNfcvPollerTransceiveReq(uint8_t cmd, uint8_t flags, uint8_t param, const uint8_t *uid, const uint8_t *data, uint16_t dataLen, uint8_t *rxBuf, uint16_t rxBufLen, uint16_t *rcvLen);


    /*
    ******************************************************************************
    * RFAL ST25TB FUNCTION PROTOTYPES
    ******************************************************************************
    */


    /*!
     *****************************************************************************
     * \brief  Initialize ST25TB Poller mode
     *
     * This methods configures RFAL RF layer to perform as a
     * ST25TB Poller/RW including all default timings
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalSt25tbPollerInitialize(void);


    /*!
     *****************************************************************************
     * \brief  ST25TB Poller Check Presence
     *
     * This method checks if a ST25TB Listen device (PICC) is present on the field
     * by sending an Initiate command
     *
     * \param[out] chipId : if successfully retrieved, the device's chip ID
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or incorrect mode
     * \return ERR_PARAM        : Invalid parameters
     * \return ERR_IO           : Generic internal error
     * \return ERR_TIMEOUT      : Timeout error, no listener device detected
     * \return ERR_RF_COLLISION : Collision detected one or more device in the field
     * \return ERR_PROTO        : Protocol error detected
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalSt25tbPollerCheckPresence(uint8_t *chipId);


    /*!
     *****************************************************************************
     * \brief  ST25TB Poller Collision Resolution
     *
     * This method performs ST25TB Collision resolution, selects the each device,
     * retrieves its UID and then deselects.
     * In case only one device is identified the ST25TB device is left in select
     * state.
     *
     * \param[in]  devLimit      : device limit value, and size st25tbDevList
     * \param[out] st25tbDevList : ST35TB listener device info
     * \param[out] devCnt        : Devices found counter
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or incorrect mode
     * \return ERR_PARAM        : Invalid parameters
     * \return ERR_IO           : Generic internal error
     * \return ERR_TIMEOUT      : Timeout error, no listener device detected
     * \return ERR_RF_COLLISION : Collision detected one or more device in the field
     * \return ERR_PROTO        : Protocol error detected
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalSt25tbPollerCollisionResolution(uint8_t devLimit, rfalSt25tbListenDevice *st25tbDevList, uint8_t *devCnt);

    /*!
     *****************************************************************************
     * \brief  ST25TB Poller Initiate
     *
     * This method sends an Initiate command
     *
     * If a single device responds the chip ID will be retrieved
     *
     * \param[out]  chipId      : chip ID of the device
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or incorrect mode
     * \return ERR_PARAM        : Invalid parameters
     * \return ERR_IO           : Generic internal error
     * \return ERR_TIMEOUT      : Timeout error, no listener device detected
     * \return ERR_PROTO        : Protocol error detected
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalSt25tbPollerInitiate(uint8_t *chipId);


    /*!
     *****************************************************************************
     * \brief  ST25TB Poller Pcall
     *
     * This method sends a Pcall command
     * If successful the device's chip ID will be retrieved
     *
     * \param[out]  chipId      : Chip ID of the device
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or incorrect mode
     * \return ERR_PARAM        : Invalid parameters
     * \return ERR_IO           : Generic internal error
     * \return ERR_TIMEOUT      : Timeout error, no listener device detected
     * \return ERR_PROTO        : Protocol error detected
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalSt25tbPollerPcall(uint8_t *chipId);


    /*!
     *****************************************************************************
     * \brief  ST25TB Poller Slot Marker
     *
     * This method sends a Slot Marker
     *
     * If a single device responds the chip ID will be retrieved
     *
     * \param[in]  slotNum      : Slot Number
     * \param[out]  chipIdRes   : Chip ID of the device
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or incorrect mode
     * \return ERR_PARAM        : Invalid parameters
     * \return ERR_IO           : Generic internal error
     * \return ERR_TIMEOUT      : Timeout error, no listener device detected
     * \return ERR_PROTO        : Protocol error detected
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalSt25tbPollerSlotMarker(uint8_t slotNum, uint8_t *chipIdRes);


    /*!
     *****************************************************************************
     * \brief  ST25TB Poller Select
     *
     * This method sends a ST25TB Select command with the given chip ID.
     *
     * If the device is already in Selected state and receives an incorrect chip
     * ID, it goes into Deselected state
     *
     * \param[in]  chipId       : chip ID of the device to be selected
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or incorrect mode
     * \return ERR_PARAM        : Invalid parameters
     * \return ERR_IO           : Generic internal error
     * \return ERR_TIMEOUT      : Timeout error, no listener device detected
     * \return ERR_PROTO        : Protocol error detected
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalSt25tbPollerSelect(uint8_t chipId);


    /*!
     *****************************************************************************
     * \brief  ST25TB Get UID
     *
     * This method sends a Get_UID command
     *
     * If a single device responds the chip UID will be retrieved
     *
     * \param[out]  UID      : UID of the found device
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or incorrect mode
     * \return ERR_PARAM        : Invalid parameters
     * \return ERR_IO           : Generic internal error
     * \return ERR_TIMEOUT      : Timeout error, no listener device detected
     * \return ERR_PROTO        : Protocol error detected
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalSt25tbPollerGetUID(rfalSt25tbUID *UID);


    /*!
     *****************************************************************************
     * \brief  ST25TB Poller Read Block
     *
     * This method reads a block of the ST25TB
     *
     * \param[in]   blockAddress : address of the block to be read
     * \param[out]  blockData    : location to place the data read from block
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or incorrect mode
     * \return ERR_PARAM        : Invalid parameters
     * \return ERR_IO           : Generic internal error
     * \return ERR_TIMEOUT      : Timeout error, no listener device detected
     * \return ERR_PROTO        : Protocol error detected
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalSt25tbPollerReadBlock(uint8_t blockAddress, rfalSt25tbBlock *blockData);


    /*!
     *****************************************************************************
     * \brief  ST25TB Poller Write Block
     *
     * This method writes a block of the ST25TB
     *
     * \param[in]  blockAddress : address of the block to be written
     * \param[in]  blockData    : data to be written on the block
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or incorrect mode
     * \return ERR_PARAM        : Invalid parameters
     * \return ERR_IO           : Generic internal error
     * \return ERR_TIMEOUT      : Timeout error, no listener device detected
     * \return ERR_PROTO        : Protocol error detected
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalSt25tbPollerWriteBlock(uint8_t blockAddress, const rfalSt25tbBlock *blockData);


    /*!
     *****************************************************************************
     * \brief  ST25TB Poller Completion
     *
     * This method sends a completion command to the ST25TB. After the
     * completion the card no longer will reply to any command.
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or incorrect mode
     * \return ERR_PARAM        : Invalid parameters
     * \return ERR_IO           : Generic internal error
     * \return ERR_TIMEOUT      : Timeout error, no listener device detected
     * \return ERR_PROTO        : Protocol error detected, invalid SENSB_RES received
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalSt25tbPollerCompletion(void);


    /*!
     *****************************************************************************
     * \brief  ST25TB Poller Reset to Inventory
     *
     * This method sends a Reset to Inventory command to the ST25TB.
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or incorrect mode
     * \return ERR_PARAM        : Invalid parameters
     * \return ERR_IO           : Generic internal error
     * \return ERR_TIMEOUT      : Timeout error, no listener device detected
     * \return ERR_PROTO        : Protocol error detected, invalid SENSB_RES received
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalSt25tbPollerResetToInventory(void);


    /*
    ******************************************************************************
    * RFAL ST25XV FUNCTION PROTOTYPES
    ******************************************************************************
    */


    /*!
     *****************************************************************************
     * \brief  NFC-V Poller Read Single Block (M24LR)
     *
     * Reads a Single Block from a M24LR tag which has the number of blocks
     * bigger than 256 (M24LR16 ; M24LR64)
     *
     * \param[in]  flags        : Flags to be used: Sub-carrier; Data_rate; Option
     *                            default: RFAL_NFCV_REQ_FLAG_DEFAULT
     * \param[in]  uid          : UID of the device to be put to be read
     *                             if not provided Select mode will be used
     * \param[in]  blockNum     : Number of the block to read (16 bits)
     * \param[out] rxBuf        : buffer to store response (also with RES_FLAGS)
     * \param[in]  rxBufLen     : length of rxBuf
     * \param[out] rcvLen       : number of bytes received
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or incorrect mode
     * \return ERR_PARAM        : Invalid parameters
     * \return ERR_IO           : Generic internal error
     * \return ERR_CRC          : CRC error detected
     * \return ERR_FRAMING      : Framing error detected
     * \return ERR_PROTO        : Protocol error detected
     * \return ERR_TIMEOUT      : Timeout error
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalST25xVPollerM24LRReadSingleBlock(uint8_t flags, const uint8_t *uid, uint16_t blockNum, uint8_t *rxBuf, uint16_t rxBufLen, uint16_t *rcvLen);

    /*!
     *****************************************************************************
     * \brief  NFC-V Poller Fast Read Single Block (M24LR)
     *
     * Reads a Single Block from a M24LR tag which has the number of blocks
     * bigger than 256 (M24LR16 ; M24LR64) using ST Fast mode
     *
     * \param[in]  flags        : Flags to be used: Sub-carrier; Data_rate; Option
     *                            default: RFAL_NFCV_REQ_FLAG_DEFAULT
     * \param[in]  uid          : UID of the device to be put to be read
     *                             if not provided Select mode will be used
     * \param[in]  blockNum     : Number of the block to read (16 bits)
     * \param[out] rxBuf        : buffer to store response (also with RES_FLAGS)
     * \param[in]  rxBufLen     : length of rxBuf
     * \param[out] rcvLen       : number of bytes received
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or incorrect mode
     * \return ERR_PARAM        : Invalid parameters
     * \return ERR_IO           : Generic internal error
     * \return ERR_CRC          : CRC error detected
     * \return ERR_FRAMING      : Framing error detected
     * \return ERR_PROTO        : Protocol error detected
     * \return ERR_TIMEOUT      : Timeout error
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalST25xVPollerM24LRFastReadSingleBlock(uint8_t flags, const uint8_t *uid, uint16_t blockNum, uint8_t *rxBuf, uint16_t rxBufLen, uint16_t *rcvLen);

    /*!
     *****************************************************************************
     * \brief  NFC-V Poller Write Single Block (M24LR)
     *
     * Writes a Single Block from a M24LR tag which has the number of blocks
     * bigger than 256 (M24LR16 ; M24LR64)
     *
     * \param[in]  flags        : Flags to be used: Sub-carrier; Data_rate; Option
     *                            for NFC-Forum use: RFAL_NFCV_REQ_FLAG_DEFAULT
     * \param[in]  uid          : UID of the device to be put to be written
     *                             if not provided Select mode will be used
     * \param[in]  blockNum     : Number of the block to write (16 bits)
     * \param[in]  wrData       : data to be written on the given block
     * \param[in]  blockLen     : number of bytes of a block
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or incorrect mode
     * \return ERR_PARAM        : Invalid parameters
     * \return ERR_IO           : Generic internal error
     * \return ERR_CRC          : CRC error detected
     * \return ERR_FRAMING      : Framing error detected
     * \return ERR_PROTO        : Protocol error detected
     * \return ERR_TIMEOUT      : Timeout error
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalST25xVPollerM24LRWriteSingleBlock(uint8_t flags, const uint8_t *uid, uint16_t blockNum, const uint8_t *wrData, uint8_t blockLen);

    /*!
     *****************************************************************************
     * \brief  NFC-V Poller Read Multiple Blocks (M24LR)
     *
     * Reads Multiple Blocks from a device from a M24LR tag which has the number of blocks
     * bigger than 256 (M24LR16 ; M24LR64)
     *
     * \param[in]  flags          : Flags to be used: Sub-carrier; Data_rate; Option
     *                              for NFC-Forum use: RFAL_NFCV_REQ_FLAG_DEFAULT
     * \param[in]  uid            : UID of the device to be put to be read
     *                               if not provided Select mode will be used
     * \param[in]  firstBlockNum  : first block to be read (16 bits)
     * \param[in]  numOfBlocks    : number of block to read
     * \param[out] rxBuf          : buffer to store response (also with RES_FLAGS)
     * \param[in]  rxBufLen       : length of rxBuf
     * \param[out] rcvLen         : number of bytes received
     *
     * \return ERR_WRONG_STATE    : RFAL not initialized or incorrect mode
     * \return ERR_PARAM          : Invalid parameters
     * \return ERR_IO             : Generic internal error
     * \return ERR_CRC            : CRC error detected
     * \return ERR_FRAMING        : Framing error detected
     * \return ERR_PROTO          : Protocol error detected
     * \return ERR_TIMEOUT        : Timeout error
     * \return ERR_NONE           : No error
     *****************************************************************************
     */
    ReturnCode rfalST25xVPollerM24LRReadMultipleBlocks(uint8_t flags, const uint8_t *uid, uint16_t firstBlockNum, uint8_t numOfBlocks, uint8_t *rxBuf, uint16_t rxBufLen, uint16_t *rcvLen);


    /*!
     *****************************************************************************
     * \brief  NFC-V Poller Fast Read Multiple Blocks (M24LR)
     *
     * Reads Multiple Blocks from a device from a M24LR tag which has the number of blocks
     * bigger than 256 (M24LR16 ; M24LR64) using ST Fast mode
     *
     * \param[in]  flags          : Flags to be used: Sub-carrier; Data_rate; Option
     *                              for NFC-Forum use: RFAL_NFCV_REQ_FLAG_DEFAULT
     * \param[in]  uid            : UID of the device to be put to be read
     *                               if not provided Select mode will be used
     * \param[in]  firstBlockNum  : first block to be read (16 bits)
     * \param[in]  numOfBlocks    : number of block to read
     * \param[out] rxBuf          : buffer to store response (also with RES_FLAGS)
     * \param[in]  rxBufLen       : length of rxBuf
     * \param[out] rcvLen         : number of bytes received
     *
     * \return ERR_WRONG_STATE    : RFAL not initialized or incorrect mode
     * \return ERR_PARAM          : Invalid parameters
     * \return ERR_IO             : Generic internal error
     * \return ERR_CRC            : CRC error detected
     * \return ERR_FRAMING        : Framing error detected
     * \return ERR_PROTO          : Protocol error detected
     * \return ERR_TIMEOUT        : Timeout error
     * \return ERR_NONE           : No error
     *****************************************************************************
     */
    ReturnCode rfalST25xVPollerM24LRFastReadMultipleBlocks(uint8_t flags, const uint8_t *uid, uint16_t firstBlockNum, uint8_t numOfBlocks, uint8_t *rxBuf, uint16_t rxBufLen, uint16_t *rcvLen);

    /*!
     *****************************************************************************
     * \brief  NFC-V Poller Fast Read Single Block
     *
     * Reads a Single Block from a device (VICC) using ST Fast mode
     *
     * \param[in]  flags        : Flags to be used: Sub-carrier; Data_rate; Option
     *                            for NFC-Forum use: RFAL_NFCV_REQ_FLAG_DEFAULT
     * \param[in]  uid          : UID of the device to be put to be read
     *                             if not provided Select mode will be used
     * \param[in]  blockNum     : Number of the block to read
     * \param[out] rxBuf        : buffer to store response (also with RES_FLAGS)
     * \param[in]  rxBufLen     : length of rxBuf
     * \param[out] rcvLen       : number of bytes received
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or incorrect mode
     * \return ERR_PARAM        : Invalid parameters
     * \return ERR_IO           : Generic internal error
     * \return ERR_CRC          : CRC error detected
     * \return ERR_FRAMING      : Framing error detected
     * \return ERR_PROTO        : Protocol error detected
     * \return ERR_TIMEOUT      : Timeout error
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalST25xVPollerFastReadSingleBlock(uint8_t flags, const uint8_t *uid, uint8_t blockNum, uint8_t *rxBuf, uint16_t rxBufLen, uint16_t *rcvLen);

    /*!
     *****************************************************************************
     * \brief  NFC-V Poller Fast Read Multiple Blocks
     *
     * Reads Multiple Blocks from a device (VICC) using ST Fast mode
     *
     * \param[in]  flags          : Flags to be used: Sub-carrier; Data_rate; Option
     *                              for NFC-Forum use: RFAL_NFCV_REQ_FLAG_DEFAULT
     * \param[in]  uid            : UID of the device to be put to be read
     *                               if not provided Select mode will be used
     * \param[in]  firstBlockNum  : first block to be read
     * \param[in]  numOfBlocks    : number of block to read
     * \param[out] rxBuf          : buffer to store response (also with RES_FLAGS)
     * \param[in]  rxBufLen       : length of rxBuf
     * \param[out] rcvLen         : number of bytes received
     *
     * \return ERR_WRONG_STATE    : RFAL not initialized or incorrect mode
     * \return ERR_PARAM          : Invalid parameters
     * \return ERR_IO             : Generic internal error
     * \return ERR_CRC            : CRC error detected
     * \return ERR_FRAMING        : Framing error detected
     * \return ERR_PROTO          : Protocol error detected
     * \return ERR_TIMEOUT        : Timeout error
     * \return ERR_NONE           : No error
     *****************************************************************************
     */
    ReturnCode rfalST25xVPollerFastReadMultipleBlocks(uint8_t flags, const uint8_t *uid, uint8_t firstBlockNum, uint8_t numOfBlocks, uint8_t *rxBuf, uint16_t rxBufLen, uint16_t *rcvLen);

    /*!
     *****************************************************************************
     * \brief  NFC-V Poller Fast Extended Read Single Block
     *
     * Reads a Single Block from a device (VICC) supporting extended commands using ST Fast mode
     *
     * \param[in]  flags        : Flags to be used: Sub-carrier; Data_rate; Option
     *                            for NFC-Forum use: RFAL_NFCV_REQ_FLAG_DEFAULT
     * \param[in]  uid          : UID of the device to be put to be read
     *                             if not provided Select mode will be used
     * \param[in]  blockNum     : Number of the block to read (16 bits)
     * \param[out] rxBuf        : buffer to store response (also with RES_FLAGS)
     * \param[in]  rxBufLen     : length of rxBuf
     * \param[out] rcvLen       : number of bytes received
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or incorrect mode
     * \return ERR_PARAM        : Invalid parameters
     * \return ERR_IO           : Generic internal error
     * \return ERR_CRC          : CRC error detected
     * \return ERR_FRAMING      : Framing error detected
     * \return ERR_PROTO        : Protocol error detected
     * \return ERR_TIMEOUT      : Timeout error
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalST25xVPollerFastExtendedReadSingleBlock(uint8_t flags, const uint8_t *uid, uint16_t blockNum, uint8_t *rxBuf, uint16_t rxBufLen, uint16_t *rcvLen);

    /*!
     *****************************************************************************
     * \brief  NFC-V Poller Fast Extended Read Multiple Blocks
     *
     * Reads Multiple Blocks from a device (VICC) supporting extended commands using ST Fast mode
     *
     * \param[in]  flags          : Flags to be used: Sub-carrier; Data_rate; Option
     *                              for NFC-Forum use: RFAL_NFCV_REQ_FLAG_DEFAULT
     * \param[in]  uid            : UID of the device to be put to be read
     *                               if not provided Select mode will be used
     * \param[in]  firstBlockNum  : first block to be read (16 bits)
     * \param[in]  numOfBlocks    : number of consecutive blocks to read (16 bits)
     * \param[out] rxBuf          : buffer to store response (also with RES_FLAGS)
     * \param[in]  rxBufLen       : length of rxBuf
     * \param[out] rcvLen         : number of bytes received
     *
     * \return ERR_WRONG_STATE    : RFAL not initialized or incorrect mode
     * \return ERR_PARAM          : Invalid parameters
     * \return ERR_IO             : Generic internal error
     * \return ERR_CRC            : CRC error detected
     * \return ERR_FRAMING        : Framing error detected
     * \return ERR_PROTO          : Protocol error detected
     * \return ERR_TIMEOUT        : Timeout error
     * \return ERR_NONE           : No error
     *****************************************************************************
     */
    ReturnCode rfalST25xVPollerFastExtReadMultipleBlocks(uint8_t flags, const uint8_t *uid, uint16_t firstBlockNum, uint16_t numOfBlocks, uint8_t *rxBuf, uint16_t rxBufLen, uint16_t *rcvLen);

    /*!
     *****************************************************************************
     * \brief  NFC-V Poller Read Configuration
     *
     * Reads static configuration registers at the Pointer address
     *
     * \param[in]  flags          : Flags to be used: Sub-carrier; Data_rate; Option
     *                              for NFC-Forum use: RFAL_NFCV_REQ_FLAG_DEFAULT
     * \param[in]  uid            : UID of the device to be put to be read
     *                               if not provided Select mode will be used
     * \param[in]  pointer        : Pointer address
     * \param[out] regValue       : Register value
     *
     * \return ERR_WRONG_STATE    : RFAL not initialized or incorrect mode
     * \return ERR_PARAM          : Invalid parameters
     * \return ERR_IO             : Generic internal error
     * \return ERR_CRC            : CRC error detected
     * \return ERR_FRAMING        : Framing error detected
     * \return ERR_PROTO          : Protocol error detected
     * \return ERR_TIMEOUT        : Timeout error
     * \return ERR_NONE           : No error
     *****************************************************************************
     */
    ReturnCode rfalST25xVPollerReadConfiguration(uint8_t flags, const uint8_t *uid, uint8_t pointer, uint8_t *regValue);

    /*!
     *****************************************************************************
     * \brief  NFC-V Poller Write Configuration
     *
     * Writes static configuration registers at the Pointer address
     *
     * \param[in]  flags          : Flags to be used: Sub-carrier; Data_rate; Option
     *                              for NFC-Forum use: RFAL_NFCV_REQ_FLAG_DEFAULT
     * \param[in]  uid            : UID of the device to be put to be read
     *                               if not provided Select mode will be used
     * \param[in]  pointer        : Pointer address
     * \param[in]  regValue       : Register value
     *
     * \return ERR_WRONG_STATE    : RFAL not initialized or incorrect mode
     * \return ERR_PARAM          : Invalid parameters
     * \return ERR_IO             : Generic internal error
     * \return ERR_CRC            : CRC error detected
     * \return ERR_FRAMING        : Framing error detected
     * \return ERR_PROTO          : Protocol error detected
     * \return ERR_TIMEOUT        : Timeout error
     * \return ERR_NONE           : No error
     *****************************************************************************
     */
    ReturnCode rfalST25xVPollerWriteConfiguration(uint8_t flags, const uint8_t *uid, uint8_t pointer, uint8_t regValue);


    /*!
     *****************************************************************************
     * \brief  NFC-V Poller Read Dynamic Configuration
     *
     * Reads dynamic registers at the Pointer address
     *
     * \param[in]  flags          : Flags to be used: Sub-carrier; Data_rate; Option
     *                              for NFC-Forum use: RFAL_NFCV_REQ_FLAG_DEFAULT
     * \param[in]  uid            : UID of the device to be put to be read
     *                               if not provided Select mode will be used
     * \param[in]  pointer        : Pointer address
     * \param[out] regValue       : Register value
     *
     * \return ERR_WRONG_STATE    : RFAL not initialized or incorrect mode
     * \return ERR_PARAM          : Invalid parameters
     * \return ERR_IO             : Generic internal error
     * \return ERR_CRC            : CRC error detected
     * \return ERR_FRAMING        : Framing error detected
     * \return ERR_PROTO          : Protocol error detected
     * \return ERR_TIMEOUT        : Timeout error
     * \return ERR_NONE           : No error
     *****************************************************************************
     */
    ReturnCode rfalST25xVPollerReadDynamicConfiguration(uint8_t flags, const uint8_t *uid, uint8_t pointer, uint8_t *regValue);

    /*!
     *****************************************************************************
     * \brief  NFC-V Poller Write Dynamic Configuration
     *
     * Writes dynamic registers at the Pointer address
     *
     * \param[in]  flags          : Flags to be used: Sub-carrier; Data_rate; Option
     *                              for NFC-Forum use: RFAL_NFCV_REQ_FLAG_DEFAULT
     * \param[in]  uid            : UID of the device to be put to be read
     *                               if not provided Select mode will be used
     * \param[in]  pointer        : Pointer address
     * \param[in]  regValue       : Register value
     *
     * \return ERR_WRONG_STATE    : RFAL not initialized or incorrect mode
     * \return ERR_PARAM          : Invalid parameters
     * \return ERR_IO             : Generic internal error
     * \return ERR_CRC            : CRC error detected
     * \return ERR_FRAMING        : Framing error detected
     * \return ERR_PROTO          : Protocol error detected
     * \return ERR_TIMEOUT        : Timeout error
     * \return ERR_NONE           : No error
     *****************************************************************************
     */
    ReturnCode rfalST25xVPollerWriteDynamicConfiguration(uint8_t flags, const uint8_t *uid, uint8_t pointer, uint8_t regValue);

    /*!
     *****************************************************************************
     * \brief  NFC-V Poller Fast Read Dynamic Configuration
     *
     * Reads dynamic registers at the Pointer address using ST Fast mode
     *
     * \param[in]  flags          : Flags to be used: Sub-carrier; Data_rate; Option
     *                              for NFC-Forum use: RFAL_NFCV_REQ_FLAG_DEFAULT
     * \param[in]  uid            : UID of the device to be put to be read
     *                               if not provided Select mode will be used
     * \param[in]  pointer        : Pointer address
     * \param[out] regValue       : Register value
     *
     * \return ERR_WRONG_STATE    : RFAL not initialized or incorrect mode
     * \return ERR_PARAM          : Invalid parameters
     * \return ERR_IO             : Generic internal error
     * \return ERR_CRC            : CRC error detected
     * \return ERR_FRAMING        : Framing error detected
     * \return ERR_PROTO          : Protocol error detected
     * \return ERR_TIMEOUT        : Timeout error
     * \return ERR_NONE           : No error
     *****************************************************************************
     */
    ReturnCode rfalST25xVPollerFastReadDynamicConfiguration(uint8_t flags, const uint8_t *uid, uint8_t pointer, uint8_t *regValue);

    /*!
     *****************************************************************************
     * \brief  NFC-V Poller Fast Write Dynamic Configuration
     *
     * Writes dynamic registers at the Pointer address using ST Fast mode
     *
     * \param[in]  flags          : Flags to be used: Sub-carrier; Data_rate; Option
     *                              for NFC-Forum use: RFAL_NFCV_REQ_FLAG_DEFAULT
     * \param[in]  uid            : UID of the device to be put to be read
     *                               if not provided Select mode will be used
     * \param[in]  pointer        : Pointer address
     * \param[in]  regValue       : Register value
     *
     * \return ERR_WRONG_STATE    : RFAL not initialized or incorrect mode
     * \return ERR_PARAM          : Invalid parameters
     * \return ERR_IO             : Generic internal error
     * \return ERR_CRC            : CRC error detected
     * \return ERR_FRAMING        : Framing error detected
     * \return ERR_PROTO          : Protocol error detected
     * \return ERR_TIMEOUT        : Timeout error
     * \return ERR_NONE           : No error
     *****************************************************************************
     */
    ReturnCode rfalST25xVPollerFastWriteDynamicConfiguration(uint8_t flags, const uint8_t *uid, uint8_t pointer, uint8_t regValue);

    /*!
     *****************************************************************************
     * \brief  NFC-V Poller Present Password
     *
     * Sends the Present Password command
     *
     * \param[in]  flags          : Flags to be used: Sub-carrier; Data_rate; Option
     *                              for NFC-Forum use: RFAL_NFCV_REQ_FLAG_DEFAULT
     * \param[in]  uid            : UID of the device to be put to be read
     *                               if not provided Select mode will be used
     * \param[in]  pwdNum         : Password number
     * \param[in]  pwd            : Password
     * \param[in]  pwdLen         : Password length
     *
     * \return ERR_WRONG_STATE    : RFAL not initialized or incorrect mode
     * \return ERR_PARAM          : Invalid parameters
     * \return ERR_IO             : Generic internal error
     * \return ERR_CRC            : CRC error detected
     * \return ERR_FRAMING        : Framing error detected
     * \return ERR_PROTO          : Protocol error detected
     * \return ERR_TIMEOUT        : Timeout error
     * \return ERR_NONE           : No error
     *****************************************************************************
     */
    ReturnCode rfalST25xVPollerPresentPassword(uint8_t flags, const uint8_t *uid, uint8_t pwdNum, const uint8_t *pwd, uint8_t pwdLen);

    /*!
     *****************************************************************************
     * \brief  NFC-V Poller Get Random Number
     *
     *  Returns a 16 bit random number
     *
     * \param[in]  flags          : Flags to be used: Sub-carrier; Data_rate; Option
     *                              for NFC-Forum use: RFAL_NFCV_REQ_FLAG_DEFAULT
     * \param[in]  uid            : UID of the device to be put to be read
     *                               if not provided Select mode will be used
     * \param[out] rxBuf          : buffer to store response (also with RES_FLAGS)
     * \param[in]  rxBufLen       : length of rxBuf
     * \param[out] rcvLen         : number of bytes received
     *
     * \return ERR_WRONG_STATE    : RFAL not initialized or incorrect mode
     * \return ERR_PARAM          : Invalid parameters
     * \return ERR_IO             : Generic internal error
     * \return ERR_CRC            : CRC error detected
     * \return ERR_FRAMING        : Framing error detected
     * \return ERR_PROTO          : Protocol error detected
     * \return ERR_TIMEOUT        : Timeout error
     * \return ERR_NONE           : No error
     *****************************************************************************
     */
    ReturnCode rfalST25xVPollerGetRandomNumber(uint8_t flags, const uint8_t *uid, uint8_t *rxBuf, uint16_t rxBufLen, uint16_t *rcvLen);

    /*!
     *****************************************************************************
     * \brief  NFC-V Poller Read Message length
     *
     * Sends a Read Message Length message to retrieve the value of MB_LEN_Dyn
     *
     * \param[in]  flags          : Flags to be used: Sub-carrier; Data_rate; Option
     *                              for NFC-Forum use: RFAL_NFCV_REQ_FLAG_DEFAULT
     * \param[in]  uid            : UID of the device to be put to be read
     *                               if not provided Select mode will be used
     * \param[out] msgLen         : Message Length
     *
     * \return ERR_WRONG_STATE    : RFAL not initialized or incorrect mode
     * \return ERR_PARAM          : Invalid parameters
     * \return ERR_IO             : Generic internal error
     * \return ERR_CRC            : CRC error detected
     * \return ERR_FRAMING        : Framing error detected
     * \return ERR_PROTO          : Protocol error detected
     * \return ERR_TIMEOUT        : Timeout error
     * \return ERR_NONE           : No error
     *****************************************************************************
     */
    ReturnCode rfalST25xVPollerReadMessageLength(uint8_t flags, const uint8_t *uid, uint8_t *msgLen);

    /*!
     *****************************************************************************
     * \brief  NFC-V Poller Fast Read Message length
     *
     * Sends a Fast Read Message Length message to retrieve the value of MB_LEN_Dyn using ST Fast mode.
     *
     * \param[in]  flags          : Flags to be used: Sub-carrier; Data_rate; Option
     *                              for NFC-Forum use: RFAL_NFCV_REQ_FLAG_DEFAULT
     * \param[in]  uid            : UID of the device to be put to be read
     *                               if not provided Select mode will be used
     * \param[out] msgLen         : Message Length
     *
     * \return ERR_WRONG_STATE    : RFAL not initialized or incorrect mode
     * \return ERR_PARAM          : Invalid parameters
     * \return ERR_IO             : Generic internal error
     * \return ERR_CRC            : CRC error detected
     * \return ERR_FRAMING        : Framing error detected
     * \return ERR_PROTO          : Protocol error detected
     * \return ERR_TIMEOUT        : Timeout error
     * \return ERR_NONE           : No error
     *****************************************************************************
     */
    ReturnCode rfalST25xVPollerFastReadMsgLength(uint8_t flags, const uint8_t *uid, uint8_t *msgLen);

    /*!
     *****************************************************************************
     * \brief  NFC-V Poller Read Message
     *
     * Reads up to 256 bytes in the Mailbox from the location
     * specified by MBpointer and sends back their value in the rxBuf response.
     * First MailBox location is '00'. When Number of bytes is set to 00h
     * and MBPointer is equals to 00h, the MB_LEN bytes of the full message
     * are returned. Otherwise, Read Message command returns (Number of Bytes + 1) bytes
     * (i.e. 01h returns 2 bytes, FFh returns 256 bytes).
     * An error is reported if (Pointer + Nb of bytes + 1) is greater than the message length.
     * RF Reading of the last byte of the mailbox message automatically clears b1
     * of MB_CTRL_Dyn HOST_PUT_MSG, and allows RF to put a new message.
     *
     * \param[in]  flags          : Flags to be used: Sub-carrier; Data_rate; Option
     *                              for NFC-Forum use: RFAL_NFCV_REQ_FLAG_DEFAULT
     * \param[in]  uid            : UID of the device to be put to be read
     *                               if not provided Select mode will be used
     * \param[in]  mbPointer      : MPpointer
     * \param[in]  numBytes       : number of bytes
     * \param[out] rxBuf          : buffer to store response (also with RES_FLAGS)
     * \param[in]  rxBufLen       : length of rxBuf
     * \param[out] rcvLen         : number of bytes received
     *
     * \return ERR_WRONG_STATE    : RFAL not initialized or incorrect mode
     * \return ERR_PARAM          : Invalid parameters
     * \return ERR_IO             : Generic internal error
     * \return ERR_CRC            : CRC error detected
     * \return ERR_FRAMING        : Framing error detected
     * \return ERR_PROTO          : Protocol error detected
     * \return ERR_TIMEOUT        : Timeout error
     * \return ERR_NONE           : No error
     *****************************************************************************
     */
    ReturnCode rfalST25xVPollerReadMessage(uint8_t flags, const uint8_t *uid, uint8_t mbPointer, uint8_t numBytes, uint8_t *rxBuf, uint16_t rxBufLen, uint16_t *rcvLen);

    /*!
     *****************************************************************************
     * \brief  NFC-V Poller Fast Read Message
     *
     * Reads up to 256 bytes in the Mailbox from the location
     * specified by MBpointer and sends back their value in the rxBuf response using ST Fast mode.
     * First MailBox location is '00'. When Number of bytes is set to 00h
     * and MBPointer is equals to 00h, the MB_LEN bytes of the full message
     * are returned. Otherwise, Read Message command returns (Number of Bytes + 1) bytes
     * (i.e. 01h returns 2 bytes, FFh returns 256 bytes).
     * An error is reported if (Pointer + Nb of bytes + 1) is greater than the message length.
     * RF Reading of the last byte of the mailbox message automatically clears b1
     * of MB_CTRL_Dyn  HOST_PUT_MSG, and allows RF to put a new message.
     *
     * \param[in]  flags          : Flags to be used: Sub-carrier; Data_rate; Option
     *                              for NFC-Forum use: RFAL_NFCV_REQ_FLAG_DEFAULT
     * \param[in]  uid            : UID of the device to be put to be read
     *                               if not provided Select mode will be used
     * \param[in]  mbPointer      : MPpointer
     * \param[in]  numBytes       : number of bytes
     * \param[out] rxBuf          : buffer to store response (also with RES_FLAGS)
     * \param[in]  rxBufLen       : length of rxBuf
     * \param[out] rcvLen         : number of bytes received
     *
     * \return ERR_WRONG_STATE    : RFAL not initialized or incorrect mode
     * \return ERR_PARAM          : Invalid parameters
     * \return ERR_IO             : Generic internal error
     * \return ERR_CRC            : CRC error detected
     * \return ERR_FRAMING        : Framing error detected
     * \return ERR_PROTO          : Protocol error detected
     * \return ERR_TIMEOUT        : Timeout error
     * \return ERR_NONE           : No error
     *****************************************************************************
     */
    ReturnCode rfalST25xVPollerFastReadMessage(uint8_t flags, const uint8_t *uid, uint8_t mbPointer, uint8_t numBytes, uint8_t *rxBuf, uint16_t rxBufLen, uint16_t *rcvLen);

    /*!
     *****************************************************************************
     * \brief  NFC-V Poller Write Message
     *
     * Sends Write message Command
     *
     * On receiving the Write Message command, the ST25DVxxx puts the data contained
     * in the request into the Mailbox buffer, update the MB_LEN_Dyn register, and
     * set bit RF_PUT_MSG in MB_CTRL_Dyn register. It then reports if the write operation was successful
     * in the response. The ST25DVxxx Mailbox contains up to 256 data bytes which are filled from the
     *  first location '00'. MSGlength parameter of the command is the number of
     * Data bytes minus 1 (00 for 1 byte of data, FFh for 256 bytes of data).
     * Write Message could be executed only when Mailbox is accessible by RF.
     *
     * \param[in]  flags          : Flags to be used: Sub-carrier; Data_rate; Option
     *                              for NFC-Forum use: RFAL_NFCV_REQ_FLAG_DEFAULT
     * \param[in]  uid            : UID of the device to be put to be read
     *                               if not provided Select mode will be used
     * \param[in]  msgLen         : MSGLen  number of Data bytes minus 1
     * \param[in]  msgData        : Message Data
     * \param[out] txBuf          : buffer to used to build the Write Message command
     * \param[in]  txBufLen       : length of txBuf
     *
     * \return ERR_WRONG_STATE    : RFAL not initialized or incorrect mode
     * \return ERR_PARAM          : Invalid parameters
     * \return ERR_IO             : Generic internal error
     * \return ERR_CRC            : CRC error detected
     * \return ERR_FRAMING        : Framing error detected
     * \return ERR_PROTO          : Protocol error detected
     * \return ERR_TIMEOUT        : Timeout error
     * \return ERR_NONE           : No error
     *****************************************************************************
     */
    ReturnCode rfalST25xVPollerWriteMessage(uint8_t flags, const uint8_t *uid, uint8_t msgLen, const uint8_t *msgData, uint8_t *txBuf, uint16_t txBufLen);

    /*!
     *****************************************************************************
     * \brief  NFC-V Poller Fast Write Message
     *
     * Sends Fast Write message Command using ST Fast mode
     *
     * On receiving the Write Message command, the ST25DVxxx puts the data contained
     * in the request into the Mailbox buffer, update the MB_LEN_Dyn register, and
     * set bit RF_PUT_MSG in MB_CTRL_Dyn register. It then reports if the write operation was successful
     * in the response. The ST25DVxxx Mailbox contains up to 256 data bytes which are filled from the
     *  first location '00'. MSGlength parameter of the command is the number of
     * Data bytes minus 1 (00 for 1 byte of data, FFh for 256 bytes of data).
     * Write Message could be executed only when Mailbox is accessible by RF.
     *
     * \param[in]  flags          : Flags to be used: Sub-carrier; Data_rate; Option
     *                              for NFC-Forum use: RFAL_NFCV_REQ_FLAG_DEFAULT
     * \param[in]  uid            : UID of the device to be put to be read
     *                               if not provided Select mode will be used
     * \param[in]  msgLen         : MSGLen  number of Data bytes minus 1
     * \param[in]  msgData        : Message Data
     * \param[out] txBuf          : buffer to used to build the Write Message command
     * \param[in]  txBufLen       : length of txBuf
     *
     * \return ERR_WRONG_STATE    : RFAL not initialized or incorrect mode
     * \return ERR_PARAM          : Invalid parameters
     * \return ERR_IO             : Generic internal error
     * \return ERR_CRC            : CRC error detected
     * \return ERR_FRAMING        : Framing error detected
     * \return ERR_PROTO          : Protocol error detected
     * \return ERR_TIMEOUT        : Timeout error
     * \return ERR_NONE           : No error
     *****************************************************************************
     */
    ReturnCode rfalST25xVPollerFastWriteMessage(uint8_t flags, const uint8_t *uid, uint8_t msgLen, const uint8_t *msgData, uint8_t *txBuf, uint16_t txBufLen);


    /*
    ******************************************************************************
    * RFAL T1T FUNCTION PROTOTYPES
    ******************************************************************************
    */


    /*!
     *****************************************************************************
     * \brief  Initialize NFC-A T1T Poller mode
     *
     * This methods configures RFAL RF layer to perform as a
     * NFC-A T1T Poller/RW (Topaz) including all default timings
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalT1TPollerInitialize(void);


    /*!
     *****************************************************************************
     * \brief  NFC-A T1T Poller RID
     *
     * This method reads the UID of a NFC-A T1T Listener device
     *
     *
     * \param[out]  ridRes : pointer to place the RID_RES
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
     * \return ERR_PARAM        : Invalid parameter
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalT1TPollerRid(rfalT1TRidRes *ridRes);


    /*!
     *****************************************************************************
     * \brief  NFC-A T1T Poller RALL
     *
     * This method send a Read All command to a NFC-A T1T Listener device
     *
     *
     * \param[in]   uid       : the UID of the device to read data
     * \param[out]  rxBuf     : pointer to place the read data
     * \param[in]   rxBufLen  : size of rxBuf
     * \param[out]  rxRcvdLen : actual received data
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
     * \return ERR_PARAM        : Invalid parameter
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalT1TPollerRall(const uint8_t *uid, uint8_t *rxBuf, uint16_t rxBufLen, uint16_t *rxRcvdLen);


    /*!
     *****************************************************************************
     * \brief  NFC-A T1T Poller Write
     *
     * This method writes the given data on the address of a NFC-A T1T Listener device
     *
     *
     * \param[in]   uid       : the UID of the device to read data
     * \param[in]   address   : address to write the data
     * \param[in]   data      : the data to be written
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
     * \return ERR_PARAM        : Invalid parameter
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalT1TPollerWrite(const uint8_t *uid, uint8_t address, uint8_t data);


    /*
    ******************************************************************************
    * RFAL T2T FUNCTION PROTOTYPES
    ******************************************************************************
    */


    /*!
     *****************************************************************************
     * \brief  NFC-A T2T Poller Read
     *
     * This method sends a Read command to a NFC-A T2T Listener device
     *
     *
     * \param[in]   blockNum    : Number of the block to read
     * \param[out]  rxBuf       : pointer to place the read data
     * \param[in]   rxBufLen    : size of rxBuf (RFAL_T2T_READ_DATA_LEN)
     * \param[out]  rcvLen   : actual received data
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
     * \return ERR_PARAM        : Invalid parameter
     * \return ERR_PROTO        : Protocol error
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalT2TPollerRead(uint8_t blockNum, uint8_t *rxBuf, uint16_t rxBufLen, uint16_t *rcvLen);


    /*!
     *****************************************************************************
     * \brief  NFC-A T2T Poller Write
     *
     * This method sends a Write command to a NFC-A T2T Listener device
     *
     *
     * \param[in]  blockNum     : Number of the block to write
     * \param[in]  wrData       : data to be written on the given block
     *                            size must be of RFAL_T2T_WRITE_DATA_LEN
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
     * \return ERR_PARAM        : Invalid parameter
     * \return ERR_PROTO        : Protocol error
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalT2TPollerWrite(uint8_t blockNum, const uint8_t *wrData);


    /*!
     *****************************************************************************
     * \brief  NFC-A T2T Poller Sector Select
     *
     * This method sends a Sector Select commands to a NFC-A T2T Listener device
     *
     * \param[in]  sectorNum    : Sector Number
     *
     * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
     * \return ERR_PARAM        : Invalid parameter
     * \return ERR_PROTO        : Protocol error
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalT2TPollerSectorSelect(uint8_t sectorNum);


    /*
    ******************************************************************************
    * RFAL T4T FUNCTION PROTOTYPES
    ******************************************************************************
    */


    /*!
     *****************************************************************************
     * \brief  T4T Compose APDU
     *
     * This method computes a C-APDU according to NFC Forum T4T and ISO7816-4.
     *
     * If C-APDU contains data to be sent, it must be placed inside the buffer
     *   rfalT4tTxRxApduParam.txRx.cApduBuf.apdu and signaled by Lc
     *
     * To transceive the formed APDU the ISO-DEP layer shall be used
     *
     * \see rfalIsoDepStartApduTransceive()
     * \see rfalIsoDepGetApduTransceiveStatus()
     * \see rfalT4TPollerParseRAPDU()
     *
     * \warning The ISO-DEP module is used to perform the tranceive. Usually
     *          activation has been done via ISO-DEP activatiavtion. If not
     *          please call rfalIsoDepInitialize() before.
     *
     * \param[in,out] apduParam : APDU parameters
     *                            apduParam.cApduLen will contain the APDU length
     *
     * \return ERR_PARAM        : Invalid parameter
     * \return ERR_PROTO        : Protocol error
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalT4TPollerComposeCAPDU(const rfalT4tCApduParam *apduParam);


    /*!
     *****************************************************************************
     * \brief  T4T Parse R-APDU
     *
     * This method parses a R-APDU according to NFC Forum T4T and ISO7816-4.
     * It will extract the data length and check if the Status word is expected.
     *
     * \param[in,out] apduParam : APDU parameters
     *                            apduParam.rApduBodyLen will contain the data length
     *                            apduParam.statusWord will contain the SW1 and SW2
     *
     * \return ERR_REQUEST      : Status word (SW1 SW2) different from 9000
     * \return ERR_PARAM        : Invalid parameter
     * \return ERR_PROTO        : Protocol error
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalT4TPollerParseRAPDU(rfalT4tRApduParam *apduParam);

    /*!
     *****************************************************************************
     * \brief  T4T Compose Select Application APDU
     *
     * This method computes a Select Application APDU according to NFC Forum T4T
     *
     * To transceive the formed APDU the ISO-DEP layer shall be used
     *
     * \see rfalIsoDepStartApduTransceive()
     * \see rfalIsoDepGetApduTransceiveStatus()
     *
     * \param[out]     cApduBuf : buffer where the C-APDU will be placed
     * \param[in]      aid      : Application ID to be used
     * \param[in]      aidLen   : Application ID length
     * \param[out]     cApduLen : Composed C-APDU length
     *
     * \return ERR_PARAM        : Invalid parameter
     * \return ERR_PROTO        : Protocol error
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalT4TPollerComposeSelectAppl(rfalIsoDepApduBufFormat *cApduBuf, const uint8_t *aid, uint8_t aidLen, uint16_t *cApduLen);

    /*!
     *****************************************************************************
     * \brief  T4T Compose Select File APDU
     *
     * This method computes a Select File APDU according to NFC Forum T4T
     *
     * To transceive the formed APDU the ISO-DEP layer shall be used
     *
     * \see rfalIsoDepStartApduTransceive()
     * \see rfalIsoDepGetApduTransceiveStatus()
     *
     * \param[out]     cApduBuf : buffer where the C-APDU will be placed
     * \param[in]      fid      : File ID to be used
     * \param[in]      fidLen   : File ID length
     * \param[out]     cApduLen : Composed C-APDU length
     *
     * \return ERR_PARAM        : Invalid parameter
     * \return ERR_PROTO        : Protocol error
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalT4TPollerComposeSelectFile(rfalIsoDepApduBufFormat *cApduBuf, const uint8_t *fid, uint8_t fidLen, uint16_t *cApduLen);

    /*!
     *****************************************************************************
     * \brief  T4T Compose Select File APDU for Mapping Version 1
     *
     * This method computes a Select File APDU according to NFC Forum T4TOP_v1.0
     *
     * To transceive the formed APDU the ISO-DEP layer shall be used
     *
     * \see rfalIsoDepStartApduTransceive()
     * \see rfalIsoDepGetApduTransceiveStatus()
     *
     * \param[out]     cApduBuf : buffer where the C-APDU will be placed
     * \param[in]      fid      : File ID to be used
     * \param[in]      fidLen   : File ID length
     * \param[out]     cApduLen : Composed C-APDU length
     *
     * \return ERR_PARAM        : Invalid parameter
     * \return ERR_PROTO        : Protocol error
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalT4TPollerComposeSelectFileV1Mapping(rfalIsoDepApduBufFormat *cApduBuf, const uint8_t *fid, uint8_t fidLen, uint16_t *cApduLen);

    /*!
     *****************************************************************************
     * \brief  T4T Compose Read Data APDU
     *
     * This method computes a Read Data APDU according to NFC Forum T4T
     *
     * To transceive the formed APDU the ISO-DEP layer shall be used
     *
     * \see rfalIsoDepStartApduTransceive()
     * \see rfalIsoDepGetApduTransceiveStatus()
     *
     * \param[out]     cApduBuf : buffer where the C-APDU will be placed
     * \param[in]      offset   : File offset
     * \param[in]      expLen   : Expected length (Le)
     * \param[out]     cApduLen : Composed C-APDU length
     *
     * \return ERR_PARAM        : Invalid parameter
     * \return ERR_PROTO        : Protocol error
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalT4TPollerComposeReadData(rfalIsoDepApduBufFormat *cApduBuf, uint16_t offset, uint8_t expLen, uint16_t *cApduLen);

    /*!
     *****************************************************************************
     * \brief  T4T Compose Read Data ODO APDU
     *
     * This method computes a Read Data ODO APDU according to NFC Forum T4T
     *
     * To transceive the formed APDU the ISO-DEP layer shall be used
     *
     * \see rfalIsoDepStartApduTransceive()
     * \see rfalIsoDepGetApduTransceiveStatus()
     *
     * \param[out]     cApduBuf : buffer where the C-APDU will be placed
     * \param[in]      offset   : File offset
     * \param[in]      expLen   : Expected length (Le)
     * \param[out]     cApduLen : Composed C-APDU length
     *
     * \return ERR_PARAM        : Invalid parameter
     * \return ERR_PROTO        : Protocol error
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalT4TPollerComposeReadDataODO(rfalIsoDepApduBufFormat *cApduBuf, uint32_t offset, uint8_t expLen, uint16_t *cApduLen);

    /*!
     *****************************************************************************
     * \brief  T4T Compose Write Data APDU
     *
     * This method computes a Write Data APDU according to NFC Forum T4T
     *
     * To transceive the formed APDU the ISO-DEP layer shall be used
     *
     * \see rfalIsoDepStartApduTransceive()
     * \see rfalIsoDepGetApduTransceiveStatus()
     *
     * \param[out]     cApduBuf : buffer where the C-APDU will be placed
     * \param[in]      offset   : File offset
     * \param[in]      data     : Data to be written
     * \param[in]      dataLen  : Data length to be written (Lc)
     * \param[out]     cApduLen : Composed C-APDU length
     *
     * \return ERR_PARAM        : Invalid parameter
     * \return ERR_PROTO        : Protocol error
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalT4TPollerComposeWriteData(rfalIsoDepApduBufFormat *cApduBuf, uint16_t offset, const uint8_t *data, uint8_t dataLen, uint16_t *cApduLen);

    /*!
     *****************************************************************************
     * \brief  T4T Compose Write Data ODO APDU
     *
     * This method computes a Write Data ODO sAPDU according to NFC Forum T4T
     *
     * To transceive the formed APDU the ISO-DEP layer shall be used
     *
     * \see rfalIsoDepStartApduTransceive()
     * \see rfalIsoDepGetApduTransceiveStatus()
     *
     * \param[out]     cApduBuf : buffer where the C-APDU will be placed
     * \param[in]      offset   : File offset
     * \param[in]      data     : Data to be written
     * \param[in]      dataLen  : Data length to be written (Lc)
     * \param[out]     cApduLen : Composed C-APDU length
     *
     * \return ERR_PARAM        : Invalid parameter
     * \return ERR_PROTO        : Protocol error
     * \return ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalT4TPollerComposeWriteDataODO(rfalIsoDepApduBufFormat *cApduBuf, uint32_t offset, const uint8_t *data, uint8_t dataLen, uint16_t *cApduLen);

    /*!
    *****************************************************************************
    * \brief  NFC-V Poller Write Password
    *
    * Sends the Write Password command
    *
    * \param[in]  flags          : Flags to be used: Sub-carrier; Data_rate; Option
    *                              for NFC-Forum use: RFAL_NFCV_REQ_FLAG_DEFAULT
    * \param[in]  uid            : UID of the device to be put to be read
    *                               if not provided Select mode will be used
    * \param[in]  pwdNum         : Password number
    * \param[in]  pwd            : Password
    * \param[in]  pwdLen         : Password length
    *
    * \return ERR_WRONG_STATE    : RFAL not initialized or incorrect mode
    * \return ERR_PARAM          : Invalid parameters
    * \return ERR_IO             : Generic internal error
    * \return ERR_CRC            : CRC error detected
    * \return ERR_FRAMING        : Framing error detected
    * \return ERR_PROTO          : Protocol error detected
    * \return ERR_TIMEOUT        : Timeout error
    * \return ERR_NONE           : No error
    *****************************************************************************
    */
    ReturnCode rfalST25xVPollerWritePassword(uint8_t flags, const uint8_t *uid, uint8_t pwdNum, const uint8_t *pwd,  uint8_t pwdLen);

    /*!
    *****************************************************************************
    *  \brief  NFC-A Get Technology Detection Status
    *
    *  Returns the Technology Detection status
    *
    * \return ERR_WRONG_STATE  : RFAL not initialized or incorrect mode
    * \return ERR_PARAM        : Invalid parameters
    * \return ERR_IO           : Generic internal error
    * \return ERR_NONE         : No error, one or more device in the field
    *****************************************************************************
    */
    ReturnCode rfalNfcaPollerGetTechnologyDetectionStatus(void);


    /*!
    *****************************************************************************
    * \brief  NFC-A Poller Start Full Collision Resolution
    *
    * This method starts the full Collision resolution as defined
      * in Activity 1.0 or 1.1  9.3.4
    *
    * \param[in]  compMode    : compliance mode to be performed
    * \param[in]  devLimit    : device limit value, and size nfcaDevList
    * \param[out] nfcaDevList : NFC-A listener device info
    * \param[out] devCnt      : Devices found counter
    *
    * When compMode is set to ISO compliance it assumes that the device is
    * not sleeping and therefore no ALL_REQ (WUPA) is sent at the beginning.
    * When compMode is set to NFC compliance an additional ALL_REQ (WUPA) is sent at
    * the beginning.
    *
    *
    * When devLimit = 0 it is configured to perform collision detection only. Once a collision
    * is detected the collision resolution is aborted immediately. If only one device is found
    * with no collisions, it will properly resolved.
    *
    *
    * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
    * \return ERR_PARAM        : Invalid parameters
    * \return ERR_IO           : Generic internal error
    * \return ERR_NONE         : No error
    *****************************************************************************
    */
    ReturnCode rfalNfcaPollerStartFullCollisionResolution(rfalComplianceMode compMode, uint8_t devLimit, rfalNfcaListenDevice *nfcaDevList, uint8_t *devCnt);

    /*!
    *****************************************************************************
    *  \brief  NFC-A Get Full Collision Resolution Status
    *
    *  Returns the Collision Resolution status
    *
    *  \return ERR_BUSY         : Operation is ongoing
    *  \return ERR_WRONG_STATE  : RFAL not initialized or incorrect mode
    *  \return ERR_PARAM        : Invalid parameters
    *  \return ERR_IO           : Generic internal error
    *  \return ERR_TIMEOUT      : Timeout error
    *  \return ERR_PAR          : Parity error detected
    *  \return ERR_CRC          : CRC error detected
    *  \return ERR_FRAMING      : Framing error detected
    *  \return ERR_PROTO        : Protocol error detected
    *  \return ERR_NONE         : No error, activation successful
    *****************************************************************************
    */
    ReturnCode rfalNfcaPollerGetFullCollisionResolutionStatus(void);



    /*!
    *****************************************************************************
    *  \brief  NFC-B Get Collision Resolution Status
    *
    *  Returns the Collision Resolution status
    *
    *  \return ERR_BUSY         : Operation is ongoing
    *  \return ERR_WRONG_STATE  : RFAL not initialized or incorrect mode
    *  \return ERR_PARAM        : Invalid parameters
    *  \return ERR_IO           : Generic internal error
    *  \return ERR_TIMEOUT      : Timeout error
    *  \return ERR_PAR          : Parity error detected
    *  \return ERR_CRC          : CRC error detected
    *  \return ERR_FRAMING      : Framing error detected
    *  \return ERR_PROTO        : Protocol error detected
    *  \return ERR_NONE         : No error, activation successful
    *****************************************************************************
    */
    ReturnCode rfalNfcbPollerGetCollisionResolutionStatus(void);





    RfalRfClass *getRfalRf()
    {
      return rfalRfDev;
    }


  protected:
    ReturnCode rfalNfcfPollerStartCheckPresence(void);
    ReturnCode rfalNfcfPollerGetCheckPresenceStatus(void);
    ReturnCode rfalNfcPollTechDetection(void);
    ReturnCode rfalNfcPollCollResolution(void);
    ReturnCode rfalNfcPollActivation(uint8_t devIt);
    ReturnCode rfalNfcDeactivation(void);
    ReturnCode rfalNfcNfcDepActivate(rfalNfcDevice *device, rfalNfcDepCommMode commMode, const uint8_t *atrReq, uint16_t atrReqLen);
    void isoDepClearCounters(void);
    ReturnCode isoDepTx(uint8_t pcb, const uint8_t *txBuf, uint8_t *infBuf, uint16_t infLen, uint32_t fwt);
    ReturnCode isoDepHandleControlMsg(rfalIsoDepControlMsg controlMsg, uint8_t param);
    void rfalIsoDepApdu2IBLockParam(rfalIsoDepApduTxRxParam apduParam, rfalIsoDepTxRxParam *iBlockParam, uint16_t txPos, uint16_t rxPos);
    ReturnCode isoDepDataExchangePCD(uint16_t *outActRxLen, bool *outIsChaining);
    void rfalIsoDepCalcBitRate(rfalBitRate maxAllowedBR, uint8_t piccBRCapability, rfalBitRate *dsi, rfalBitRate *dri);
    uint32_t rfalIsoDepSFGI2SFGT(uint8_t sfgi);
    uint8_t rfalNfcaCalculateBcc(const uint8_t *buf, uint8_t bufLen);
    ReturnCode rfalNfcbPollerStartCheckPresence(rfalNfcbSensCmd cmd, rfalNfcbSlots slots, rfalNfcbSensbRes *sensbRes, uint8_t *sensbResLen);
    ReturnCode rfalNfcbPollerStartSlotMarker(uint8_t slotCode, rfalNfcbSensbRes *sensbRes, uint8_t *sensbResLen);
    ReturnCode rfalNfcbPollerSleepTx(const uint8_t *nfcid0);
    ReturnCode rfalNfcaPollerStartSingleCollisionResolution(uint8_t devLimit, bool *collPending, rfalNfcaSelRes *selRes, uint8_t *nfcId1, uint8_t *nfcId1Len);
    ReturnCode rfalNfcaPollerGetSingleCollisionResolutionStatus(void);
    ReturnCode rfalIsoDepStartDeselect(void);
    ReturnCode rfalIsoDepGetDeselectStatus(void);
    ReturnCode rfalNfcbCheckSensbRes(const rfalNfcbSensbRes *sensbRes, uint8_t sensbResLen);
    ReturnCode rfalNfcfPollerStartCollisionResolution(rfalComplianceMode compMode, uint8_t devLimit, rfalNfcfListenDevice *nfcfDevList, uint8_t *devCnt);
    ReturnCode rfalNfcfPollerGetCollisionResolutionStatus(void);
    ReturnCode rfalIsoDepStartRATS(rfalIsoDepFSxI FSDI, uint8_t DID, rfalIsoDepAts *ats, uint8_t *atsLen);
    ReturnCode rfalIsoDepGetRATSStatus(void);
    ReturnCode rfalIsoDepStartPPS(uint8_t DID, rfalBitRate DSI, rfalBitRate DRI, rfalIsoDepPpsRes *ppsRes);
    ReturnCode rfalIsoDepGetPPSSTatus(void);
    ReturnCode rfalIsoDepStartATTRIB(const uint8_t *nfcid0, uint8_t PARAM1, rfalBitRate DSI, rfalBitRate DRI, rfalIsoDepFSxI FSDI, uint8_t PARAM3, uint8_t DID, const uint8_t *HLInfo, uint8_t HLInfoLen, uint32_t fwt, rfalIsoDepAttribRes *attribRes, uint8_t *attribResLen);
    ReturnCode rfalIsoDepGetATTRIBStatus(void);
    ReturnCode rfalIsoDepPollAStartActivation(rfalIsoDepFSxI FSDI, uint8_t DID, rfalBitRate maxBR, rfalIsoDepDevice *rfalIsoDepDev);
    ReturnCode rfalIsoDepPollAGetActivationStatus(void);
    ReturnCode rfalIsoDepPollBStartActivation(rfalIsoDepFSxI FSDI, uint8_t DID, rfalBitRate maxBR, uint8_t PARAM1, const rfalNfcbListenDevice *nfcbDev, const uint8_t *HLInfo, uint8_t HLInfoLen, rfalIsoDepDevice *rfalIsoDepDev);
    ReturnCode rfalIsoDepPollBGetActivationStatus(void);
    ReturnCode rfalIsoDepDataExchangePICC(void);
    ReturnCode rfalIsoDepReSendControlMsg(void);
    ReturnCode rfalNfcaPollerStartSleep(void);
    ReturnCode rfalNfcbPollerStartTechnologyDetection(rfalComplianceMode compMode, rfalNfcbSensbRes *sensbRes, uint8_t *sensbResLen);
    ReturnCode rfalNfcbPollerGetCheckPresenceStatus(void);
    ReturnCode rfalNfcbPollerGetTechnologyDetectionStatus(void);
    ReturnCode rfalNfcbPollerGetSlotMarkerStatus(void);
    ReturnCode rfalNfcbPollerStartCollisionResolution(rfalComplianceMode compMode, uint8_t devLimit, rfalNfcbListenDevice *nfcbDevList, uint8_t *devCnt);
    ReturnCode rfalNfcaPollerStartSelect(const uint8_t *nfcid1, uint8_t nfcidLen, rfalNfcaSelRes *selRes);
    ReturnCode rfalNfcaPollerGetSelectStatus(void);
    ReturnCode rfalNfcbPollerStartSlottedCollisionResolution(rfalComplianceMode compMode, uint8_t devLimit, rfalNfcbSlots initSlots, rfalNfcbSlots endSlots, rfalNfcbListenDevice *nfcbDevList, uint8_t *devCnt, bool *colPending);
    ReturnCode rfalNfcaPollerGetSleepStatus(void);
    ReturnCode rfalNfcaPollerSleepFullCollisionResolution(uint8_t devLimit, rfalNfcaListenDevice *nfcaDevList, uint8_t *devCnt);
    bool nfcipDxIsSupported(uint8_t Dx, uint8_t BRx, uint8_t BSx);
    ReturnCode nfcipTxRx(rfalNfcDepCmd cmd, uint8_t *txBuf, uint32_t fwt, uint8_t *paylBuf, uint8_t paylBufLen, uint8_t *rxBuf, uint16_t rxBufLen, uint16_t *rxActLen);
    ReturnCode nfcipTx(rfalNfcDepCmd cmd, uint8_t *txBuf, uint8_t *paylBuf, uint16_t paylLen, uint8_t pfbData, uint32_t fwt);
    ReturnCode nfcipDEPControlMsg(uint8_t pfb, uint8_t RTOX);
    void nfcipClearCounters(void);
    ReturnCode nfcipInitiatorHandleDEP(ReturnCode rxRes, uint16_t rxLen, uint16_t *outActRxLen, bool *outIsChaining);
    ReturnCode nfcipTargetHandleRX(ReturnCode rxRes, uint16_t *outActRxLen, bool *outIsChaining);
    ReturnCode nfcipTargetHandleActivation(rfalNfcDepDevice *nfcDepDev, uint8_t *outBRS);
    void nfcipConfig(const rfalNfcDepConfigs *cfg);
    void nfcipSetDEPParams(rfalNfcDepDEPParams *DEPParams);
    ReturnCode nfcipRun(uint16_t *outActRxLen, bool *outIsChaining);
    ReturnCode nfcipDataTx(uint8_t *txBuf, uint16_t txBufLen, uint32_t fwt);
    ReturnCode nfcipDataRx(bool blocking);
    void rfalNfcfComputeValidSENF(rfalNfcfListenDevice *outDevInfo, uint8_t *curDevIdx, uint8_t devLimit, bool overwrite, bool *nfcDepFound);
    ReturnCode rfalNfcvParseError(uint8_t err);
    bool rfalSt25tbPollerDoCollisionResolution(uint8_t devLimit, rfalSt25tbListenDevice *st25tbDevList, uint8_t *devCnt);
    ReturnCode rfalST25xVPollerGenericReadConfiguration(uint8_t cmd, uint8_t flags, const uint8_t *uid, uint8_t pointer, uint8_t *regValue);
    ReturnCode rfalST25xVPollerGenericWriteConfiguration(uint8_t cmd, uint8_t flags, const uint8_t *uid, uint8_t pointer, uint8_t regValue);
    ReturnCode rfalST25xVPollerGenericReadMessageLength(uint8_t cmd, uint8_t flags, const uint8_t *uid, uint8_t *msgLen);
    ReturnCode rfalST25xVPollerGenericReadMessage(uint8_t cmd, uint8_t flags, const uint8_t *uid, uint8_t mbPointer, uint8_t numBytes, uint8_t *rxBuf, uint16_t rxBufLen, uint16_t *rcvLen);
    ReturnCode rfalST25xVPollerGenericWriteMessage(uint8_t cmd, uint8_t flags, const uint8_t *uid, uint8_t msgLen, const uint8_t *msgData, uint8_t *txBuf, uint16_t txBufLen);
    uint32_t timerCalculateTimer(uint16_t time);
    bool timerIsExpired(uint32_t timer);
    ReturnCode rfalNfcListenActivation(void);
    void rfalNfcDepPdu2BLockParam(rfalNfcDepPduTxRxParam pduParam, rfalNfcDepTxRxParam *blockParam, uint16_t txPos, uint16_t rxPos);

    RfalRfClass *rfalRfDev;

    rfalNfc gNfcDev;
    rfalIsoDep gIsoDep;    /*!< ISO-DEP Module instance               */
    rfalNfcb gRfalNfcb; /*!< RFAL NFC-B Instance */
    rfalNfcDep gNfcip;                    /*!< NFCIP module instance                         */
    rfalNfcfGreedyF gRfalNfcfGreedyF;   /*!< Activity's NFCF Greedy collection */

};

#endif /* RFAL_NFC_H */


/**
  * @}
  *
  * @}
  *
  * @}
  */
