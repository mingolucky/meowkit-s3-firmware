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
 *  \brief RF Abstraction Layer (RFAL)
 *
 *  RFAL implementation for ST25R3916
 */
#ifndef RFAL_RFST25R3916_H
#define RFAL_RFST25R3916_H


/*
******************************************************************************
* INCLUDES
******************************************************************************
*/
#include "st25r3916_config.h"
#include "rfal_config.h"
#include "SPI.h"
#include "MeowNfcBus.h"
#include "rfal_rf.h"
#include "st_errno.h"
#include "nfc_utils.h"
#include "st25r3916.h"
#include "st25r3916_com.h"
#include "st25r3916_interrupt.h"
#include "rfal_rfst25r3916_analogConfig.h"
#include "rfal_rfst25r3916_iso15693_2.h"
#include "st25r3916_aat.h"
#include <functional>

/*
 ******************************************************************************
 * ENABLE SWITCH
 ******************************************************************************
 */

#ifndef RFAL_FEATURE_LISTEN_MODE
  #define RFAL_FEATURE_LISTEN_MODE    false    /* Listen Mode configuration missing. Disabled by default */
#endif /* RFAL_FEATURE_LISTEN_MODE */

#ifndef RFAL_FEATURE_WAKEUP_MODE
  #define RFAL_FEATURE_WAKEUP_MODE    false    /* Wake-Up mode configuration missing. Disabled by default */
#endif /* RFAL_FEATURE_WAKEUP_MODE */

#ifndef RFAL_FEATURE_LOWPOWER_MODE
  #define RFAL_FEATURE_LOWPOWER_MODE  false   /* Low Power mode configuration missing. Disabled by default */
#endif /* RFAL_FEATURE_LOWPOWER_MODE */


/*
******************************************************************************
* GLOBAL TYPES
******************************************************************************
*/

/*! Struct that holds all involved on a Transceive including the context passed by the caller     */
typedef struct {
  rfalTransceiveState     state;       /*!< Current transceive state                            */
  rfalTransceiveState     lastState;   /*!< Last transceive state (debug purposes)              */
  ReturnCode              status;      /*!< Current status/error of the transceive              */

  rfalTransceiveContext   ctx;         /*!< The transceive context given by the caller          */
} rfalTxRx;


/*! Struct that holds certain WU mode information to be retrieved by rfalWakeUpModeGetInfo        */
typedef struct {
  bool                 irqWut;     /*!< Wake-Up Timer IRQ received (cleared upon read)          */

  struct {
    uint8_t          lastMeas;   /*!< Value of the latest measurement                         */
    bool             irqWu;      /*!< Amplitude WU IRQ received (cleared upon read)           */
  } indAmp;                        /*!< Inductive Amplitude                                     */
  struct {
    uint8_t          lastMeas;   /*!< Value of the latest measurement                         */
    bool             irqWu;      /*!< Phase WU IRQ received (cleared upon read)               */
  } indPha;                        /*!< Inductive Phase                                         */
  struct {
    uint8_t          lastMeas;   /*!< Value of the latest measurement                         */
    bool             irqWu;      /*!< Capacitive WU IRQ received (cleared upon read)          */
  } cap;                           /*!< Capacitance                                             */
} rfalWakeUpData;


/*! Local struct that holds context for the Listen Mode                                          */
typedef struct {
  rfalLmState             state;       /*!< Current Listen Mode state                           */
  uint32_t                mdMask;      /*!< Listen Mode mask used                               */
  uint32_t                mdReg;       /*!< Listen Mode register value used                     */
  uint32_t                mdIrqs;      /*!< Listen Mode IRQs used                               */
  rfalBitRate             brDetected;  /*!< Last bit rate detected                              */

  uint8_t                *rxBuf;       /*!< Location to store incoming data in Listen Mode      */
  uint16_t                rxBufLen;    /*!< Length of rxBuf                                     */
  uint16_t               *rxLen;       /*!< Pointer to write the data length placed into rxBuf  */
  bool                    dataFlag;    /*!< Listen Mode current Data Flag                       */
  bool                    iniFlag;     /*!< Listen Mode initialized Flag  (FeliCa slots)        */
} rfalLm;


/*! Struct that holds all context for the Wake-Up Mode                                            */
typedef struct {
  rfalWumState            state;       /*!< Current Wake-Up Mode state                          */
  rfalWakeUpConfig        cfg;         /*!< Current Wake-Up Mode config                         */
  rfalWakeUpData          info;        /*!< Current Wake-Up Mode info                           */
} rfalWum;


/*! Struct that holds all context for the Low Power Mode                                          */
typedef struct {
  bool                    isRunning;
} rfalLpm;


/*! Struct that holds the timings GT and FDTs                           */
typedef struct {
  uint32_t                GT;          /*!< GT in 1/fc                */
  uint32_t                FDTListen;   /*!< FDTListen in 1/fc         */
  uint32_t                FDTPoll;     /*!< FDTPoll in 1/fc           */
  uint8_t                 nTRFW;       /*!< n*TRFW (last two bits) used during RF CA  */
} rfalTimings;


/*! Struct that holds the software timers                               */
typedef struct {
  uint32_t                GT;          /*!< RFAL's GT timer           */
  uint32_t                RXE;         /*!< Timer between RXS - RXE   */
  uint32_t                PPON2;       /*!< Timer between TXE - PPON2 */
  uint32_t                txRx;        /*!< Transceive sanity timer   */
} rfalTimers;


/*! Struct that holds the RFAL's callbacks                              */
typedef struct {
  rfalPreTxRxCallback     preTxRx;     /*!< RFAL's Pre TxRx callback  */
  rfalPostTxRxCallback    postTxRx;    /*!< RFAL's Post TxRx callback */
  rfalSyncTxRxCallback    syncTxRx;    /*!< RFAL's Sync TxRx callback */
} rfalCallbacks;


/*! Struct that holds counters to control the FIFO on Tx and Rx                                                                          */
typedef struct {
  uint16_t                expWL;       /*!< The amount of bytes expected to be Tx when a WL interrupt occurs                          */
  uint16_t                bytesTotal;  /*!< Total bytes to be transmitted OR the total bytes received                                  */
  uint16_t                bytesWritten;/*!< Amount of bytes already written on FIFO (Tx) OR read (RX) from FIFO and written on rxBuffer*/
  uint8_t                 status[ST25R3916_FIFO_STATUS_LEN];   /*!< FIFO Status Registers                                              */
} rfalFIFO;


/*! Struct that holds RFAL's configuration settings                                                      */
typedef struct {
  uint8_t                 obsvModeTx;  /*!< RFAL's config of the ST25R3916's observation mode while Tx */
  uint8_t                 obsvModeRx;  /*!< RFAL's config of the ST25R3916's observation mode while Rx */
  rfalEHandling           eHandling;   /*!< RFAL's error handling config/mode                          */
} rfalConfigs;


/*! Struct that holds NFC-A data - Used only inside rfalISO14443ATransceiveAnticollisionFrame()          */
typedef struct {
  uint8_t                 collByte;    /*!< NFC-A Anticollision collision byte                         */
  uint8_t                 *buf;        /*!< NFC-A Anticollision frame buffer                           */
  uint8_t                 *bytesToSend;/*!< NFC-A Anticollision NFCID|UID byte context                 */
  uint8_t                 *bitsToSend; /*!< NFC-A Anticollision NFCID|UID bit context                  */
  uint16_t                *rxLength;   /*!< NFC-A Anticollision received length                        */
} rfalNfcaWorkingData;


/*! Struct that holds NFC-F data - Used only inside rfalFelicaPoll()                                           */
typedef struct {
  uint16_t           actLen;                                      /* Received length                         */
  rfalFeliCaPollRes *pollResList;                                 /* Location of NFC-F device list           */
  uint8_t            pollResListSize;                             /* Size of NFC-F device list               */
  uint8_t            devDetected;                                 /* Number of devices detected              */
  uint8_t            colDetected;                                 /* Number of collisions detected           */
  uint8_t            *devicesDetected;                            /* Location to place number of devices     */
  uint8_t            *collisionsDetected;                         /* Location to place number of collisions  */
  rfalEHandling      curHandling;                                 /* RFAL's error handling                   */
  rfalFeliCaPollRes  pollResponses[RFAL_FELICA_POLL_MAX_SLOTS];   /* FeliCa Poll response buffer (16 slots)  */
} rfalNfcfWorkingData;


/*! Struct that holds NFC-V current context
 *
 * This buffer has to be big enough for coping with maximum response size (hamming coded)
 *    - inventory requests responses: 14*2+2 bytes
 *    - read single block responses: (32+4)*2+2 bytes
 *    - read multiple block could be very long... -> not supported
 *    - current implementation expects it be written in one bulk into FIFO
 *    - needs to be above FIFO water level of ST25R3916 (200)
 *    - the coding function needs to be able to
 *      put more than FIFO water level bytes into it (n*64+1)>200                                                          */
typedef struct {
  uint8_t                 codingBuffer[((2 + 255 + 3) * 2)]; /*!< Coding buffer,   length MUST be above 257: [257; ...]    */
  uint16_t                nfcvOffset;        /*!< Offset needed for ISO15693 coding function                             */
  rfalTransceiveContext   origCtx;           /*!< context provided by user                                               */
  uint16_t                ignoreBits;        /*!< Number of bits at the beginning of a frame to be ignored when decoding */
} rfalNfcvWorkingData;


/*! RFAL instance  */
typedef struct {
  rfalState               state;     /*!< RFAL's current state                          */
  rfalMode                mode;      /*!< RFAL's current mode                           */
  rfalBitRate             txBR;      /*!< RFAL's current Tx Bit Rate                    */
  rfalBitRate             rxBR;      /*!< RFAL's current Rx Bit Rate                    */
  bool                    field;     /*!< Current field state (On / Off)                */

  rfalConfigs             conf;      /*!< RFAL's configuration settings                 */
  rfalTimings             timings;   /*!< RFAL's timing setting                         */
  rfalTxRx                TxRx;      /*!< RFAL's transceive management                  */
  rfalFIFO                fifo;      /*!< RFAL's FIFO management                        */
  rfalTimers              tmr;       /*!< RFAL's Software timers                        */
  rfalCallbacks           callbacks; /*!< RFAL's callbacks                              */

#if RFAL_FEATURE_LISTEN_MODE
  rfalLm                  Lm;        /*!< RFAL's listen mode management                 */
#endif /* RFAL_FEATURE_LISTEN_MODE */

#if RFAL_FEATURE_WAKEUP_MODE
  rfalWum                 wum;       /*!< RFAL's Wake-up mode management                */
#endif /* RFAL_FEATURE_WAKEUP_MODE */


#if RFAL_FEATURE_LOWPOWER_MODE
  rfalLpm                 lpm;       /*!< RFAL's Low power mode management              */
#endif /* RFAL_FEATURE_LOWPOWER_MODE */


#if RFAL_FEATURE_NFCA
  rfalNfcaWorkingData     nfcaData;  /*!< RFAL's working data when supporting NFC-A     */
#endif /* RFAL_FEATURE_NFCA */

#if RFAL_FEATURE_NFCF
  rfalNfcfWorkingData     nfcfData;  /*!< RFAL's working data when supporting NFC-F     */
#endif /* RFAL_FEATURE_NFCF */

#if RFAL_FEATURE_NFCV
  rfalNfcvWorkingData     nfcvData;  /*!< RFAL's working data when performing NFC-V     */
#endif /* RFAL_FEATURE_NFCV */

} rfal;



/*! Felica's command set */
typedef enum {
  FELICA_CMD_POLLING                  = 0x00, /*!< Felica Poll/REQC command (aka SENSF_REQ) to identify a card    */
  FELICA_CMD_POLLING_RES              = 0x01, /*!< Felica Poll/REQC command (aka SENSF_RES) response              */
  FELICA_CMD_REQUEST_SERVICE          = 0x02, /*!< verify the existence of Area and Service                       */
  FELICA_CMD_REQUEST_RESPONSE         = 0x04, /*!< verify the existence of a card                                 */
  FELICA_CMD_READ_WITHOUT_ENCRYPTION  = 0x06, /*!< read Block Data from a Service that requires no authentication */
  FELICA_CMD_WRITE_WITHOUT_ENCRYPTION = 0x08, /*!< write Block Data to a Service that requires no authentication  */
  FELICA_CMD_REQUEST_SYSTEM_CODE      = 0x0C, /*!< acquire the System Code registered to a card                   */
  FELICA_CMD_AUTHENTICATION1          = 0x10, /*!< authenticate a card                                            */
  FELICA_CMD_AUTHENTICATION2          = 0x12, /*!< allow a card to authenticate a Reader/Writer                   */
  FELICA_CMD_READ                     = 0x14, /*!< read Block Data from a Service that requires authentication    */
  FELICA_CMD_WRITE                    = 0x16, /*!< write Block Data to a Service that requires authentication     */
} t_rfalFeliCaCmd;


/*! Union representing all PTMem sections */
typedef union { /*  PRQA S 0750 # MISRA 19.2 - Both members are of the same type, just different names.  Thus no problem can occur. */
  uint8_t PTMem_A[ST25R3916_PTM_A_LEN];       /*!< PT_Memory area allocated for NFC-A configuration               */
  uint8_t PTMem_F[ST25R3916_PTM_F_LEN];       /*!< PT_Memory area allocated for NFC-F configuration               */
  uint8_t TSN[ST25R3916_PTM_TSN_LEN];         /*!< PT_Memory area allocated for TSN - Random numbers              */
} t_rfalPTMem;



/*! Struct for Analog Config Look Up Table Update */
typedef struct {
  const uint8_t *currentAnalogConfigTbl; /*!< Reference to start of current Analog Configuration      */
  uint16_t configTblSize;          /*!< Total size of Analog Configuration                      */
  bool    ready;                  /*!< Indicate if Look Up Table is complete and ready for use */
} rfalAnalogConfigMgmt;

template <typename T>
struct Callback;

template <typename Ret, typename... Params>
struct Callback<Ret(Params...)> {
  template <typename... Args>
  static Ret callback(Args... args)
  {
    return func(args...);
  }
  static std::function<Ret(Params...)> func;
};

template <typename Ret, typename... Params>
std::function<Ret(Params...)> Callback<Ret(Params...)>::func;

typedef void (*ST25R3916IrqHandler)(void);

/*
******************************************************************************
* GLOBAL DEFINES
******************************************************************************
*/

#define RFAL_FIFO_IN_WL                 200U                                          /*!< Number of bytes in the FIFO when WL interrupt occurs while Tx                   */
#define RFAL_FIFO_OUT_WL                (ST25R3916_FIFO_DEPTH - RFAL_FIFO_IN_WL)      /*!< Number of bytes sent/out of the FIFO when WL interrupt occurs while Tx          */

#define RFAL_FIFO_STATUS_REG1           0U                                            /*!< Location of FIFO status register 1 in local copy                                */
#define RFAL_FIFO_STATUS_REG2           1U                                            /*!< Location of FIFO status register 2 in local copy                                */
#define RFAL_FIFO_STATUS_INVALID        0xFFU                                         /*!< Value indicating that the local FIFO status in invalid|cleared                  */

#define RFAL_ST25R3916_GPT_MAX_1FC      rfalConv8fcTo1fc(  0xFFFFU )                  /*!< Max GPT steps in 1fc (0xFFFF steps of 8/fc    => 0xFFFF * 590ns  = 38,7ms)      */
#define RFAL_ST25R3916_NRT_MAX_1FC      rfalConv4096fcTo1fc( 0xFFFFU )                /*!< Max NRT steps in 1fc (0xFFFF steps of 4096/fc => 0xFFFF * 302us  = 19.8s )      */
#define RFAL_ST25R3916_NRT_DISABLED     0U                                            /*!< NRT Disabled: All 0 No-response timer is not started, wait forever              */
#define RFAL_ST25R3916_MRT_MAX_1FC      rfalConv64fcTo1fc( 0x00FFU )                  /*!< Max MRT steps in 1fc (0x00FF steps of 64/fc   => 0x00FF * 4.72us = 1.2ms )      */
#define RFAL_ST25R3916_MRT_MIN_1FC      rfalConv64fcTo1fc( 0x0004U )                  /*!< Min MRT steps in 1fc ( 0<=mrt<=4 ; 4 (64/fc)  => 0x0004 * 4.72us = 18.88us )    */
#define RFAL_ST25R3916_GT_MAX_1FC       rfalConvMsTo1fc( 6000U )                      /*!< Max GT value allowed in 1/fc (SFGI=14 => SFGT + dSFGT = 5.4s)                   */
#define RFAL_ST25R3916_GT_MIN_1FC       rfalConvMsTo1fc(RFAL_ST25R3916_SW_TMR_MIN_1MS)/*!< Min GT value allowed in 1/fc                                                    */
#define RFAL_ST25R3916_SW_TMR_MIN_1MS   1U                                            /*!< Min value of a SW timer in ms                                                   */

#define RFAL_OBSMODE_DISABLE            0x00U                                         /*!< Observation Mode disabled                                                       */

#define RFAL_RX_INC_BYTE_LEN            (uint8_t)1U                                   /*!< Threshold where incoming rx shall be considered incomplete byte NFC - T2T       */
#define RFAL_EMVCO_RX_MAXLEN            (uint8_t)4U                                   /*!< Maximum value where EMVCo to apply special error handling                       */

#define RFAL_NORXE_TOUT                 50U                                           /*!< Timeout to be used on a potential missing RXE - Silicon ST25R3916 Errata #2.1.2 */

#define RFAL_ISO14443A_SDD_RES_LEN      5U                                            /*!< SDD_RES | Anticollision (UID CLn) length  -  rfalNfcaSddRes                     */
#define RFAL_ISO14443A_CRC_INTVAL       0x6363                                        /*!< ISO14443 CRC Initial Value|Register                                             */


#define RFAL_FELICA_POLL_DELAY_TIME     512U                                          /*!<  FeliCa Poll Processing time is 2.417 ms ~512*64/fc Digital 1.1 A4              */
#define RFAL_FELICA_POLL_SLOT_TIME      256U                                          /*!<  FeliCa Poll Time Slot duration is 1.208 ms ~256*64/fc Digital 1.1 A4           */

#define RFAL_LM_SENSF_RD0_POS           17U                                           /*!<  FeliCa SENSF_RES Request Data RD0 position                                     */
#define RFAL_LM_SENSF_RD1_POS           18U                                           /*!<  FeliCa SENSF_RES Request Data RD1 position                                     */

#define RFAL_LM_NFCID_INCOMPLETE        0x04U                                         /*!<  NFCA NFCID not complete bit in SEL_RES (SAK)                                   */

#define RFAL_ISO15693_IGNORE_BITS       rfalConvBytesToBits(2U)                       /*!< Ignore collisions before the UID (RES_FLAG + DSFID)                             */
#define RFAL_ISO15693_INV_RES_LEN       12U                                           /*!< ISO15693 Inventory response length with CRC (bytes)                             */
#define RFAL_ISO15693_INV_RES_DUR       4U                                            /*!< ISO15693 Inventory response duration @ 26 kbps (ms)                             */

#define RFAL_WU_MIN_WEIGHT_VAL          4U                                            /*!< ST25R3916 minimum Wake-up weight value                                         */

/*******************************************************************************/

#define RFAL_LM_GT                      rfalConvUsTo1fc(100U)                         /*!< Listen Mode Guard Time enforced (GT - Passive; TIRFG - Active)                  */
#define RFAL_FDT_POLL_ADJUSTMENT        rfalConvUsTo1fc(80U)                          /*!< FDT Poll adjustment: Time between the expiration of GPT to the actual Tx        */
#define RFAL_FDT_LISTEN_MRT_ADJUSTMENT  64U                                           /*!< MRT jitter adjustment: timeout will be between [ tout ; tout + 64 cycles ]      */
#define RFAL_AP2P_FIELDOFF_TCMDOFF      1356U                                         /*!< Time after TXE and Field Off t,CMD,OFF     Activity 2.1  3.2.1.3 & C            */

#ifndef RFAL_ST25R3916_AAT_SETTLE
  #define RFAL_ST25R3916_AAT_SETTLE   5U                                            /*!< Time in ms required for AAT pins and Osc to settle after en bit set             */
#endif /* RFAL_ST25R3916_AAT_SETTLE */

#ifndef RFAL_ST25R3916B_AAT_SETTLE
  #define RFAL_ST25R3916B_AAT_SETTLE  ST25R3916_REG_MEAS_TX_DELAY_meas_tx_del_4_83ms/*!< Time between Oscillator stable and TX On in meas_tx_del steps                    */
#endif /* RFAL_ST25R3916B_AAT_SETTLE */


/*! FWT adjustment:
 *    64 : NRT jitter between TXE and NRT start      */
#define RFAL_FWT_ADJUSTMENT             64U

/*! FWT ISO14443A adjustment:
 *   512  : 4bit length
 *    64  : Half a bit duration due to ST25R3916 Coherent receiver (1/fc)         */
#define RFAL_FWT_A_ADJUSTMENT           (512U + 64U)

/*! FWT ISO14443B adjustment:
 *    SOF (14etu) + 1Byte (10etu) + 1etu (IRQ comes 1etu after first byte) - 3etu (ST25R3916 sends TXE 3etu after) */
#define RFAL_FWT_B_ADJUSTMENT           (((14U + 10U + 1U) - 3U) * 128U)


/*! FWT FeliCa 212 adjustment:
 *    1024 : Length of the two Sync bytes at 212kbps */
#define RFAL_FWT_F_212_ADJUSTMENT       1024U

/*! FWT FeliCa 424 adjustment:
 *    512 : Length of the two Sync bytes at 424kbps  */
#define RFAL_FWT_F_424_ADJUSTMENT       512U


/*! Time between our field Off and other peer field On : Tadt + (n x Trfw)
 * Ecma 340 11.1.2 - Tadt: [56.64 , 188.72] us ;  n: [0 , 3]  ; Trfw = 37.76 us
 * Should be: 189 + (3*38) = 303us ; we'll use a more relaxed setting: 605 us    */
#define RFAL_AP2P_FIELDON_TADTTRFW      rfalConvUsTo1fc(605U)


/*! FDT Listen adjustment for ISO14443A   EMVCo 2.6  4.8.1.3  ;  Digital 1.1  6.10
 *
 *  276: Time from the rising pulse of the pause of the logic '1' (i.e. the time point to measure the deaftime from),
 *       to the actual end of the EOF sequence (the point where the MRT starts). Please note that the ST25R391x uses the
 *       ISO14443-2 definition where the EOF consists of logic '0' followed by sequence Y.
 *  -64: Further adjustment for receiver to be ready just before first bit
 */
#define RFAL_FDT_LISTEN_A_ADJUSTMENT    (276U-64U)


/*! FDT Listen adjustment for ISO14443B   EMVCo 2.6  4.8.1.6  ;  Digital 1.1  7.9
 *
 *  340: Time from the rising edge of the EoS to the starting point of the MRT timer (sometime after the final high
 *       part of the EoS is completed)
 */
#define RFAL_FDT_LISTEN_B_ADJUSTMENT    340U


/*! FDT Listen adjustment for ISO15693
 * ISO15693 2000  8.4  t1 MIN = 4192/fc
 * ISO15693 2009  9.1  t1 MIN = 4320/fc
 * Digital 2.1 B.5 FDTV,LISTEN,MIN  = 4310/fc
 * Set FDT Listen one step earlier than on the more recent spec versions for greater interoperability
 */
#define RFAL_FDT_LISTEN_V_ADJUSTMENT    64U


/*! FDT Poll adjustment for ISO14443B Correlator - sst 5 etu */
#define RFAL_FDT_LISTEN_B_ADJT_CORR     128U


/*! FDT Poll adjustment for ISO14443B Correlator sst window - 5 etu */
#define RFAL_FDT_LISTEN_B_ADJT_CORR_SST 20U



/*
******************************************************************************
* GLOBAL MACROS
******************************************************************************
*/

/*! Calculates Transceive Sanity Timer. It accounts for the slowest bit rate and the longest data format
 *    1s for transmission and reception of a 4K message at 106kpbs (~425ms each direction)
 *       plus TxRx preparation and FIFO load over Serial Interface                                      */
#define rfalCalcSanityTmr( fwt )                 (uint16_t)(1000U + rfalConv1fcToMs((fwt)))

#define rfalGennTRFW( n )                        ((uint8_t)(((n)+1U)%7U))                                          /*!< Generate next n*TRFW used for RFCA: modulo a prime to avoid alias effects */

#define rfalCalcNumBytes( nBits )                (((uint32_t)(nBits) + 7U) / 8U)                                   /*!< Returns the number of bytes required to fit given the number of bits */

#define rfalTimerStart( timer, time_ms )         (timer) = timerCalculateTimer((uint16_t)(time_ms))                /*!< Configures and starts the RTOX timer                                 */
#define rfalTimerisExpired( timer )              timerIsExpired( timer )                                   /*!< Checks if timer has expired                                          */

#define rfalST25R3916ObsModeDisable()            st25r3916WriteTestRegister(0x01U, (0x40U))                        /*!< Disable ST25R3916 Observation mode                                   */
#define rfalST25R3916ObsModeTx()                 st25r3916WriteTestRegister(0x01U, (0x40U|gRFAL.conf.obsvModeTx))  /*!< Enable Tx Observation mode                                           */
#define rfalST25R3916ObsModeRx()                 st25r3916WriteTestRegister(0x01U, (0x40U|gRFAL.conf.obsvModeRx))  /*!< Enable Rx Observation mode                                           */


#define rfalCheckDisableObsMode()                if(gRFAL.conf.obsvModeRx != 0U){ rfalST25R3916ObsModeDisable(); } /*!< Checks if the observation mode is enabled, and applies on ST25R3916  */
#define rfalCheckEnableObsModeTx()               if(gRFAL.conf.obsvModeTx != 0U){ rfalST25R3916ObsModeTx(); }      /*!< Checks if the observation mode is enabled, and applies on ST25R3916  */
#define rfalCheckEnableObsModeRx()               if(gRFAL.conf.obsvModeRx != 0U){ rfalST25R3916ObsModeRx(); }      /*!< Checks if the observation mode is enabled, and applies on ST25R3916  */


#define rfalGetIncmplBits( FIFOStatus2 )         (( (FIFOStatus2) >> 1) & 0x07U)                                           /*!< Returns the number of bits from fifo status                  */
#define rfalIsIncompleteByteError( error )       (((error) >= ERR_INCOMPLETE_BYTE) && ((error) <= ERR_INCOMPLETE_BYTE_07)) /*!< Checks if given error is a Incomplete error                  */

#define rfalAdjACBR( b )                         (((uint16_t)(b) >= (uint16_t)RFAL_BR_52p97) ? (uint16_t)(b) : ((uint16_t)(b)+1U))          /*!< Adjusts ST25R391x Bit rate to Analog Configuration              */
#define rfalConvBR2ACBR( b )                     (((rfalAdjACBR((b)))<<RFAL_ANALOG_CONFIG_BITRATE_SHIFT) & RFAL_ANALOG_CONFIG_BITRATE_MASK) /*!< Converts ST25R391x Bit rate to Analog Configuration bit rate id */

#define rfalConvTDFormat( v )                    ((uint16_t)(v) << 8U) /*!< Converts a uint8_t to the format used in SW Tag Detection */
#define rfalAddFracTDFormat( fd )                ((((uint16_t)(fd)) & 0x03U) * 64U)


#define meowSt25RunBlocking( e, fn )                 do{ (e)=(fn); rfalWorker(); }while( (e) == ERR_BUSY )



class RfalRfST25R3916Class : public RfalRfClass {
  public:

    /*
    ******************************************************************************
    * RFAL RF FUNCTION PROTOTYPES
    ******************************************************************************
    */

    RfalRfST25R3916Class(SPIClass *spi, int cs_pin, int int_pin, uint32_t spi_speed = 5000000);
    RfalRfST25R3916Class(MeowNfcBus *i2c, int int_pin);
    ReturnCode rfalInitialize(void);
    ReturnCode rfalCalibrate(void);
    ReturnCode rfalAdjustRegulators(uint16_t *result);
    void rfalSetUpperLayerCallback(rfalUpperLayerCallback pFunc);
    void rfalSetPreTxRxCallback(rfalPreTxRxCallback pFunc);
    void rfalSetPostTxRxCallback(rfalPostTxRxCallback pFunc);
    void rfalSetLmEonCallback(rfalLmEonCallback pFunc);
    ReturnCode rfalDeinitialize(void);
    ReturnCode rfalSetMode(rfalMode mode, rfalBitRate txBR, rfalBitRate rxBR);
    rfalMode rfalGetMode(void);
    ReturnCode rfalSetBitRate(rfalBitRate txBR, rfalBitRate rxBR);
    ReturnCode rfalGetBitRate(rfalBitRate *txBR, rfalBitRate *rxBR);
    void rfalSetErrorHandling(rfalEHandling eHandling);
    rfalEHandling rfalGetErrorHandling(void);
    void rfalSetObsvMode(uint32_t txMode, uint32_t rxMode);
    void rfalGetObsvMode(uint8_t *txMode, uint8_t *rxMode);
    void rfalDisableObsvMode(void);
    void rfalSetFDTPoll(uint32_t FDTPoll);
    uint32_t rfalGetFDTPoll(void);
    void rfalSetFDTListen(uint32_t FDTListen);
    uint32_t rfalGetFDTListen(void);
    uint32_t rfalGetGT(void);
    void rfalSetGT(uint32_t GT);
    bool rfalIsGTExpired(void);
    ReturnCode rfalFieldOnAndStartGT(void);
    ReturnCode rfalFieldOff(void);
    ReturnCode rfalStartTransceive(const rfalTransceiveContext *ctx);
    rfalTransceiveState rfalGetTransceiveState(void);
    ReturnCode rfalGetTransceiveStatus(void);
    bool rfalIsTransceiveInTx(void);
    bool rfalIsTransceiveInRx(void);
    ReturnCode rfalGetTransceiveRSSI(uint16_t *rssi);
    void rfalWorker(void);
    void rfalSetSyncTxRxCallback(rfalSyncTxRxCallback pFunc);
    bool rfalIsTransceiveSubcDetected(void);
    ReturnCode rfalISO14443AStartTransceiveAnticollisionFrame(uint8_t *buf, uint8_t *bytesToSend, uint8_t *bitsToSend, uint16_t *rxLength, uint32_t fwt);
    ReturnCode rfalISO14443AGetTransceiveAnticollisionFrameStatus(void);

    ReturnCode rfalLowPowerModeStart(rfalLpMode mode);
    ReturnCode rfalLowPowerModeStop(void);
    ReturnCode rfalChipMeasureIQ(int8_t *resI, int8_t *resQ);
    ReturnCode rfalChipMeasureCombinedIQ(uint8_t *result);
    ReturnCode rfalChipSetAntennaMode(bool single, bool rfiox);
    ReturnCode rfalRunListenModeWorker(void);
    ReturnCode rfalISO14443ATransceiveShortFrame(rfal14443AShortFrameCmd txCmd, uint8_t *rxBuf, uint8_t rxBufLen, uint16_t *rxRcvdLen, uint32_t fwt);
    ReturnCode rfalISO14443ATransceiveAnticollisionFrame(uint8_t *buf, uint8_t *bytesToSend, uint8_t *bitsToSend, uint16_t *rxLength, uint32_t fwt);
    ReturnCode rfalFeliCaPoll(rfalFeliCaPollSlots slots, uint16_t sysCode, uint8_t reqCode, rfalFeliCaPollRes *pollResList, uint8_t pollResListSize, uint8_t *devicesDetected, uint8_t *collisionsDetected);
    ReturnCode rfalStartFeliCaPoll(rfalFeliCaPollSlots slots, uint16_t sysCode, uint8_t reqCode, rfalFeliCaPollRes *pollResList, uint8_t pollResListSize, uint8_t *devicesDetected, uint8_t *collisionsDetected);
    ReturnCode rfalGetFeliCaPollStatus(void);
    ReturnCode rfalISO15693TransceiveAnticollisionFrame(uint8_t *txBuf, uint8_t txBufLen, uint8_t *rxBuf, uint8_t rxBufLen, uint16_t *actLen);
    ReturnCode rfalISO15693TransceiveEOFAnticollision(uint8_t *rxBuf, uint8_t rxBufLen, uint16_t *actLen);
    ReturnCode rfalISO15693TransceiveEOF(uint8_t *rxBuf, uint16_t rxBufLen, uint16_t *actLen);
    ReturnCode rfalTransceiveBlockingTx(uint8_t *txBuf, uint16_t txBufLen, uint8_t *rxBuf, uint16_t rxBufLen, uint16_t *actLen, uint32_t flags, uint32_t fwt);
    ReturnCode rfalTransceiveBlockingRx(void);
    ReturnCode rfalTransceiveBlockingTxRx(uint8_t *txBuf, uint16_t txBufLen, uint8_t *rxBuf, uint16_t rxBufLen, uint16_t *actLen, uint32_t flags, uint32_t fwt);
    bool rfalIsExtFieldOn(void);
    ReturnCode rfalListenStart(uint32_t lmMask, const rfalLmConfPA *confA, const rfalLmConfPB *confB, const rfalLmConfPF *confF, uint8_t *rxBuf, uint16_t rxBufLen, uint16_t *rxLen);
    ReturnCode rfalListenSleepStart(rfalLmState sleepSt, uint8_t *rxBuf, uint16_t rxBufLen, uint16_t *rxLen);
    ReturnCode rfalListenStop(void);
    rfalLmState rfalListenGetState(bool *dataFlag, rfalBitRate *lastBR);
    ReturnCode rfalListenSetState(rfalLmState newSt);
    bool rfalWakeUpModeIsEnabled(void);
    ReturnCode rfalWakeUpModeStart(const rfalWakeUpConfig *config);
    ReturnCode rfalWakeUpModeGetInfo(bool force, rfalWakeUpInfo *info);
    bool rfalWakeUpModeHasWoke(void);
    ReturnCode rfalWakeUpModeStop(void);

    /*
    ******************************************************************************
    * RFAL ANALOG CONFIG FUNCTION PROTOTYPES
    ******************************************************************************
    */


    /*!
     *****************************************************************************
     * \brief Initialize the Analog Configuration
     *
     * Reset the Analog Configuration LUT pointer to reference to default settings.
     *
     *****************************************************************************
     */
    void rfalAnalogConfigInitialize(void);


    /*!
     *****************************************************************************
     * \brief Indicate if the current Analog Configuration Table is complete and ready to be used.
     *
     * \return true if current Analog Configuration Table is complete and ready to be used.
     * \return false if current Analog Configuration Table is incomplete
     *
     *****************************************************************************
     */
    bool rfalAnalogConfigIsReady(void);

    /*!
     *****************************************************************************
     * \brief  Write the whole Analog Configuration table in raw format
     *
     * Writes the Analog Configuration and Look Up Table with the given raw table
     *
     * NOTE: Function does not check the validity of the given Table contents
     *
     * \param[in]  configTbl:     location of config Table to be loaded
     * \param[in]  configTblSize: size of the config Table to be loaded
     *
     * \return ERR_NONE    : if setting is updated
     * \return ERR_PARAM   : if configTbl is invalid
     * \return ERR_NOMEM   : if the given Table is bigger exceeds the max size
     * \return ERR_REQUEST : if the update Configuration Id is disabled
     *
     *****************************************************************************
     */
    ReturnCode rfalAnalogConfigListWriteRaw(const uint8_t *configTbl, uint16_t configTblSize);

    /*!
     *****************************************************************************
     * \brief  Write the Analog Configuration table with new analog settings.
     *
     * Writes the Analog Configuration and Look Up Table with the new list of register-mask-value
     * and Configuration ID respectively.
     *
     * NOTE: Function does not check for the validity of the Register Address.
     *
     * \param[in]  more: 0x00 indicates it is last Configuration ID settings;
     *                   0x01 indicates more Configuration ID setting(s) are coming.
     * \param[in]  *config: reference to the configuration list of current Configuration ID.
     *
     * \return ERR_PARAM   : if Configuration ID or parameter is invalid
     * \return ERR_NOMEM   : if LUT is full
     * \return ERR_REQUEST : if the update Configuration Id is disabled
     * \return ERR_NONE    : if setting is updated
     *
     *****************************************************************************
     */
    ReturnCode rfalAnalogConfigListWrite(uint8_t more, const rfalAnalogConfig *config);

    /*!
     *****************************************************************************
     * \brief  Read the whole Analog Configuration table in raw format
     *
     * Reads the whole Analog Configuration Table in raw format
     *
     * \param[out]   tblBuf: location to the buffer to place the Config Table
     * \param[in]    tblBufLen: length of the buffer to place the Config Table
     * \param[out]   configTblSize: Config Table size
     *
     * \return ERR_PARAM : if configTbl or configTblSize is invalid
     * \return ERR_NOMEM : if configTblSize is not enough for the whole table
     * \return ERR_NONE  : if read is successful
     *
     *****************************************************************************
     */
    ReturnCode rfalAnalogConfigListReadRaw(uint8_t *tblBuf, uint16_t tblBufLen, uint16_t *configTblSize);

    /*!
     *****************************************************************************
     * \brief  Read the Analog Configuration table.
     *
     * Read the Analog Configuration Table
     *
     * \param[in]     configOffset: offset to the next Configuration ID in the List Table to be read.
     * \param[out]    more: 0x00 indicates it is last Configuration ID settings;
     *                      0x01 indicates more Configuration ID setting(s) are coming.
     * \param[out]    config: configuration id, number of configuration sets and register-mask-value sets
     * \param[in]     numConfig: the remaining configuration settings space available;
     *
     * \return ERR_NOMEM : if number of Configuration for respective Configuration ID is greater the the remaining configuration setting space available
     * \return ERR_NONE  : if read is successful
     *
     *****************************************************************************
     */
    ReturnCode rfalAnalogConfigListRead(rfalAnalogConfigOffset *configOffset, uint8_t *more, rfalAnalogConfig *config, rfalAnalogConfigNum numConfig);

    /*!
     *****************************************************************************
     * \brief  Set the Analog settings of indicated Configuration ID.
     *
     * Update the chip with indicated analog settings of indicated Configuration ID.
     *
     * \param[in]  configId: configuration ID
     *
     * \return ERR_PARAM if Configuration ID is invalid
     * \return ERR_INTERNAL if error updating setting to chip
     * \return ERR_NONE if new settings is applied to chip
     *
     *****************************************************************************
     */
    ReturnCode rfalSetAnalogConfig(rfalAnalogConfigId configId);


    /*
    ******************************************************************************
    * RFAL RF CHIP FUNCTION PROTOTYPES
    ******************************************************************************
    */


    /*!
     *****************************************************************************
     * \brief Writes a register on the RF Chip
     *
     * Checks if the given register is valid and if so, writes the value(s)
     * on the RF Chip register
     *
     * \param[in] reg: register address to be written, or the first if len > 1
     * \param[in] values: pointer with content to be written on the register(s)
     * \param[in] len: number of consecutive registers to be written
     *
     *
     * \return ERR_PARAM    : Invalid register or bad request
     * \return ERR_NOTSUPP  : Feature not supported
     * \return ERR_NONE     : Write done with no error
     *****************************************************************************
     */
    ReturnCode rfalChipWriteReg(uint16_t reg, const uint8_t *values, uint8_t len);

    /*!
     *****************************************************************************
     * \brief Reads a register on the RF Chip
     *
     * Checks if the given register is valid and if so, reads the value(s)
     * of the RF Chip register(s)
     *
     * \param[in]  reg: register address to be read, or the first if len > 1
     * \param[out] values: pointer where the register(s) read content will be placed
     * \param[in]  len: number of consecutive registers to be read
     *
     * \return ERR_PARAM    : Invalid register or bad request
     * \return ERR_NOTSUPP  : Feature not supported
     * \return ERR_NONE     : Read done with no error
     *****************************************************************************
     */
    ReturnCode rfalChipReadReg(uint16_t reg, uint8_t *values, uint8_t len);

    /*!
     *****************************************************************************
     * \brief Change a register on the RF Chip
     *
     * Change the value of the register bits on the RF Chip Test set in the valueMask.
     *
     * \param[in] reg: register address to be modified
     * \param[in] valueMask: mask value of the register bits to be changed
     * \param[in] value: register value to be set
     *
     * \return ERR_PARAM    : Invalid register or bad request
     * \return ERR_NOTSUPP  : Feature not supported
     * \return ERR_OK       : Change done with no error
     *****************************************************************************
     */
    ReturnCode rfalChipChangeRegBits(uint16_t reg, uint8_t valueMask, uint8_t value);

    /*!
     *****************************************************************************
     * \brief Writes a Test register on the RF Chip
     *
     * Writes the value on the RF Chip Test register
     *
     * \param[in] reg: register address to be written
     * \param[in] value: value to be written on the register
     *
     *
     * \return ERR_PARAM    : Invalid register or bad request
     * \return ERR_NOTSUPP  : Feature not supported
     * \return ERR_NONE     : Write done with no error
     *****************************************************************************
     */
    ReturnCode rfalChipWriteTestReg(uint16_t reg, uint8_t value);

    /*!
     *****************************************************************************
     * \brief Reads a Test register on the RF Chip
     *
     * Reads the value of the RF Chip Test register
     *
     * \param[in]  reg: register address to be read
     * \param[out] value: pointer where the register content will be placed
     *
     * \return ERR_PARAM    :Invalid register or bad request
     * \return ERR_NOTSUPP  : Feature not supported
     * \return ERR_NONE     : Read done with no error
     *****************************************************************************
     */
    ReturnCode rfalChipReadTestReg(uint16_t reg, uint8_t *value);

    /*!
     *****************************************************************************
     * \brief Change a Test register on the RF Chip
     *
     * Change the value of the register bits on the RF Chip Test set in the valueMask.
     *
     * \param[in] reg: test register address to be modified
     * \param[in] valueMask: mask value of the register bits to be changed
     * \param[in] value: register value to be set
     *
     * \return ERR_PARAM     : Invalid register or bad request
     * \return ERR_NOTSUPP   : Feature not supported
     * \return ERR_OK        : Change done with no error
     *****************************************************************************
     */
    ReturnCode rfalChipChangeTestRegBits(uint16_t reg, uint8_t valueMask, uint8_t value);

    /*!
     *****************************************************************************
     * \brief Execute command on the RF Chip
     *
     * Checks if the given command is valid and if so, executes it on
     * the RF Chip
     *
     * \param[in] cmd: direct command to be executed
     *
     * \return ERR_PARAM     : Invalid command or bad request
     * \return  ERR_NOTSUPP  : Feature not supported
     * \return ERR_NONE      : Direct command executed with no error
     *****************************************************************************
     */
    ReturnCode rfalChipExecCmd(uint16_t cmd);

    /*!
     *****************************************************************************
     * \brief  Set RFO
     *
     * Sets the RFO value to be used when the field is on (unmodulated/active)
     *
     * \param[in] rfo : the RFO value to be used
     *
     * \return  ERR_IO           : Internal error
     * \return  ERR_NOTSUPP      : Feature not supported
     * \return  ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalChipSetRFO(uint8_t rfo);


    /*!
     *****************************************************************************
     * \brief  Get RFO
     *
     * Gets the RFO value used used when the field is on (unmodulated/active)
     *
     * \param[out] result : the current RFO value
     *
     * \return  ERR_IO           : Internal error
     * \return  ERR_NOTSUPP      : Feature not supported
     * \return  ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalChipGetRFO(uint8_t *result);


    /*!
     *****************************************************************************
     * \brief  Measure Amplitude
     *
     * Measures the RF Amplitude
     *
     * \param[out] result : result of RF measurement
     *
     * \return  ERR_IO           : Internal error
     * \return  ERR_NOTSUPP      : Feature not supported
     * \return  ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalChipMeasureAmplitude(uint8_t *result);


    /*!
     *****************************************************************************
     * \brief  Measure Phase
     *
     * Measures the Phase
     *
     * \param[out] result : result of Phase measurement
     *
     * \return  ERR_IO           : Internal error
     * \return  ERR_NOTSUPP      : Feature not supported
     * \return  ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalChipMeasurePhase(uint8_t *result);


    /*!
     *****************************************************************************
     * \brief  Measure Capacitance
     *
     * Measures the Capacitance
     *
     * \param[out] result : result of Capacitance measurement
     *
     * \return  ERR_IO           : Internal error
     * \return  ERR_NOTSUPP      : Feature not supported
     * \return  ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalChipMeasureCapacitance(uint8_t *result);


    /*!
     *****************************************************************************
     * \brief  Measure Power Supply
     *
     * Measures the Power Supply
     *
     * \param[in]   param : measurement parameter (chip specific)
     * \param[out] result : result of the measurement
     *
     * \return  ERR_IO           : Internal error
     * \return  ERR_NOTSUPP      : Feature not supported
     * \return  ERR_NONE         : No error
     *****************************************************************************
     */
    ReturnCode rfalChipMeasurePowerSupply(uint8_t param, uint8_t *result);


    /*
    ******************************************************************************
    * RFAL CRC FUNCTION PROTOTYPES
    ******************************************************************************
    */


    /*!
     *****************************************************************************
     *  \brief  Calculate CRC according to CCITT standard.
     *
     *  This function takes \a length bytes from \a buf and calculates the CRC
     *  for this data. The result is returned.
     *  \note This implementation calculates the CRC with LSB first, i.e. all
     *  bytes are "read" from right to left.
     *
     *  \param[in] preloadValue : Initial value of CRC calculation.
     *  \param[in] buf : buffer to calculate the CRC for.
     *  \param[in] length : size of the buffer.
     *
     *  \return 16 bit long crc value.
     *
     *****************************************************************************
     */
    uint16_t rfalCrcCalculateCcitt(uint16_t preloadValue, const uint8_t *buf, uint16_t length);


    /*
    ******************************************************************************
    * RFAL ISO 15693_2 FUNCTION PROTOTYPES
    ******************************************************************************
    */


    /*!
     *****************************************************************************
     *  \brief  Initialize the ISO15693 phy
     *
     *  \param[in] config : ISO15693 phy related configuration (See #iso15693PhyConfig_t)
     *  \param[out] needed_stream_config : return a pointer to the stream config
     *              needed for this rfalIso15693 config. To be used for configure RF chip.
     *
     *  \return ERR_IO : Error during communication.
     *  \return ERR_NONE : No error.
     *
     *****************************************************************************
     */
    ReturnCode rfalIso15693PhyConfigure(const rfalIso15693PhyConfig_t *config,
                                        const struct rfalIso15693StreamConfig **needed_stream_config);

    /*!
     *****************************************************************************
     *  \brief  Return current phy configuration
     *
     *  This function returns current Phy configuration previously
     *  set by #iso15693PhyConfigure
     *
     *  \param[out] config : ISO15693 phy configuration.
     *
     *  \return ERR_NONE : No error.
     *
     *****************************************************************************
     */
    ReturnCode rfalIso15693PhyGetConfiguration(rfalIso15693PhyConfig_t *config);

    /*!
     *****************************************************************************
     *  \brief  Code an ISO15693 compatible frame
     *
     *  This function takes \a length bytes from \a buffer, perform proper
     *  encoding and sends out the frame to the ST25R391x.
     *
     *  \param[in] buffer : data to send, modified to adapt flags.
     *  \param[in] length : number of bytes to send.
     *  \param[in] sendCrc : If set to true, CRC is appended to the frame
     *  \param[in] sendFlags: If set to true, flag field is sent according to
     *                        ISO15693.
     *  \param[in] picopassMode :  If set to true, the coding will be according to Picopass
     *  \param[out] subbit_total_length : Return the complete bytes which need to
     *                                   be send for the current coding
     *  \param[in,out] offset : Set to 0 for first transfer, function will update it to
                  point to next byte to be coded
     *  \param[out] outbuf : buffer where the function will store the coded subbit stream
     *  \param[out] outBufSize : the size of the output buffer
     *  \param[out] actOutBufSize : the amount of data stored into the buffer at this call
     *
     *  \return ERR_IO : Error during communication.
     *  \return ERR_AGAIN : Data was not coded all the way. Call function again with a new/emptied buffer
     *  \return ERR_NO_MEM : In case outBuf is not big enough. Needs to have at
                 least 5 bytes for 1of4 coding and 65 bytes for 1of256 coding
     *  \return ERR_NONE : No error.
     *
     *****************************************************************************
     */
    ReturnCode rfalIso15693VCDCode(uint8_t *buffer, uint16_t length, bool sendCrc, bool sendFlags, bool picopassMode,
                                   uint16_t *subbit_total_length, uint16_t *offset,
                                   uint8_t *outbuf, uint16_t outBufSize, uint16_t *actOutBufSize);


    /*!
     *****************************************************************************
     *  \brief  Receive an ISO15693 compatible frame
     *
     *  This function receives an ISO15693 frame from the ST25R391x, decodes the frame
     *  and writes the raw data to \a buffer.
     *  \note Buffer needs to be big enough to hold CRC also (+2 bytes)
     *
     *  \param[in] inBuf : buffer with the hamming coded stream to be decoded
     *  \param[in] inBufLen : number of bytes to decode (=length of buffer).
     *  \param[out] outBuf : buffer where received data shall be written to.
     *  \param[in] outBufLen : Length of output buffer, should be approx twice the size of inBuf
     *  \param[out] outBufPos : The number of decoded bytes. Could be used in
     *                          extended implementation to allow multiple calls
     *  \param[out] bitsBeforeCol : in case of ERR_COLLISION this value holds the
     *   number of bits in the current byte where the collision happened.
     *  \param[in] ignoreBits : number of bits in the beginning where collisions will be ignored
     *  \param[in] picopassMode :  if set to true, the decoding will be according to Picopass
     *
     *  \return ERR_COLLISION : collision occurred, data incorrect
     *  \return ERR_CRC : CRC error, data incorrect
     *  \return ERR_TIMEOUT : timeout waiting for data.
     *  \return ERR_NONE : No error.
     *
     *****************************************************************************
     */
    ReturnCode rfalIso15693VICCDecode(const uint8_t *inBuf,
                                      uint16_t inBufLen,
                                      uint8_t *outBuf,
                                      uint16_t outBufLen,
                                      uint16_t *outBufPos,
                                      uint16_t *bitsBeforeCol,
                                      uint16_t ignoreBits,
                                      bool picopassMode);


    /*
    ******************************************************************************
    * RFAL ST25R3916 FUNCTION PROTOTYPES
    ******************************************************************************
    */


    /*!
     *****************************************************************************
     *  \brief  Initialise ST25R3916 driver
     *
     *  This function initialises the ST25R3916 driver.
     *
     *  \return ERR_NONE         : Operation successful
     *  \return ERR_HW_MISMATCH  : Expected HW do not match or communication error
     *****************************************************************************
     */
    ReturnCode st25r3916Initialize(void);

    /*!
     *****************************************************************************
     *  \brief  Deinitialize ST25R3916 driver
     *
     *  Calling this function deinitializes the ST25R3916 driver.
     *
     *****************************************************************************
     */
    void st25r3916Deinitialize(void);

    /*!
     *****************************************************************************
     *  \brief  Turn on Oscillator and Regulator
     *
     *  This function turn on oscillator and regulator and waits for the
     *  oscillator to become stable
     *
     *****************************************************************************
     */
    ReturnCode st25r3916OscOn(void);

    /*!
     *****************************************************************************
     *  \brief  Sets the bitrate
     *
     *  This function sets the bitrates for rx and tx
     *
     *  \param txrate : speed is 2^txrate * 106 kb/s
     *                  0xff : don't set txrate (ST25R3916_BR_DO_NOT_SET)
     *  \param rxrate : speed is 2^rxrate * 106 kb/s
     *                  0xff : don't set rxrate (ST25R3916_BR_DO_NOT_SET)
     *
     *  \return ERR_PARAM: At least one bit rate was invalid
     *  \return ERR_NONE : No error, both bit rates were set
     *
     *****************************************************************************
     */
    ReturnCode st25r3916SetBitrate(uint8_t txrate, uint8_t rxrate);

    /*!
     *****************************************************************************
     *  \brief  Adjusts supply regulators according to the current supply voltage
     *
     *  This function the power level is measured in maximum load conditions and
     *  the regulated voltage reference is set to 250mV below this level.
     *  Execution of this function lasts around 5ms.
     *
     *  The regulated voltages will be set to the result of Adjust Regulators
     *
     *  \param [out] result_mV : Result of calibration in milliVolts
     *
     *  \return ERR_IO : Error during communication with ST25R3916
     *  \return ERR_NONE : No error
     *
     *****************************************************************************
     */
    ReturnCode st25r3916AdjustRegulators(uint16_t *result_mV);

    /*!
     *****************************************************************************
     *  \brief  Measure Amplitude
     *
     *  This function measured the amplitude on the RFI inputs and stores the
     *  result in parameter \a result.
     *
     *  \param[out] result:  result of RF measurement.
     *
     *  \return ERR_PARAM : Invalid parameter
     *  \return ERR_NONE  : No error
     *
     *****************************************************************************
     */
    ReturnCode st25r3916MeasureAmplitude(uint8_t *result);

    /*!
     *****************************************************************************
     *  \brief  Measure Power Supply
     *
     *  This function executes Measure Power Supply and returns the raw value
     *
     *  \param[in] mpsv : one of ST25R3916_REG_REGULATOR_CONTROL_mpsv_vdd
     *                           ST25R3916_REG_REGULATOR_CONTROL_mpsv_vdd_rf
     *                           ST25R3916_REG_REGULATOR_CONTROL_mpsv_vdd_a
     *                           ST25R3916_REG_REGULATOR_CONTROL_mpsv_vdd_d
     *                           ST25R3916_REG_REGULATOR_CONTROL_mpsv_vdd_am
     *
     *  \return the measured voltage in raw format.
     *
     *****************************************************************************
     */
    uint8_t st25r3916MeasurePowerSupply(uint8_t mpsv);

    /*!
     *****************************************************************************
     *  \brief  Measure Voltage
     *
     *  This function measures the voltage on one of VDD and VDD_* and returns
     *  the result in mV
     *
     *  \param[in] mpsv : one of ST25R3916_REG_REGULATOR_CONTROL_mpsv_vdd
     *                           ST25R3916_REG_REGULATOR_CONTROL_mpsv_vdd_rf
     *                           ST25R3916_REG_REGULATOR_CONTROL_mpsv_vdd_a
     *                           ST25R3916_REG_REGULATOR_CONTROL_mpsv_vdd_d
     *                    or     ST25R3916_REG_REGULATOR_CONTROL_mpsv_vdd_am
     *
     *  \return the measured voltage in mV
     *
     *****************************************************************************
     */
    uint16_t st25r3916MeasureVoltage(uint8_t mpsv);

    /*!
     *****************************************************************************
     *  \brief  Measure Phase
     *
     *  This function performs a Phase measurement.
     *  The result is stored in the \a result parameter.
     *
     *  \param[out] result: 8 bit long result of the measurement.
     *
     *  \return ERR_PARAM : Invalid parameter
     *  \return ERR_NONE  : No error
     *
     *****************************************************************************
     */
    ReturnCode st25r3916MeasurePhase(uint8_t *result);


    /*!
     *****************************************************************************
     *  \brief  Measure Capacitance
     *
     *  This function performs the capacitance measurement and stores the
     *  result in parameter \a result.
     *
     *  \param[out] result: 8 bit long result of RF measurement.
     *
     *  \return ERR_PARAM : Invalid parameter
     *  \return ERR_NONE  : No error
     *
     *****************************************************************************
     */
    ReturnCode st25r3916MeasureCapacitance(uint8_t *result);

    /*!
     *****************************************************************************
     *  \brief  Calibrates Capacitive Sensor
     *
     *  This function performs automatic calibration of the capacitive sensor
     *  and stores the result in parameter \a result.
     *
     * \warning To avoid interference with Xtal oscillator and reader magnetic
     *          field, it is strongly recommended to perform calibration
     *          in Power-down mode only.
     *          This method does not modify the Oscillator nor transmitter state,
     *          these should be configured before by user.
     *
     *  \param[out] result: 5 bit long result of the calibration.
     *                      Binary weighted, step 0.1 pF, max 3.1 pF
     *
     *  \return ERR_PARAM : Invalid parameter
     *  \return ERR_IO    : The calibration was not successful
     *  \return ERR_NONE  : No error
     *
     *****************************************************************************
     */
    ReturnCode st25r3916CalibrateCapacitiveSensor(uint8_t *result);

    /*!
     *****************************************************************************
     *  \brief  Get NRT time
     *
     *  This returns the last value set on the NRT
     *
     *  \warning it does not read chip register, just the sw var that contains the
     *  last value set before
     *
     *  \return the value of the NRT in 64/fc
     */
    uint32_t st25r3916GetNoResponseTime(void);

    /*!
     *****************************************************************************
     *  \brief  Set NRT time
     *
     *  This function sets the No Response Time with the given value
     *
     *  \param [in] nrt_64fcs : no response time in steps of 64/fc (4.72us)
     *
     *  \return ERR_PARAM : Invalid parameter (time is too large)
     *  \return ERR_NONE  : No error
     *
     *****************************************************************************
     */
    ReturnCode st25r3916SetNoResponseTime(uint32_t nrt_64fcs);

    /*!
     *****************************************************************************
     *  \brief  Set and Start NRT
     *
     *  This function sets the No Response Time with the given value and
     *  immediately starts it
     *  Used when needs to add more time before timeout without performing Tx
     *
     *  \param [in] nrt_64fcs : no response time in steps of 64/fc (4.72us)
     *
     *  \return ERR_PARAM : Invalid parameter
     *  \return ERR_NONE  : No error
     *
     *****************************************************************************
     */
    ReturnCode st25r3916SetStartNoResponseTimer(uint32_t nrt_64fcs);

    /*!
     *****************************************************************************
     *  \brief  Set GPT time
     *
     *  This function sets the General Purpose Timer time registers
     *
     *  \param [in] gpt_8fcs : general purpose timer timeout in steps of 8/fc (590ns)
     *
     *****************************************************************************
     */
    void st25r3916SetGPTime(uint16_t gpt_8fcs);

    /*!
     *****************************************************************************
     *  \brief  Set and Start GPT
     *
     *  This function sets the General Purpose Timer with the given timeout and
     *  immediately starts it ONLY if the trigger source is not set to none.
     *
     *  \param [in] gpt_8fcs : general purpose timer timeout in  steps of8/fc (590ns)
     *  \param [in] trigger_source : no trigger, start of Rx, end of Rx, end of Tx in NFC mode
     *
     *  \return ERR_PARAM : Invalid parameter
     *  \return ERR_NONE  : No error
     *
     *****************************************************************************
     */
    ReturnCode st25r3916SetStartGPTimer(uint16_t gpt_8fcs, uint8_t trigger_source);

    /*!
     *****************************************************************************
     *  \brief  Sets the number Tx Bits
     *
     *  Sets ST25R3916 internal registers with correct number of complete bytes and
     *  bits to be sent
     *
     *  \param [in] nBits : number of bits to be set/transmitted
     *
     *****************************************************************************
     */
    void st25r3916SetNumTxBits(uint16_t nBits);

    /*!
     *****************************************************************************
     *  \brief  Get amount of bytes in FIFO
     *
     *  Gets the number of bytes currently in the FIFO
     *
     *  \return the number of bytes currently in the FIFO
     *
     *****************************************************************************
     */
    uint16_t st25r3916GetNumFIFOBytes(void);

    /*!
     *****************************************************************************
     *  \brief  Get amount of bits of the last FIFO byte if incomplete
     *
     *  Gets the number of bits of the last FIFO byte if incomplete
     *
     *  \return the number of bits of the last FIFO byte if incomplete, 0 if
     *          the last byte is complete
     *
     *****************************************************************************
     */
    uint8_t st25r3916GetNumFIFOLastBits(void);

    /*!
     *****************************************************************************
     *  \brief  Perform Collision Avoidance
     *
     *  Performs Collision Avoidance with the given threshold and with the
     *  n number of TRFW
     *
     *  \param[in] FieldONCmd  : Field ON command to be executed ST25R3916_CMD_INITIAL_RF_COLLISION
     *                           or ST25R3916_CMD_RESPONSE_RF_COLLISION_N
     *  \param[in] pdThreshold : Peer Detection Threshold  (ST25R3916_REG_FIELD_THRESHOLD_trg_xx)
     *                           0xff : don't set Threshold (ST25R3916_THRESHOLD_DO_NOT_SET)
     *  \param[in] caThreshold : Collision Avoidance Threshold (ST25R3916_REG_FIELD_THRESHOLD_rfe_xx)
     *                           0xff : don't set Threshold (ST25R3916_THRESHOLD_DO_NOT_SET)
     *  \param[in] nTRFW       : Number of TRFW
     *
     *  \return ERR_PARAM        : Invalid parameter
     *  \return ERR_RF_COLLISION : Collision detected
     *  \return ERR_NONE         : No collision detected
     *
     *****************************************************************************
     */
    ReturnCode st25r3916PerformCollisionAvoidance(uint8_t FieldONCmd, uint8_t pdThreshold, uint8_t caThreshold, uint8_t nTRFW);

    /*!
     *****************************************************************************
     *  \brief  Check Identity
     *
     *  Checks if the chip ID is as expected.
     *
     *  5 bit IC type code for ST25R3916: 00101
     *  The 3 lsb contain the IC revision code
     *
     *  \param[out] rev : the IC revision code
     *
     *  \return  true when IC type is as expected
     *  \return  false otherwise
     */
    bool st25r3916CheckChipID(uint8_t *rev);

    /*!
     *****************************************************************************
     *  \brief  Retrieves all  internal registers from ST25R3916
     *
     *  \param[out] regDump : pointer to the struct/buffer where the reg dump
     *                        will be written
     *
     *  \return ERR_PARAM : Invalid parameter
     *  \return ERR_NONE  : No error
     *****************************************************************************
     */
    ReturnCode st25r3916GetRegsDump(t_st25r3916Regs *regDump);

    /*!
     *****************************************************************************
     *  \brief  Check if command is valid
     *
     *  Checks if the given command is a valid ST25R3916 command
     *
     *  \param[in] cmd: Command to check
     *
     *  \return  true if is a valid command
     *  \return  false otherwise
     *
     *****************************************************************************
     */
    bool st25r3916IsCmdValid(uint8_t cmd);

    /*!
     *****************************************************************************
     *  \brief  Configure the stream mode of ST25R3916
     *
     *  This function initializes the stream with the given parameters
     *
     *  \param[in] config : all settings for bitrates, type, etc.
     *
     *  \return ERR_PARAM : Invalid parameter
     *  \return ERR_NONE  : No error, stream mode driver initialized
     *
     *****************************************************************************
     */
    ReturnCode st25r3916StreamConfigure(const struct st25r3916StreamConfig *config);

    /*!
     *****************************************************************************
     *  \brief  Executes a direct command and returns the result
     *
     *  This function executes the direct command given by \a cmd waits for
     *  \a sleeptime for I_dct and returns the result read from register \a resreg.
     *  The value of cmd is not checked.
     *
     *  \param[in]  cmd   : direct command to execute
     *  \param[in]  resReg: address of the register containing the result
     *  \param[in]  tout  : time in milliseconds to wait before reading the result
     *  \param[out] result: result
     *
     *  \return ERR_NONE  : No error
     *
     *****************************************************************************
     */
    ReturnCode st25r3916ExecuteCommandAndGetResult(uint8_t cmd, uint8_t resReg, uint8_t tout, uint8_t *result);

    /*!
     *****************************************************************************
     *  \brief  Gets the RSSI values
     *
     *  This function gets the RSSI value of the previous reception taking into
     *  account the gain reductions that were used.
     *  RSSI value for both AM and PM channel can be retrieved.
     *
     *  \param[out] amRssi: the RSSI on the AM channel expressed in mV
     *  \param[out] pmRssi: the RSSI on the PM channel expressed in mV
     *
     *  \return ERR_PARAM : Invalid parameter
     *  \return ERR_NONE  : No error
     *
     *****************************************************************************
     */
    ReturnCode st25r3916GetRSSI(uint16_t *amRssi, uint16_t *pmRssi);

    /*!
    *****************************************************************************
    * \brief  Set Antenna mode
    *
    * Sets the antenna mode.
    * Differential or single ended antenna mode (RFO1 or RFO2)
    *
    *  \param[in]    single:  FALSE differential ; single ended mode
    *  \param[in]     rfiox:  FALSE   RFI1/RFO1  ; TRUE   RFI2/RFO2
    *
    * \return  RFAL_ERR_IO      : Internal error
    * \return  RFAL_ERR_NOTSUPP : Feature not supported
    * \return  RFAL_ERR_NONE    : No error
    *****************************************************************************
    */
    ReturnCode st25r3916SetAntennaMode(bool single, bool rfiox);
    /*
    ******************************************************************************
    * RFAL ST25R3916 COM FUNCTION PROTOTYPES
    ******************************************************************************
    */


    /*!
     *****************************************************************************
     *  \brief  Returns the content of a register within the ST25R3916
     *
     *  This function is used to read out the content of ST25R3916 registers.
     *
     *  \param[in]  reg: Address of register to read.
     *  \param[out] val: Returned value.
     *
     *  \return ERR_NONE  : Operation successful
     *  \return ERR_PARAM : Invalid parameter
     *  \return ERR_SEND  : Transmission error or acknowledge not received
     *****************************************************************************
     */
    ReturnCode st25r3916ReadRegister(uint8_t reg, uint8_t *val);

    /*!
     *****************************************************************************
     *  \brief  Reads from multiple ST25R3916 registers
     *
     *  This function is used to read from multiple registers using the
     *  auto-increment feature. That is, after each read the address pointer
     *  inside the ST25R3916 gets incremented automatically.
     *
     *  \param[in]  reg: Address of the first register to read from.
     *  \param[in]  values: pointer to a buffer where the result shall be written to.
     *  \param[in]  length: Number of registers to be read out.
     *
     *  \return ERR_NONE  : Operation successful
     *  \return ERR_PARAM : Invalid parameter
     *  \return ERR_SEND  : Transmission error or acknowledge not received
     *****************************************************************************
     */
    ReturnCode st25r3916ReadMultipleRegisters(uint8_t reg, uint8_t *values, uint8_t length);

    /*!
     *****************************************************************************
     *  \brief  Writes a given value to a register within the ST25R3916
     *
     *  This function is used to write \a val to address \a reg within the ST25R3916.
     *
     *  \param[in]  reg: Address of the register to write.
     *  \param[in]  val: Value to be written.
     *
     *  \return ERR_NONE  : Operation successful
     *  \return ERR_PARAM : Invalid parameter
     *  \return ERR_SEND  : Transmission error or acknowledge not received
     *****************************************************************************
     */
    ReturnCode st25r3916WriteRegister(uint8_t reg, uint8_t val);

    /*!
     *****************************************************************************
     *  \brief  Writes multiple values to ST25R3916 registers
     *
     *  This function is used to write multiple values to the ST25R3916 using the
     *  auto-increment feature. That is, after each write the address pointer
     *  inside the ST25R3916 gets incremented automatically.
     *
     *  \param[in]  reg: Address of the first register to write.
     *  \param[in]  values: pointer to a buffer containing the values to be written.
     *  \param[in]  length: Number of values to be written.
     *
     *  \return ERR_NONE  : Operation successful
     *  \return ERR_PARAM : Invalid parameter
     *  \return ERR_SEND  : Transmission error or acknowledge not received
     *****************************************************************************
     */
    ReturnCode st25r3916WriteMultipleRegisters(uint8_t reg, const uint8_t *values, uint8_t length);

    /*!
     *****************************************************************************
     *  \brief  Writes values to ST25R3916 FIFO
     *
     *  This function needs to be called in order to write to the ST25R3916 FIFO.
     *
     *  \param[in]  values: pointer to a buffer containing the values to be written
     *                      to the FIFO.
     *  \param[in]  length: Number of values to be written.
     *
     *  \return ERR_NONE  : Operation successful
     *  \return ERR_PARAM : Invalid parameter
     *  \return ERR_SEND  : Transmission error or acknowledge not received
     *****************************************************************************
     */
    ReturnCode st25r3916WriteFifo(const uint8_t *values, uint16_t length);

    /*!
     *****************************************************************************
     *  \brief  Read values from ST25R3916 FIFO
     *
     *  This function needs to be called in order to read from ST25R3916 FIFO.
     *
     *  \param[out]  buf: pointer to a buffer where the FIFO content shall be
     *                       written to.
     *  \param[in]  length: Number of bytes to read.
     *
     *  \note: This function doesn't check whether \a length is really the
     *  number of available bytes in FIFO
     *
     *  \return ERR_NONE  : Operation successful
     *  \return ERR_PARAM : Invalid parameter
     *  \return ERR_SEND  : Transmission error or acknowledge not received
     *****************************************************************************
     */
    ReturnCode st25r3916ReadFifo(uint8_t *buf, uint16_t length);

    /*!
     *****************************************************************************
     *  \brief  Writes values to ST25R3916 PTM
     *
     *  Accesses to the begging of ST25R3916 Passive Target Memory (PTM A Config)
     *  and writes the given values
     *
     *  \param[in]  values: pointer to a buffer containing the values to be written
     *                      to the Passive Target Memory.
     *  \param[in]  length: Number of values to be written.
     *
     *  \return ERR_NONE  : Operation successful
     *  \return ERR_PARAM : Invalid parameter
     *  \return ERR_SEND  : Transmission error or acknowledge not received
     *****************************************************************************
     */
    ReturnCode st25r3916WritePTMem(const uint8_t *values, uint16_t length);

    /*!
     *****************************************************************************
     *  \brief  Reads the ST25R3916 PTM
     *
     *  Accesses to the begging of ST25R3916 Passive Target Memory (PTM A Config)
     *  and reads the memory for the given length
     *
     *  \param[out] values: pointer to a buffer where the PTM content shall be
     *                       written to.
     *  \param[in]  length: Number of bytes to read.
     *
     *  \return ERR_NONE  : Operation successful
     *  \return ERR_PARAM : Invalid parameter
     *  \return ERR_SEND  : Transmission error or acknowledge not received
     *****************************************************************************
     */
    ReturnCode st25r3916ReadPTMem(uint8_t *values, uint16_t length);

    /*!
     *****************************************************************************
     *  \brief  Writes values to ST25R3916 PTM F config
     *
     *  Accesses ST25R3916 Passive Target Memory F config and writes the given values
     *
     *  \param[in]  values: pointer to a buffer containing the values to be written
     *                      to the Passive Target Memory
     *  \param[in]  length: Number of values to be written.
     *
     *  \return ERR_NONE  : Operation successful
     *  \return ERR_PARAM : Invalid parameter
     *  \return ERR_SEND  : Transmission error or acknowledge not received
     *****************************************************************************
     */
    ReturnCode st25r3916WritePTMemF(const uint8_t *values, uint16_t length);

    /*!
     *****************************************************************************
     *  \brief  Writes values to ST25R3916 PTM TSN Data
     *
     *  Accesses ST25R3916 Passive Target Memory TSN data and writes the given values
     *
     *  \param[in]  values: pointer to a buffer containing the values to be written
     *                      to the Passive Target Memory.
     *  \param[in]  length: Number of values to be written.
     *
     *  \return ERR_NONE  : Operation successful
     *  \return ERR_PARAM : Invalid parameter
     *  \return ERR_SEND  : Transmission error or acknowledge not received
     *****************************************************************************
     */
    ReturnCode st25r3916WritePTMemTSN(const uint8_t *values, uint16_t length);

    /*!
     *****************************************************************************
     *  \brief  Execute a direct command
     *
     *  This function is used to start so-called direct command. These commands
     *  are implemented inside the chip and each command has unique code (see
     *  datasheet).
     *
     *  \param[in]  cmd : code of the direct command to be executed.
     *
     *  \return ERR_NONE  : Operation successful
     *  \return ERR_PARAM : Invalid parameter
     *  \return ERR_SEND  : Transmission error or acknowledge not received
     *****************************************************************************
     */
    ReturnCode st25r3916ExecuteCommand(uint8_t cmd);

    /*!
     *****************************************************************************
     *  \brief  Read a test register within the ST25R3916
     *
     *  This function is used to read the content of test address \a reg within the ST25R3916
     *
     *  \param[in]   reg: Address of the register to read
     *  \param[out]  val: Returned read value
     *
     *  \return ERR_NONE  : Operation successful
     *  \return ERR_PARAM : Invalid parameter
     *  \return ERR_SEND  : Transmission error or acknowledge not received
     *****************************************************************************
     */
    ReturnCode st25r3916ReadTestRegister(uint8_t reg, uint8_t *val);

    /*!
     *****************************************************************************
     *  \brief  Writes a given value to a test register within the ST25R3916
     *
     *  This function is used to write \a val to test address \a reg within the ST25R3916
     *
     *  \param[in]  reg: Address of the register to write
     *  \param[in]  val: Value to be written
     *
     *  \return ERR_NONE  : Operation successful
     *  \return ERR_PARAM : Invalid parameter
     *  \return ERR_SEND  : Transmission error or acknowledge not received
     *****************************************************************************
     */
    ReturnCode st25r3916WriteTestRegister(uint8_t reg, uint8_t val);

    /*!
     *****************************************************************************
     *  \brief  Cleart bits on Register
     *
     *  This function clears the given bitmask on the register
     *
     *  \param[in]  reg: Address of the register clear
     *  \param[in]  clr_mask: Bitmask of bit to be cleared
     *
     *  \return ERR_NONE  : Operation successful
     *  \return ERR_PARAM : Invalid parameter
     *  \return ERR_SEND  : Transmission error or acknowledge not received
     *****************************************************************************
     */
    ReturnCode st25r3916ClrRegisterBits(uint8_t reg, uint8_t clr_mask);

    /*!
     *****************************************************************************
     *  \brief  Set bits on Register
     *
     *  This function sets the given bitmask on the register
     *
     *  \param[in]  reg: Address of the register clear
     *  \param[in]  set_mask: Bitmask of bit to be cleared
     *
     *  \return ERR_NONE  : Operation successful
     *  \return ERR_PARAM : Invalid parameter
     *  \return ERR_SEND  : Transmission error or acknowledge not received
     *****************************************************************************
     */
    ReturnCode st25r3916SetRegisterBits(uint8_t reg, uint8_t set_mask);

    /*!
     *****************************************************************************
     *  \brief  Changes the given bits on a ST25R3916 register
     *
     *  This function is used if only a particular bits should be changed within
     *  an ST25R3916 register.
     *
     *  \param[in]  reg: Address of the register to change.
     *  \param[in]  valueMask: bitmask of bits to be changed
     *  \param[in]  value: the bits to be written on the enabled valueMask bits
     *
     *  \return ERR_NONE  : Operation successful
     *  \return ERR_PARAM : Invalid parameter
     *  \return ERR_SEND  : Transmission error or acknowledge not received
     *****************************************************************************
     */
    ReturnCode st25r3916ChangeRegisterBits(uint8_t reg, uint8_t valueMask, uint8_t value);

    /*!
     *****************************************************************************
     *  \brief  Modifies a value within a ST25R3916 register
     *
     *  This function is used if only a particular bits should be changed within
     *  an ST25R3916 register.
     *
     *  \param[in]  reg: Address of the register to write.
     *  \param[in]  clr_mask: bitmask of bits to be cleared to 0.
     *  \param[in]  set_mask: bitmask of bits to be set to 1.
     *
     *  \return ERR_NONE  : Operation successful
     *  \return ERR_PARAM : Invalid parameter
     *  \return ERR_SEND  : Transmission error or acknowledge not received
     *****************************************************************************
     */
    ReturnCode st25r3916ModifyRegister(uint8_t reg, uint8_t clr_mask, uint8_t set_mask);

    /*!
     *****************************************************************************
     *  \brief  Changes the given bits on a ST25R3916 Test register
     *
     *  This function is used if only a particular bits should be changed within
     *  an ST25R3916 register.
     *
     *  \param[in]  reg: Address of the Test register to change.
     *  \param[in]  valueMask: bitmask of bits to be changed
     *  \param[in]  value: the bits to be written on the enabled valueMask bits
     *
     *  \return ERR_NONE  : Operation successful
     *  \return ERR_PARAM : Invalid parameter
     *  \return ERR_SEND  : Transmission error or acknowledge not received
     *****************************************************************************
     */
    ReturnCode st25r3916ChangeTestRegisterBits(uint8_t reg, uint8_t valueMask, uint8_t value);

    /*!
     *****************************************************************************
     *  \brief  Checks if register contains a expected value
     *
     *  This function checks if the given reg contains a value that once masked
     *  equals the expected value
     *
     *  \param reg  : the register to check the value
     *  \param mask : the mask apply on register value
     *  \param val  : expected value to be compared to
     *
     *  \return  true when reg contains the expected value | false otherwise
     */
    bool st25r3916CheckReg(uint8_t reg, uint8_t mask, uint8_t val);

    /*!
     *****************************************************************************
     *  \brief  Check if register ID is valid
     *
     *  Checks if the given register ID a valid ST25R3916 register
     *
     *  \param[in]  reg: Address of register to check
     *
     *  \return  true if is a valid register ID
     *  \return  false otherwise
     *
     *****************************************************************************
     */
    bool st25r3916IsRegValid(uint8_t reg);


    /*
    ******************************************************************************
    * RFAL ST25R3916 INTERRUPT FUNCTION PROTOTYPES
    ******************************************************************************
    */


    /*!
     *****************************************************************************
     *  \brief  Wait until an ST25R3916 interrupt occurs
     *
     *  This function is used to access the ST25R3916 interrupt flags. Use this
     *  to wait for max. \a tmo milliseconds for the \b first interrupt indicated
     *  with mask \a mask to occur.
     *
     *  \param[in] mask : mask indicating the interrupts to wait for.
     *  \param[in] tmo : time in milliseconds until timeout occurs. If set to 0
     *                   the functions waits forever.
     *
     *  \return : 0 if timeout occurred otherwise a mask indicating the cleared
     *              interrupts.
     *
     *****************************************************************************
     */
    uint32_t st25r3916WaitForInterruptsTimed(uint32_t mask, uint16_t tmo);

    /*!
     *****************************************************************************
     *  \brief  Get status for the given interrupt
     *
     *  This function is used to check whether the interrupt given by \a mask
     *  has occurred. If yes the interrupt gets cleared. This function returns
     *  only status bits which are inside \a mask.
     *
     *  \param[in] mask : mask indicating the interrupt to check for.
     *
     *  \return the mask of the interrupts occurred
     *
     *****************************************************************************
     */
    uint32_t st25r3916GetInterrupt(uint32_t mask);

    /*!
     *****************************************************************************
     *  \brief  Init the 3916 interrupt
     *
     *  This function is used to check whether the interrupt given by \a mask
     *  has occurred.
     *
     *****************************************************************************
     */
    void st25r3916InitInterrupts(void);

    /*!
     *****************************************************************************
     *  \brief  Modifies the Interrupt
     *
     *  This function modifies the interrupt
     *
     *  \param[in] clr_mask : bit mask to be cleared on the interrupt mask
     *  \param[in] set_mask : bit mask to be set on the interrupt mask
     *****************************************************************************
     */
    void st25r3916ModifyInterrupts(uint32_t clr_mask, uint32_t set_mask);

    /*!
     *****************************************************************************
     *  \brief Checks received interrupts
     *
     *  Checks received interrupts and saves the result into global params
     *****************************************************************************
     */
    void st25r3916CheckForReceivedInterrupts(void);

    /*!
     *****************************************************************************
     *  \brief  Enable a given ST25R3916 Interrupt source
     *
     *  This function enables all interrupts given by \a mask,
     *  ST25R3916_IRQ_MASK_ALL enables all interrupts.
     *
     *  \param[in] mask: mask indicating the interrupts to be enabled
     *
     *****************************************************************************
     */
    void st25r3916EnableInterrupts(uint32_t mask);

    /*!
     *****************************************************************************
     *  \brief  Disable one or more a given ST25R3916 Interrupt sources
     *
     *  This function disables all interrupts given by \a mask. 0xff disables all.
     *
     *  \param[in] mask: mask indicating the interrupts to be disabled.
     *
     *****************************************************************************
     */
    void st25r3916DisableInterrupts(uint32_t mask);

    /*!
     *****************************************************************************
     *  \brief  Clear all ST25R3916 irq flags
     *
     *****************************************************************************
     */
    void st25r3916ClearInterrupts(void);

    /*!
     *****************************************************************************
     *  \brief  Clears and then enables the given ST25R3916 Interrupt sources
     *
     *  \param[in] mask: mask indicating the interrupts to be cleared and enabled
     *****************************************************************************
     */
    void st25r3916ClearAndEnableInterrupts(uint32_t mask);

    /*!
     *****************************************************************************
     *  \brief  Sets IRQ callback for the ST25R3916 interrupt
     *
     *****************************************************************************
     */
    void st25r3916IRQCallbackSet(void (*cb)(void));

    /*!
     *****************************************************************************
     *  \brief  Sets IRQ callback for the ST25R3916 interrupt
     *
     *****************************************************************************
     */
    void st25r3916IRQCallbackRestore(void);


    /*
    ******************************************************************************
    * RFAL ST25R3916 TIMER FUNCTION PROTOTYPES
    ******************************************************************************
    */


    /*!
     *****************************************************************************
     * \brief  Calculate Timer
     *
     * This method calculates when the timer will be expired given the amount
     * time in milliseconds /a tOut.
     * Once the timer has been calculated it will then be used to check when
     * it expires.
     *
     * \see timersIsExpired
     *
     * \param[in]  time : time/duration in Milliseconds for the timer
     *
     * \return u32 : The new timer calculated based on the given time
     *****************************************************************************
     */
    uint32_t timerCalculateTimer(uint16_t time);


    /*!
     *****************************************************************************
     * \brief  Checks if a Timer is Expired
     *
     * This method checks if a timer has already expired.
     * Based on the given timer previously calculated it checks if this timer
     * has already elapsed
     *
     * \see timersCalculateTimer
     *
     * \param[in]  timer : the timer to check
     *
     * \return true  : timer has already expired
     * \return false : timer is still running
     *****************************************************************************
     */
    bool timerIsExpired(uint32_t timer);


    /*!
     *****************************************************************************
     * \brief  Performs a Delay
     *
     * This method performs a delay for the given amount of time in Milliseconds
     *
     * \param[in]  time : time/duration in Milliseconds of the delay
     *
     *****************************************************************************
     */
    void timerDelay(uint16_t time);


    /*!
     *****************************************************************************
     * \brief  Stopwatch start
     *
     * This method initiates the stopwatch to later measure the time in ms
     *
     *****************************************************************************
     */
    void timerStopwatchStart(void);


    /*!
     *****************************************************************************
     * \brief  Stopwatch Measure
     *
     * This method returns the elapsed time in ms since the stopwatch was initiated
     *
     * \return The time in ms since the stopwatch was started
     *****************************************************************************
     */
    uint32_t timerStopwatchMeasure(void);

    /*
    ******************************************************************************
    * RFAL ST25R3916 AAT FUNCTION PROTOTYPES
    ******************************************************************************
    */

    /*!
     *****************************************************************************
     *  \brief  Perform antenna tuning
     *
     *  This function starts an antenna tuning procedure by modifying the serial
     *  and parallel capacitors of the antenna matching circuit via the AAT_A
     *  and AAT_B registers.
     *
     *  \param[in] tuningParams : Input parameters for the tuning algorithm. If NULL
     *                            default values will be used.
     *  \param[out] tuningStatus : Result information of performed tuning. If NULL
     *                             no further information is returned, only registers
     *                             ST25R3916 (AAT_A,B) will be adapted.
     *
     *  \return ERR_IO    : Error during communication.
     *  \return ERR_PARAM : Invalid input parameters
     *  \return ERR_NONE  : No error.
     *
     *****************************************************************************
     */
    ReturnCode st25r3916AatTune(const struct st25r3916AatTuneParams *tuningParams, struct st25r3916AatTuneResult *tuningStatus);

  protected:

    void rfalTransceiveTx(void);
    void rfalTransceiveRx(void);
    ReturnCode rfalTransceiveRunBlockingTx(void);
    void rfalPrepareTransceive(void);
    void rfalCleanupTransceive(void);
    void rfalErrorHandling(void);
    ReturnCode rfalRunTransceiveWorker(void);

#if RFAL_FEATURE_WAKEUP_MODE
    void rfalRunWakeUpModeWorker(void);
    uint16_t rfalWakeUpModeFilter(uint16_t curRef, uint16_t curVal, uint8_t weight);
#endif /* RFAL_FEATURE_WAKEUP_MODE */

    void rfalFIFOStatusUpdate(void);
    void rfalFIFOStatusClear(void);
    bool rfalFIFOStatusIsMissingPar(void);
    bool rfalFIFOStatusIsIncompleteByte(void);
    uint16_t rfalFIFOStatusGetNumBytes(void);
    uint8_t rfalFIFOGetNumIncompleteBits(void);
    rfalAnalogConfigNum rfalAnalogConfigSearch(rfalAnalogConfigId configId, uint16_t *configOffset);
    uint16_t rfalCrcUpdateCcitt(uint16_t crcSeed, uint8_t dataByte);
    ReturnCode aatHillClimb(const struct st25r3916AatTuneParams *tuningParams, struct st25r3916AatTuneResult *tuningStatus);
    int32_t aatGreedyDescent(uint32_t *f_min, const struct st25r3916AatTuneParams *tuningParams, struct st25r3916AatTuneResult *tuningStatus, int32_t previousDir);
    int32_t aatSteepestDescent(uint32_t *f_min, const struct st25r3916AatTuneParams *tuningParams, struct st25r3916AatTuneResult *tuningStatus, int32_t previousDir, int32_t previousDir2);
    ReturnCode aatMeasure(uint8_t serCap, uint8_t parCap, uint8_t *amplitude, uint8_t *phase, uint16_t *measureCnt);
    uint32_t aatCalcF(const struct st25r3916AatTuneParams *tuningParams, uint8_t amplitude, uint8_t phase);
    ReturnCode aatStepDacVals(const struct st25r3916AatTuneParams *tuningParams, uint8_t *a, uint8_t *b, int32_t dir);
    /*!
     *****************************************************************************
     *  \brief  ISR Service routine
     *
     *  This function modifies the interrupt
     *****************************************************************************
     */
    void  st25r3916Isr(void);

    MeowNfcBus *dev_i2c;
    SPIClass *dev_spi;
    int cs_pin;
    int int_pin;
    uint32_t spi_speed;

    rfal gRFAL;              /*!< RFAL module instance               */
    rfalAnalogConfigMgmt gRfalAnalogConfigMgmt;  /*!< Analog Configuration LUT management */
    rfalIso15693PhyConfig_t rfalIso15693PhyConfig; /*!< current phy configuration */
    uint32_t gST25R3916NRT_64fcs;
    volatile st25r3916Interrupt st25r3916interrupt; /*!< Instance of ST25R3916 interrupt */
    uint32_t timerStopwatchTick;
    bool i2c_enabled;
    volatile bool isr_pending;
    ST25R3916IrqHandler irq_handler;
};

#ifdef __cplusplus
extern "C" {
#endif
ReturnCode rfalIso15693PhyVCDCode1Of4(const uint8_t data, uint8_t *outbuffer, uint16_t maxOutBufLen, uint16_t *outBufLen);
ReturnCode rfalIso15693PhyVCDCode1Of256(const uint8_t data, uint8_t *outbuffer, uint16_t maxOutBufLen, uint16_t *outBufLen);
#ifdef __cplusplus
}
#endif

#endif /* RFAL_RFST25R3916_H */
