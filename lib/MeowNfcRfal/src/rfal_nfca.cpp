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
 *  \brief Provides several NFC-A convenience methods and definitions
 *
 *  It provides a Poller (ISO14443A PCD) interface and as well as
 *  some NFC-A Listener (ISO14443A PICC) helpers.
 *
 *  The definitions and helpers methods provided by this module are only
 *  up to ISO14443-3 layer
 *
 */

/*
 ******************************************************************************
 * INCLUDES
 ******************************************************************************
 */
#include "rfal_nfc.h"
#include "rfal_nfca.h"
#include "nfc_utils.h"

/*
 ******************************************************************************
 * ENABLE SWITCH
 ******************************************************************************
 */
/* Feature switch may be enabled or disabled by user at rfal_platform.h
 * Default configuration (ST25R dependent) also provided at rfal_default_config.h
 *
 *    RFAL_FEATURE_NFCA
 */

#if RFAL_FEATURE_NFCA

/*
 ******************************************************************************
 * GLOBAL DEFINES
 ******************************************************************************
 */

#define RFAL_NFCA_SLP_FWT           rfalConvMsTo1fc(1)    /*!< Check 1ms for any modulation  ISO14443-3 6.4.3   */
#define RFAL_NFCA_SLP_CMD           0x50U                 /*!< SLP cmd (byte1)    Digital 1.1  6.9.1 & Table 20 */
#define RFAL_NFCA_SLP_BYTE2         0x00U                 /*!< SLP byte2          Digital 1.1  6.9.1 & Table 20 */
#define RFAL_NFCA_SLP_CMD_POS       0U                    /*!< SLP cmd position   Digital 1.1  6.9.1 & Table 20 */
#define RFAL_NFCA_SLP_BYTE2_POS     1U                    /*!< SLP byte2 position Digital 1.1  6.9.1 & Table 20 */

#define RFAL_NFCA_SDD_CT            0x88U                 /*!< Cascade Tag value Digital 1.1 6.7.2              */
#define RFAL_NFCA_SDD_CT_LEN        1U                    /*!< Cascade Tag length                               */

#define RFAL_NFCA_SLP_REQ_LEN       2U                    /*!< SLP_REQ length                                   */

#define RFAL_NFCA_SEL_CMD_LEN       1U                    /*!< SEL_CMD length                                   */
#define RFAL_NFCA_SEL_PAR_LEN       1U                    /*!< SEL_PAR length                                   */
#define RFAL_NFCA_SEL_SELPAR        rfalNfcaSelPar(7U, 0U)/*!< SEL_PAR on Select is always with 4 data/nfcid    */
#define RFAL_NFCA_BCC_LEN           1U                    /*!< BCC length                                       */

#define RFAL_NFCA_SDD_REQ_LEN       (RFAL_NFCA_SEL_CMD_LEN + RFAL_NFCA_SEL_PAR_LEN)   /*!< SDD_REQ length       */
#define RFAL_NFCA_SDD_RES_LEN       (RFAL_NFCA_CASCADE_1_UID_LEN + RFAL_NFCA_BCC_LEN) /*!< SDD_RES length       */

#define RFAL_NFCA_T_RETRANS         5U                    /*!< t RETRANSMISSION [3, 33]ms   EMVCo 2.6  A.5      */
#define RFAL_NFCA_N_RETRANS         2U                    /*!< Number of retries            EMVCo 2.6  9.6.1.3  */


/*! SDD_REQ (Select) Cascade Levels  */
enum {
  RFAL_NFCA_SEL_CASCADE_L1 = 0,  /*!< SDD_REQ Cascade Level 1 */
  RFAL_NFCA_SEL_CASCADE_L2 = 1,  /*!< SDD_REQ Cascade Level 2 */
  RFAL_NFCA_SEL_CASCADE_L3 = 2   /*!< SDD_REQ Cascade Level 3 */
};

/*! SDD_REQ (Select) request Cascade Level command   Digital 1.1 Table 15 */
enum {
  RFAL_NFCA_CMD_SEL_CL1 = 0x93, /*!< SDD_REQ command Cascade Level 1 */
  RFAL_NFCA_CMD_SEL_CL2 = 0x95, /*!< SDD_REQ command Cascade Level 2 */
  RFAL_NFCA_CMD_SEL_CL3 = 0x97, /*!< SDD_REQ command Cascade Level 3 */
};

/*
******************************************************************************
* GLOBAL MACROS
******************************************************************************
*/
#define rfalNfcaSelPar( nBy, nbi )         (uint8_t)((((nBy)<<4U) & 0xF0U) | ((nbi)&0x0FU) )         /*!< Calculates SEL_PAR with the bytes/bits to be sent */
#define rfalNfcaCLn2SELCMD( cl )           (uint8_t)((uint8_t)(RFAL_NFCA_CMD_SEL_CL1) + (2U*(cl)))   /*!< Calculates SEL_CMD with the given cascade level   */
#define rfalNfcaNfcidLen2CL( len )         ((len) / 5U)                                              /*!< Calculates cascade level by the NFCID length      */

/*! Executes the given Tx method (f) and if a Timeout error is detected it retries (rt) times performing a delay of (dl) in between  */
#define rfalNfcaTxRetry( r, f, rt, dl )                            \
      {                                                      \
        uint8_t rts = (uint8_t)(rt);                       \
        do {                                   \
          (r)=(f);                                       \
          if (((rt)!=0U) && ((dl)!=0U)) {                \
            delay(dl);                         \
          }                                              \
        } while( ((rts--) != 0U) && ((r)==ERR_TIMEOUT) );  \
      }

/*
******************************************************************************
* GLOBAL TYPES
******************************************************************************
*/

/*! Technology Detection context */
typedef struct {
  rfalComplianceMode    compMode;        /*!< Compliance mode to be used      */
  ReturnCode            ret;             /*!< Outcome of presence check       */
} rfalNfcaTechDetParams;


/*! Collision Resolution states */
typedef enum {
  RFAL_NFCA_CR_IDLE,                      /*!< IDLE state                      */
  RFAL_NFCA_CR_CL,                        /*!< New Cascading Level state       */
  RFAL_NFCA_CR_SDD_TX,                    /*!< Perform anticollsion Tx state   */
  RFAL_NFCA_CR_SDD,                       /*!< Perform anticollsion state      */
  RFAL_NFCA_CR_SEL_TX,                    /*!< Perform CL Selection Tx state   */
  RFAL_NFCA_CR_SEL,                       /*!< Perform CL Selection state      */
  RFAL_NFCA_CR_DONE                       /*!< Collision Resolution done state */
} rfalNfcaColResState;


/*! Full Collision Resolution states */
typedef enum {
  RFAL_NFCA_CR_FULL_START,                /*!< Start Full Collision Resolution state                   */
  RFAL_NFCA_CR_FULL_SLPCHECK,             /*!< Sleep and Check for restart state                       */
  RFAL_NFCA_CR_FULL_RESTART               /*!< Restart Full Collision Resolution state                 */
} rfalNfcaFColResState;


/*! Collision Resolution context */
typedef struct {
  uint8_t               devLimit;         /*!< Device limit to be used                                 */
  rfalComplianceMode    compMode;         /*!< Compliance mode to be used                              */
  rfalNfcaListenDevice *nfcaDevList;      /*!< Location of the device list                             */
  uint8_t              *devCnt;           /*!< Location of the device counter                          */
  bool                  collPending;      /*!< Collision pending flag                                  */

  bool                 *collPend;         /*!< Location of collision pending flag (Single CR)          */
  rfalNfcaSelReq        selReq;           /*!< SelReqused during anticollision (Single CR)             */
  rfalNfcaSelRes       *selRes;           /*!< Location to place of the SEL_RES(SAK) (Single CR)       */
  uint8_t              *nfcId1;           /*!< Location to place the NFCID1 (Single CR)                */
  uint8_t              *nfcId1Len;        /*!< Location to place the NFCID1 length (Single CR)         */
  uint8_t               cascadeLv;        /*!< Current Cascading Level (Single CR)                     */
  rfalNfcaColResState   state;            /*!< Single Collision Resolution state (Single CR)           */
  rfalNfcaFColResState  fState;           /*!< Full Collision Resolution state (Full CR)               */
  uint8_t               bytesTxRx;        /*!< TxRx bytes used during anticollision loop (Single CR)   */
  uint8_t               bitsTxRx;         /*!< TxRx bits used during anticollision loop (Single CR)    */
  uint16_t              rxLen;            /*!< Local reception length                                  */
  uint32_t              tmrFDT;           /*!< FDT timer used between SED_REQs  (Single CR)            */
  uint8_t               retries;          /*!< Retries to be performed upon a timeout error (Single CR)*/
  uint8_t               backtrackCnt;     /*!< Backtrack retries (Single CR)                           */
  bool                  doBacktrack;      /*!< Backtrack flag (Single CR)                              */
} rfalNfcaColResParams;


/*! Collision Resolution context */
typedef struct {
  uint8_t               cascadeLv;        /*!< Current Cascading Level                                 */
  uint8_t               fCascadeLv;       /*!< Final Cascading Level                                   */
  rfalNfcaSelRes       *selRes;           /*!< Location to place of the SEL_RES(SAK)                   */
  uint16_t              rxLen;            /*!< Local reception length                                  */
  const uint8_t        *nfcid1;           /*!< Location of the NFCID to be selected                    */
  uint8_t               nfcidOffset;      /*!< Selected NFCID offset                                   */
  bool                  isRx;             /*!< Selection is in reception state                         */
} rfalNfcaSelParams;

/*! SLP_REQ (HLTA) format   Digital 1.1  6.9.1 & Table 20 */
typedef struct {
  uint8_t      frame[RFAL_NFCA_SLP_REQ_LEN];  /*!< SLP:  0x50 0x00  */
} rfalNfcaSlpReq;

/*! RFAL NFC-A instance */
typedef struct {
  rfalNfcaTechDetParams DT;               /*!< Technology Detection context                            */
  rfalNfcaColResParams  CR;               /*!< Collision Resolution context                            */
  rfalNfcaSelParams     SEL;              /*!< Selection|Activation context                            */

  rfalNfcaSlpReq        slpReq;           /*!< SLP_REx buffer                                          */
} rfalNfca;

// timerPollTimeoutValue is necessary after timerCalculateTimeout so that system will wake up upon timer timeout.
#define nfcaTimerStart( timer, time_ms ) (timer) = timerCalculateTimer((uint16_t)(time_ms))            /*!< Configures and starts the RTOX timer            */
#define nfcaTimerisExpired( timer )      timerIsExpired( timer )                               /*!< Checks RTOX timer has expired                   */

/*
******************************************************************************
* LOCAL VARIABLES
******************************************************************************
*/
rfalNfca gNfca;  /*!< RFAL NFC-A instance  */

/*
 ******************************************************************************
 * LOCAL FUNCTIONS
 ******************************************************************************
 */

uint8_t RfalNfcClass::rfalNfcaCalculateBcc(const uint8_t *buf, uint8_t bufLen)
{
  uint8_t i;
  uint8_t BCC;

  BCC = 0;

  /* BCC is XOR over first 4 bytes of the SDD_RES  Digital 1.1 6.7.2 */
  for (i = 0; i < bufLen; i++) {
    BCC ^= buf[i];
  }

  return BCC;
}

/*******************************************************************************/
ReturnCode RfalNfcClass::rfalNfcaPollerStartSingleCollisionResolution(uint8_t devLimit, bool *collPending, rfalNfcaSelRes *selRes, uint8_t *nfcId1, uint8_t *nfcId1Len)
{
  /* Check parameters */
  if ((collPending == NULL) || (selRes == NULL) || (nfcId1 == NULL) || (nfcId1Len == NULL)) {
    return ERR_PARAM;
  }

  /* Initialize output parameters */
  *collPending = false;  /* Activity 1.1  9.3.4.6 */
  *nfcId1Len   = 0;
  ST_MEMSET(nfcId1, 0x00, RFAL_NFCA_CASCADE_3_UID_LEN);


  /* Save parameters */
  gNfca.CR.devLimit    = devLimit;
  gNfca.CR.collPend    = collPending;
  gNfca.CR.selRes      = selRes;
  gNfca.CR.nfcId1      = nfcId1;
  gNfca.CR.nfcId1Len   = nfcId1Len;

  gNfca.CR.tmrFDT      = RFAL_TIMING_NONE;
  gNfca.CR.retries     = RFAL_NFCA_N_RETRANS;
  gNfca.CR.cascadeLv   = (uint8_t)RFAL_NFCA_SEL_CASCADE_L1;
  gNfca.CR.state       = RFAL_NFCA_CR_CL;

  gNfca.CR.doBacktrack  = false;
  gNfca.CR.backtrackCnt = 3U;

  return ERR_NONE;
}

/*******************************************************************************/
ReturnCode RfalNfcClass::rfalNfcaPollerGetSingleCollisionResolutionStatus(void)
{
  ReturnCode ret;
  uint8_t    collBit = 1U;  /* standards mandate or recommend collision bit to be set to One. */


  /* Check if FDT timer is still running */
  if (gNfca.CR.tmrFDT != RFAL_TIMING_NONE) {
    if ((!nfcaTimerisExpired(gNfca.CR.tmrFDT))) {
      return ERR_BUSY;
    }
  }

  /*******************************************************************************/
  /* Go through all Cascade Levels     Activity 1.1  9.3.4 */
  if (gNfca.CR.cascadeLv > (uint8_t)RFAL_NFCA_SEL_CASCADE_L3) {
    return ERR_INTERNAL;
  }

  switch (gNfca.CR.state) {
    /*******************************************************************************/
    case RFAL_NFCA_CR_CL:

      /* Initialize the SDD_REQ to send for the new cascade level */
      ST_MEMSET((uint8_t *)&gNfca.CR.selReq, 0x00, sizeof(rfalNfcaSelReq));

      gNfca.CR.bytesTxRx = RFAL_NFCA_SDD_REQ_LEN;
      gNfca.CR.bitsTxRx  = 0U;
      gNfca.CR.state     = RFAL_NFCA_CR_SDD_TX;

    /* fall through */

    /*******************************************************************************/
    case RFAL_NFCA_CR_SDD_TX:   /*  PRQA S 2003 # MISRA 16.3 - Intentional fall through */

      /* Calculate SEL_CMD and SEL_PAR with the bytes/bits to be sent */
      gNfca.CR.selReq.selCmd = rfalNfcaCLn2SELCMD(gNfca.CR.cascadeLv);
      gNfca.CR.selReq.selPar = rfalNfcaSelPar(gNfca.CR.bytesTxRx, gNfca.CR.bitsTxRx);

      /* Send SDD_REQ (Anticollision frame) */
      rfalRfDev->rfalISO14443AStartTransceiveAnticollisionFrame((uint8_t *)&gNfca.CR.selReq, &gNfca.CR.bytesTxRx, &gNfca.CR.bitsTxRx, &gNfca.CR.rxLen, RFAL_NFCA_FDTMIN);

      gNfca.CR.state = RFAL_NFCA_CR_SDD;
      break;


    /*******************************************************************************/
    case RFAL_NFCA_CR_SDD:

      EXIT_ON_BUSY(ret, rfalRfDev->rfalISO14443AGetTransceiveAnticollisionFrameStatus());

      /* Retry upon timeout  EMVCo 2.6  9.6.1.3 */
      if ((ret == ERR_TIMEOUT) && (gNfca.CR.devLimit == 0U) && (gNfca.CR.retries != 0U)) {
        gNfca.CR.retries--;
        nfcaTimerStart(gNfca.CR.tmrFDT, RFAL_NFCA_T_RETRANS);

        gNfca.CR.state = RFAL_NFCA_CR_SDD_TX;
        break;
      }

      /* Convert rxLen into bytes */
      gNfca.CR.rxLen = rfalConvBitsToBytes(gNfca.CR.rxLen);


      if ((ret == ERR_TIMEOUT)
          && (gNfca.CR.backtrackCnt != 0U) && (!gNfca.CR.doBacktrack)
          && (!((RFAL_NFCA_SDD_REQ_LEN == gNfca.CR.bytesTxRx) && (0U == gNfca.CR.bitsTxRx)))) {
        /* In multiple card scenarios it may always happen that some
          * collisions of a weaker tag go unnoticed. If then a later
          * collision is recognized and the strong tag has a 0 at the
          * collision position then no tag will respond. Catch this
          * corner case and then try with the bit being sent as zero. */
        rfalNfcaSensRes sensRes;
        ret = ERR_RF_COLLISION;
        rfalNfcaPollerCheckPresence(RFAL_14443A_SHORTFRAME_CMD_REQA, &sensRes);
        /* Algorithm below does a post-increment, decrement to go back to current position */
        if (0U == gNfca.CR.bitsTxRx) {
          gNfca.CR.bitsTxRx = 7;
          gNfca.CR.bytesTxRx--;
        } else {
          gNfca.CR.bitsTxRx--;
        }
        collBit = (uint8_t)(((uint8_t *)&gNfca.CR.selReq)[gNfca.CR.bytesTxRx] & (1U << gNfca.CR.bitsTxRx));
        collBit = (uint8_t)((0U == collBit) ? 1U : 0U);                          /* Invert the collision bit */
        gNfca.CR.doBacktrack = true;
        gNfca.CR.backtrackCnt--;
      } else {
        gNfca.CR.doBacktrack = false;
      }

      if (ret == ERR_RF_COLLISION) {
        /* Check received length */
        if ((gNfca.CR.bytesTxRx + ((gNfca.CR.bitsTxRx != 0U) ? 1U : 0U)) > (RFAL_NFCA_SDD_RES_LEN + RFAL_NFCA_SDD_REQ_LEN)) {
          return ERR_PROTO;
        }

        /* Collision in BCC: Anticollide only UID part */
        if (((gNfca.CR.bytesTxRx + ((gNfca.CR.bitsTxRx != 0U) ? 1U : 0U)) > (RFAL_NFCA_CASCADE_1_UID_LEN + RFAL_NFCA_SDD_REQ_LEN)) && (gNfca.CR.backtrackCnt != 0U)) {
          gNfca.CR.backtrackCnt--;
          gNfca.CR.bytesTxRx = (RFAL_NFCA_CASCADE_1_UID_LEN + RFAL_NFCA_SDD_REQ_LEN) - 1U;
          gNfca.CR.bitsTxRx = 7;
          collBit = (uint8_t)(((uint8_t *)&gNfca.CR.selReq)[gNfca.CR.bytesTxRx] & (1U << gNfca.CR.bitsTxRx));  /* Not a real collision, extract the actual bit for the subsequent code */
        }


        /* Activity 1.0 & 1.1  9.3.4.12: If CON_DEVICES_LIMIT has a value of 0, then
          * NFC Forum Device is configured to perform collision detection only       */
        if ((gNfca.CR.devLimit == 0U) && (!(*gNfca.CR.collPend))) {
          *gNfca.CR.collPend = true;
          return ERR_IGNORE;
        }

        *gNfca.CR.collPend = true;

        /* Set and select the collision bit, with the number of bytes/bits successfully TxRx */
        if (collBit != 0U) {
          ((uint8_t *)&gNfca.CR.selReq)[gNfca.CR.bytesTxRx] = (uint8_t)(((uint8_t *)&gNfca.CR.selReq)[gNfca.CR.bytesTxRx] | (1U << gNfca.CR.bitsTxRx)); /* MISRA 10.3 */
        } else {
          ((uint8_t *)&gNfca.CR.selReq)[gNfca.CR.bytesTxRx] = (uint8_t)(((uint8_t *)&gNfca.CR.selReq)[gNfca.CR.bytesTxRx] & ~(1U << gNfca.CR.bitsTxRx)); /* MISRA 10.3 */
        }

        gNfca.CR.bitsTxRx++;

        /* Check if number of bits form a byte */
        if (gNfca.CR.bitsTxRx == RFAL_BITS_IN_BYTE) {
          gNfca.CR.bitsTxRx = 0;
          gNfca.CR.bytesTxRx++;
        }

        gNfca.CR.state = RFAL_NFCA_CR_SDD_TX;
        break;
      }

      /*******************************************************************************/
      /* Check if Collision loop has failed */
      if (ret != ERR_NONE) {
        return ret;
      }


      /* If collisions are to be reported check whether the response is complete */
      if ((gNfca.CR.devLimit == 0U) && (gNfca.CR.rxLen != sizeof(rfalNfcaSddRes))) {
        return ERR_PROTO;
      }

      /* Check if the received BCC match */
      if (gNfca.CR.selReq.bcc != rfalNfcaCalculateBcc(gNfca.CR.selReq.nfcid1, RFAL_NFCA_CASCADE_1_UID_LEN)) {
        return ERR_PROTO;
      }

      /*******************************************************************************/
      /* Anticollision OK, Select this Cascade Level */
      gNfca.CR.selReq.selPar = RFAL_NFCA_SEL_SELPAR;

      gNfca.CR.retries = RFAL_NFCA_N_RETRANS;
      gNfca.CR.state   = RFAL_NFCA_CR_SEL_TX;
      break;

    /*******************************************************************************/
    case RFAL_NFCA_CR_SEL_TX:

      /* Send SEL_REQ (Select command) - Retry upon timeout  EMVCo 2.6  9.6.1.3 */
      rfalRfDev->rfalTransceiveBlockingTx((uint8_t *)&gNfca.CR.selReq, sizeof(rfalNfcaSelReq), (uint8_t *)gNfca.CR.selRes, sizeof(rfalNfcaSelRes), &gNfca.CR.rxLen, RFAL_TXRX_FLAGS_DEFAULT, RFAL_NFCA_FDTMIN);
      gNfca.CR.state   = RFAL_NFCA_CR_SEL;
      break;

    /*******************************************************************************/
    case RFAL_NFCA_CR_SEL:

      EXIT_ON_BUSY(ret, rfalRfDev->rfalGetTransceiveStatus());

      /* Retry upon timeout  EMVCo 2.6  9.6.1.3 */
      if ((ret == ERR_TIMEOUT) && (gNfca.CR.devLimit == 0U) && (gNfca.CR.retries != 0U)) {
        gNfca.CR.retries--;
        nfcaTimerStart(gNfca.CR.tmrFDT, RFAL_NFCA_T_RETRANS);

        gNfca.CR.state = RFAL_NFCA_CR_SEL_TX;
        break;
      }

      if (ret != ERR_NONE) {
        return ret;
      }

      gNfca.CR.rxLen = rfalConvBitsToBytes(gNfca.CR.rxLen);

      /* Ensure proper response length */
      if (gNfca.CR.rxLen != sizeof(rfalNfcaSelRes)) {
        return ERR_PROTO;
      }

      /*******************************************************************************/
      /* Check cascade byte, if cascade tag then go next cascade level */
      if (*gNfca.CR.selReq.nfcid1 == RFAL_NFCA_SDD_CT) {
        /* Cascade Tag present, store nfcid1 bytes (excluding cascade tag) and continue for next CL */
        ST_MEMCPY(&gNfca.CR.nfcId1[*gNfca.CR.nfcId1Len], &((uint8_t *)&gNfca.CR.selReq.nfcid1)[RFAL_NFCA_SDD_CT_LEN], (RFAL_NFCA_CASCADE_1_UID_LEN - RFAL_NFCA_SDD_CT_LEN));
        *gNfca.CR.nfcId1Len += (RFAL_NFCA_CASCADE_1_UID_LEN - RFAL_NFCA_SDD_CT_LEN);

        /* Go to next cascade level */
        gNfca.CR.state = RFAL_NFCA_CR_CL;
        gNfca.CR.cascadeLv++;
      } else {
        /* UID Selection complete, Stop Cascade Level loop */
        ST_MEMCPY(&gNfca.CR.nfcId1[*gNfca.CR.nfcId1Len], (uint8_t *)&gNfca.CR.selReq.nfcid1, RFAL_NFCA_CASCADE_1_UID_LEN);
        *gNfca.CR.nfcId1Len += RFAL_NFCA_CASCADE_1_UID_LEN;

        gNfca.CR.state = RFAL_NFCA_CR_DONE;
        break;                             /* Only flag operation complete on the next execution */
      }
      break;

    /*******************************************************************************/
    case RFAL_NFCA_CR_DONE:
      return ERR_NONE;

    /*******************************************************************************/
    default:
      return ERR_WRONG_STATE;
  }
  return ERR_BUSY;
}

/*
******************************************************************************
* GLOBAL FUNCTIONS
******************************************************************************
*/

/*******************************************************************************/
ReturnCode RfalNfcClass::rfalNfcaPollerInitialize(void)
{
  ReturnCode ret;

  EXIT_ON_ERR(ret, rfalRfDev->rfalSetMode(RFAL_MODE_POLL_NFCA, RFAL_BR_106, RFAL_BR_106));
  rfalRfDev->rfalSetErrorHandling(ERRORHANDLING_NONE);

  rfalRfDev->rfalSetGT(RFAL_GT_NFCA);
  rfalRfDev->rfalSetFDTListen(RFAL_FDT_LISTEN_NFCA_POLLER);
  rfalRfDev->rfalSetFDTPoll(RFAL_FDT_POLL_NFCA_POLLER);

  return ERR_NONE;
}


/*******************************************************************************/
ReturnCode RfalNfcClass::rfalNfcaPollerCheckPresence(rfal14443AShortFrameCmd cmd, rfalNfcaSensRes *sensRes)
{
  ReturnCode ret;
  uint16_t   rcvLen;

  /* Digital 1.1 6.10.1.3  For Commands ALL_REQ, SENS_REQ, SDD_REQ, and SEL_REQ, the NFC Forum Device      *
   *              MUST treat receipt of a Listen Frame at a time after FDT(Listen, min) as a Timeour Error */

  ret = rfalRfDev->rfalISO14443ATransceiveShortFrame(cmd, (uint8_t *)sensRes, (uint8_t)rfalConvBytesToBits(sizeof(rfalNfcaSensRes)), &rcvLen, RFAL_NFCA_FDTMIN);
  if ((ret == ERR_RF_COLLISION) || (ret == ERR_CRC)  || (ret == ERR_NOMEM) || (ret == ERR_FRAMING) || (ret == ERR_PAR) || (ret == ERR_INCOMPLETE_BYTE)) {
    ret = ERR_NONE;
  }

  return ret;
}


/*******************************************************************************/
ReturnCode RfalNfcClass::rfalNfcaPollerTechnologyDetection(rfalComplianceMode compMode, rfalNfcaSensRes *sensRes)
{
  ReturnCode ret;

  EXIT_ON_ERR(ret, rfalNfcaPollerStartTechnologyDetection(compMode, sensRes));
  rfalRunBlocking(ret, rfalNfcaPollerGetTechnologyDetectionStatus());

  return ret;
}

/*******************************************************************************/
ReturnCode RfalNfcClass::rfalNfcaPollerStartTechnologyDetection(rfalComplianceMode compMode, rfalNfcaSensRes *sensRes)
{
  ReturnCode ret;

  gNfca.DT.compMode = compMode;
  gNfca.DT.ret      = rfalNfcaPollerCheckPresence(((compMode == RFAL_COMPLIANCE_MODE_EMV) ? RFAL_14443A_SHORTFRAME_CMD_WUPA : RFAL_14443A_SHORTFRAME_CMD_REQA), sensRes);

  /* Send SLP_REQ as  Activity 1.1  9.2.3.6 and EMVCo 2.6  9.2.1.3 */
  if ((gNfca.DT.compMode != RFAL_COMPLIANCE_MODE_ISO) && (gNfca.DT.ret == ERR_NONE)) {
    EXIT_ON_ERR(ret, rfalNfcaPollerStartSleep());
  }

  return ERR_NONE;
}

/*******************************************************************************/
ReturnCode RfalNfcClass::rfalNfcaPollerGetTechnologyDetectionStatus(void)
{
  ReturnCode ret;

  /* If Sleep was sent, wait until its termination */
  if ((gNfca.DT.compMode != RFAL_COMPLIANCE_MODE_ISO) && (gNfca.DT.ret == ERR_NONE)) {
    EXIT_ON_BUSY(ret, rfalNfcaPollerGetSleepStatus());
  }

  return gNfca.DT.ret;
}

/*******************************************************************************/
ReturnCode RfalNfcClass::rfalNfcaPollerSingleCollisionResolution(uint8_t devLimit, bool *collPending, rfalNfcaSelRes *selRes, uint8_t *nfcId1, uint8_t *nfcId1Len)
{
  ReturnCode ret;

  EXIT_ON_ERR(ret, rfalNfcaPollerStartSingleCollisionResolution(devLimit, collPending, selRes, nfcId1, nfcId1Len));
  rfalRunBlocking(ret, rfalNfcaPollerGetSingleCollisionResolutionStatus());

  return ret;
}

/*******************************************************************************/
ReturnCode RfalNfcClass::rfalNfcaPollerStartFullCollisionResolution(rfalComplianceMode compMode, uint8_t devLimit, rfalNfcaListenDevice *nfcaDevList, uint8_t *devCnt)
{
  ReturnCode      ret;
  rfalNfcaSensRes sensRes;
  uint16_t        rcvLen;

  if ((nfcaDevList == NULL) || (devCnt == NULL)) {
    return ERR_PARAM;
  }

  *devCnt = 0;
  ret     = ERR_NONE;

  /*******************************************************************************/
  /* Send ALL_REQ before Anticollision if a Sleep was sent before  Activity 1.1  9.3.4.1 and EMVco 2.6  9.3.2.1 */
  if (compMode != RFAL_COMPLIANCE_MODE_ISO) {
    ret = rfalRfDev->rfalISO14443ATransceiveShortFrame(RFAL_14443A_SHORTFRAME_CMD_WUPA, (uint8_t *)&nfcaDevList->sensRes, (uint8_t)rfalConvBytesToBits(sizeof(rfalNfcaSensRes)), &rcvLen, RFAL_NFCA_FDTMIN);
    if (ret != ERR_NONE) {
      if ((compMode == RFAL_COMPLIANCE_MODE_EMV) || ((ret != ERR_RF_COLLISION) && (ret != ERR_CRC) && (ret != ERR_FRAMING) && (ret != ERR_PAR) && (ret != ERR_INCOMPLETE_BYTE))) {
        return ret;
      }
    }

    /* Check proper SENS_RES/ATQA size */
    if ((ret == ERR_NONE) && (rfalConvBytesToBits(sizeof(rfalNfcaSensRes)) != rcvLen)) {
      return ERR_PROTO;
    }
  }

  /*******************************************************************************/
  /* Store the SENS_RES from Technology Detection or from WUPA */
  sensRes = nfcaDevList->sensRes;

  if (devLimit > 0U) { /* MISRA 21.18 */
    ST_MEMSET(nfcaDevList, 0x00, (sizeof(rfalNfcaListenDevice) * devLimit));
  }

  /* Restore the prev SENS_RES, assuming that the SENS_RES received is from first device
    * When only one device is detected it's not woken up then we'll have no SENS_RES (ATQA) */
  nfcaDevList->sensRes = sensRes;

  /* Save parameters */
  gNfca.CR.devCnt      = devCnt;
  gNfca.CR.devLimit    = devLimit;
  gNfca.CR.nfcaDevList = nfcaDevList;
  gNfca.CR.compMode    = compMode;
  gNfca.CR.fState      = RFAL_NFCA_CR_FULL_START;


#if RFAL_FEATURE_T1T
  /*******************************************************************************/
  /* Only check for T1T if previous SENS_RES was received without a transmission  *
    * error. When collisions occur bits in the SENS_RES may look like a T1T        */
  /* If T1T Anticollision is not supported  Activity 1.1  9.3.4.3 */
  if (rfalNfcaIsSensResT1T(&nfcaDevList->sensRes) && (devLimit != 0U) && (ret == ERR_NONE) && (compMode != RFAL_COMPLIANCE_MODE_EMV)) {
    /* RID_REQ shall be performed              Activity 1.1  9.3.4.24 */
    rfalT1TPollerInitialize();
    EXIT_ON_ERR(ret, rfalT1TPollerRid(&nfcaDevList->ridRes));

    *devCnt = 1U;
    nfcaDevList->isSleep   = false;
    nfcaDevList->type      = RFAL_NFCA_T1T;
    nfcaDevList->nfcId1Len = RFAL_NFCA_CASCADE_1_UID_LEN;
    ST_MEMCPY(&nfcaDevList->nfcId1, &nfcaDevList->ridRes.uid, RFAL_NFCA_CASCADE_1_UID_LEN);

    return ERR_NONE;
  }
#endif /* RFAL_FEATURE_T1T */


  EXIT_ON_ERR(ret, rfalNfcaPollerStartSingleCollisionResolution(devLimit, &gNfca.CR.collPending, &nfcaDevList->selRes, (uint8_t *)&nfcaDevList->nfcId1, &nfcaDevList->nfcId1Len));

  gNfca.CR.fState = RFAL_NFCA_CR_FULL_START;
  return ERR_NONE;
}

/*******************************************************************************/
ReturnCode RfalNfcClass::rfalNfcaPollerGetFullCollisionResolutionStatus(void)
{
  ReturnCode ret;
  uint8_t    newDevType;

  if ((gNfca.CR.nfcaDevList == NULL) || (gNfca.CR.devCnt == NULL)) {
    return ERR_WRONG_STATE;
  }


  switch (gNfca.CR.fState) {
    /*******************************************************************************/
    case RFAL_NFCA_CR_FULL_START:

      /*******************************************************************************/
      /* Check whether a T1T has already been detected */
      if (rfalNfcaIsSensResT1T(&gNfca.CR.nfcaDevList->sensRes) && (gNfca.CR.nfcaDevList->type == RFAL_NFCA_T1T)) {
        /* T1T doesn't support Anticollision */
        return ERR_NONE;
      }

    /* fall through */

    /*******************************************************************************/
    case RFAL_NFCA_CR_FULL_RESTART:  /*  PRQA S 2003 # MISRA 16.3 - Intentional fall through */

      /*******************************************************************************/
      EXIT_ON_ERR(ret, rfalNfcaPollerGetSingleCollisionResolutionStatus());

      /* Assign Listen Device */
      newDevType = ((uint8_t)gNfca.CR.nfcaDevList[*gNfca.CR.devCnt].selRes.sak) & RFAL_NFCA_SEL_RES_CONF_MASK;  /* MISRA 10.8 */
      /* PRQA S 4342 1 # MISRA 10.5 - Guaranteed that no invalid enum values are created: see guard_eq_RFAL_NFCA_T2T, .... */
      gNfca.CR.nfcaDevList[*gNfca.CR.devCnt].type    = (rfalNfcaListenDeviceType) newDevType;
      gNfca.CR.nfcaDevList[*gNfca.CR.devCnt].isSleep = false;
      (*gNfca.CR.devCnt)++;


      /* If a collision was detected and device counter is lower than limit  Activity 1.1  9.3.4.21 */
      if ((*gNfca.CR.devCnt < gNfca.CR.devLimit) && (gNfca.CR.collPending)) {
        /* Put this device to Sleep  Activity 1.1  9.3.4.22 */
        EXIT_ON_ERR(ret, rfalNfcaPollerStartSleep());
        gNfca.CR.nfcaDevList[(*gNfca.CR.devCnt - 1U)].isSleep = true;

        gNfca.CR.fState = RFAL_NFCA_CR_FULL_SLPCHECK;
        return ERR_BUSY;
      } else {
        /* Exit loop */
        gNfca.CR.collPending = false;
      }
      break;


    /*******************************************************************************/
    case RFAL_NFCA_CR_FULL_SLPCHECK:

      EXIT_ON_BUSY(ret, rfalNfcaPollerGetSleepStatus());

      /* Send a new SENS_REQ to check for other cards  Activity 1.1  9.3.4.23 */
      ret = rfalNfcaPollerCheckPresence(RFAL_14443A_SHORTFRAME_CMD_REQA, &gNfca.CR.nfcaDevList[*gNfca.CR.devCnt].sensRes);
      if (ret == ERR_TIMEOUT) {
        /* No more devices found, exit */
        gNfca.CR.collPending = false;
      } else {
        /* Another device found, restart|continue loop */
        gNfca.CR.collPending = true;

        /*******************************************************************************/
        /* Check if collision resolution shall continue */
        if ((*gNfca.CR.devCnt < gNfca.CR.devLimit) && (gNfca.CR.collPending)) {
          EXIT_ON_ERR(ret, rfalNfcaPollerStartSingleCollisionResolution(gNfca.CR.devLimit,
                                                                        &gNfca.CR.collPending,
                                                                        &gNfca.CR.nfcaDevList[*gNfca.CR.devCnt].selRes,
                                                                        (uint8_t *)&gNfca.CR.nfcaDevList[*gNfca.CR.devCnt].nfcId1,
                                                                        &gNfca.CR.nfcaDevList[*gNfca.CR.devCnt].nfcId1Len));

          gNfca.CR.fState = RFAL_NFCA_CR_FULL_RESTART;
          return ERR_BUSY;
        }
      }
      break;

    /*******************************************************************************/
    default:
      return ERR_WRONG_STATE;
  }

  return ERR_NONE;
}


/*******************************************************************************/
ReturnCode RfalNfcClass::rfalNfcaPollerFullCollisionResolution(rfalComplianceMode compMode, uint8_t devLimit, rfalNfcaListenDevice *nfcaDevList, uint8_t *devCnt)
{
  ReturnCode ret;

  EXIT_ON_ERR(ret, rfalNfcaPollerStartFullCollisionResolution(compMode, devLimit, nfcaDevList, devCnt));
  rfalRunBlocking(ret, rfalNfcaPollerGetFullCollisionResolutionStatus());

  return ret;

}

/*******************************************************************************/
ReturnCode RfalNfcClass::rfalNfcaPollerSleepFullCollisionResolution(uint8_t devLimit, rfalNfcaListenDevice *nfcaDevList, uint8_t *devCnt)
{
  bool       firstRound;
  uint8_t    tmpDevCnt;
  ReturnCode ret;


  if ((nfcaDevList == NULL) || (devCnt == NULL)) {
    return ERR_PARAM;
  }

  /* Only use ALL_REQ (WUPA) on the first round */
  firstRound = true;
  *devCnt    = 0;


  /* Perform collision resolution until no new device is found */
  do {
    tmpDevCnt = 0;
    ret = rfalNfcaPollerFullCollisionResolution((firstRound ? RFAL_COMPLIANCE_MODE_NFC : RFAL_COMPLIANCE_MODE_ISO), (devLimit - *devCnt), &nfcaDevList[*devCnt], &tmpDevCnt);

    if ((ret == ERR_NONE) && (tmpDevCnt > 0U)) {
      *devCnt += tmpDevCnt;

      /* Check whether to search for more devices */
      if (*devCnt < devLimit) {
        /* Set last found device to sleep (all others are slept already) */
        rfalNfcaPollerSleep();
        nfcaDevList[((*devCnt) - 1U)].isSleep = true;

        /* Check if any other device is present */
        ret = rfalNfcaPollerCheckPresence(RFAL_14443A_SHORTFRAME_CMD_REQA, &nfcaDevList[*devCnt].sensRes);
        if (ret == ERR_NONE) {
          firstRound = false;
          continue;
        }
      }
    }
    break;
  } while (true);

  return ((*devCnt > 0U) ? ERR_NONE : ret);
}

/*******************************************************************************/
ReturnCode RfalNfcClass::rfalNfcaPollerSelect(const uint8_t *nfcid1, uint8_t nfcidLen, rfalNfcaSelRes *selRes)
{
  ReturnCode ret;

  EXIT_ON_ERR(ret, rfalNfcaPollerStartSelect(nfcid1, nfcidLen, selRes));
  rfalRunBlocking(ret, rfalNfcaPollerGetSelectStatus());

  return ret;
}

/*******************************************************************************/
ReturnCode RfalNfcClass::rfalNfcaPollerStartSelect(const uint8_t *nfcid1, uint8_t nfcidLen, rfalNfcaSelRes *selRes)
{
  if ((nfcid1 == NULL) || (nfcidLen > RFAL_NFCA_CASCADE_3_UID_LEN) || (selRes == NULL)) {
    return ERR_PARAM;
  }


  /* Calculate Cascate Level */
  gNfca.SEL.fCascadeLv = rfalNfcaNfcidLen2CL(nfcidLen);
  gNfca.SEL.cascadeLv  = RFAL_NFCA_SEL_CASCADE_L1;

  gNfca.SEL.nfcidOffset  = 0;
  gNfca.SEL.isRx         = false;
  gNfca.SEL.selRes       = selRes;
  gNfca.SEL.nfcid1       = nfcid1;

  return ERR_NONE;
}


/*******************************************************************************/
ReturnCode RfalNfcClass::rfalNfcaPollerGetSelectStatus(void)
{
  ReturnCode     ret;
  rfalNfcaSelReq selReq;

  if ((!gNfca.SEL.isRx)) {
    /*******************************************************************************/
    /* Go through all Cascade Levels     Activity 1.1  9.4.4 */
    if (gNfca.SEL.cascadeLv <= gNfca.SEL.fCascadeLv) {
      /* Assign SEL_CMD according to the CLn and SEL_PAR*/
      selReq.selCmd = rfalNfcaCLn2SELCMD(gNfca.SEL.cascadeLv);
      selReq.selPar = RFAL_NFCA_SEL_SELPAR;

      /* Compute NFCID/Data on the SEL_REQ command   Digital 1.1  Table 18 */
      if (gNfca.SEL.fCascadeLv != gNfca.SEL.cascadeLv) {
        *selReq.nfcid1 = RFAL_NFCA_SDD_CT;
        ST_MEMCPY(&selReq.nfcid1[RFAL_NFCA_SDD_CT_LEN], &gNfca.SEL.nfcid1[gNfca.SEL.nfcidOffset], (RFAL_NFCA_CASCADE_1_UID_LEN - RFAL_NFCA_SDD_CT_LEN));
        gNfca.SEL.nfcidOffset += (RFAL_NFCA_CASCADE_1_UID_LEN - RFAL_NFCA_SDD_CT_LEN);
      } else {
        ST_MEMCPY(selReq.nfcid1, &gNfca.SEL.nfcid1[gNfca.SEL.nfcidOffset], RFAL_NFCA_CASCADE_1_UID_LEN);
      }

      /* Calculate nfcid's BCC */
      selReq.bcc = rfalNfcaCalculateBcc((uint8_t *)&selReq.nfcid1, sizeof(selReq.nfcid1));

      /*******************************************************************************/
      /* Send SEL_REQ  */
      EXIT_ON_ERR(ret, rfalRfDev->rfalTransceiveBlockingTx((uint8_t *)&selReq, sizeof(rfalNfcaSelReq), (uint8_t *)gNfca.SEL.selRes, sizeof(rfalNfcaSelRes), &gNfca.SEL.rxLen, RFAL_TXRX_FLAGS_DEFAULT, RFAL_NFCA_FDTMIN));

      /* Wait for Rx to conclude */
      gNfca.SEL.isRx = true;

      return ERR_BUSY;
    }
  } else {
    EXIT_ON_BUSY(ret, rfalRfDev->rfalGetTransceiveStatus());

    /* Ensure proper response length */
    if (rfalConvBitsToBytes(gNfca.SEL.rxLen) != sizeof(rfalNfcaSelRes)) {
      return ERR_PROTO;
    }

    /* Check if there are more level(s) to be selected */
    if (gNfca.SEL.cascadeLv < gNfca.SEL.fCascadeLv) {
      /* Advance to the next cascade level */
      gNfca.SEL.cascadeLv++;
      gNfca.SEL.isRx = false;

      return ERR_BUSY;
    }
  }

  /* REMARK: Could check if NFCID1 is complete */

  return ERR_NONE;
}

/*******************************************************************************/
ReturnCode RfalNfcClass::rfalNfcaPollerSleep(void)
{
  ReturnCode ret;

  EXIT_ON_ERR(ret, rfalNfcaPollerStartSleep());
  rfalRunBlocking(ret, rfalNfcaPollerGetSleepStatus());

  return ret;
}

/*******************************************************************************/
ReturnCode RfalNfcClass::rfalNfcaPollerStartSleep(void)
{
  rfalTransceiveContext ctx;

  gNfca.slpReq.frame[RFAL_NFCA_SLP_CMD_POS]   = RFAL_NFCA_SLP_CMD;
  gNfca.slpReq.frame[RFAL_NFCA_SLP_BYTE2_POS] = RFAL_NFCA_SLP_BYTE2;

  rfalCreateByteFlagsTxRxContext(ctx, (uint8_t *)&gNfca.slpReq, sizeof(rfalNfcaSlpReq), (uint8_t *)&gNfca.slpReq, sizeof(gNfca.slpReq), NULL, RFAL_TXRX_FLAGS_DEFAULT, RFAL_NFCA_SLP_FWT);
  return rfalRfDev->rfalStartTransceive(&ctx);
}

/*******************************************************************************/
ReturnCode RfalNfcClass::rfalNfcaPollerGetSleepStatus(void)
{
  ReturnCode ret;

  /* ISO14443-3 6.4.3  HLTA - If PICC responds with any modulation during 1 ms this response shall be interpreted as not acknowledge
     Digital 2.0  6.9.2.1 & EMVCo 3.0  5.6.2.1 - consider the HLTA command always acknowledged
     No check to be compliant with NFC and EMVCo, and to improve interoperability (Kovio RFID Tag)
  */
  EXIT_ON_BUSY(ret, rfalRfDev->rfalGetTransceiveStatus());

  return ERR_NONE;
}

/*******************************************************************************/
bool RfalNfcClass::rfalNfcaListenerIsSleepReq(const uint8_t *buf, uint16_t bufLen)
{
  /* Check if length and payload match */
  if ((bufLen != sizeof(rfalNfcaSlpReq)) || (buf[RFAL_NFCA_SLP_CMD_POS] != RFAL_NFCA_SLP_CMD) || (buf[RFAL_NFCA_SLP_BYTE2_POS] != RFAL_NFCA_SLP_BYTE2)) {
    return false;
  }

  return true;
}

/* If the guards here don't compile then the code above cannot work anymore. */
extern uint8_t guard_eq_RFAL_NFCA_T2T[((RFAL_NFCA_SEL_RES_CONF_MASK & (uint8_t)RFAL_NFCA_T2T) == (uint8_t)RFAL_NFCA_T2T) ? 1 : (-1)];
extern uint8_t guard_eq_RFAL_NFCA_T4T[((RFAL_NFCA_SEL_RES_CONF_MASK & (uint8_t)RFAL_NFCA_T4T) == (uint8_t)RFAL_NFCA_T4T) ? 1 : (-1)];
extern uint8_t guard_eq_RFAL_NFCA_NFCDEP[((RFAL_NFCA_SEL_RES_CONF_MASK & (uint8_t)RFAL_NFCA_NFCDEP) == (uint8_t)RFAL_NFCA_NFCDEP) ? 1 : (-1)];
extern uint8_t guard_eq_RFAL_NFCA_T4T_NFCDEP[((RFAL_NFCA_SEL_RES_CONF_MASK & (uint8_t)RFAL_NFCA_T4T_NFCDEP) == (uint8_t)RFAL_NFCA_T4T_NFCDEP) ? 1 : (-1)];

#endif /* RFAL_FEATURE_NFCA */
