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
 */

/*
 ******************************************************************************
 * INCLUDES
 ******************************************************************************
 */
#include "rfal_nfc.h"
#include "rfal_nfcb.h"
#include "nfc_utils.h"

/*
 ******************************************************************************
 * ENABLE SWITCH
 ******************************************************************************
 */

/* Feature switch may be enabled or disabled by user at rfal_platform.h
 * Default configuration (ST25R dependent) also provided at rfal_default_config.h
 *
 *    RFAL_FEATURE_NFCB
 */

#if RFAL_FEATURE_NFCB

/*
 ******************************************************************************
 * GLOBAL DEFINES
 ******************************************************************************
 */

#define RFAL_NFCB_SENSB_REQ_EXT_SENSB_RES_SUPPORTED  0x10U /*!< Bit mask for Extended SensB Response support in SENSB_REQ */
#define RFAL_NFCB_SENSB_RES_PROT_TYPE_RFU            0x08U /*!< Bit mask for Protocol Type RFU in SENSB_RES               */
#define RFAL_NFCB_SLOT_MARKER_SC_SHIFT               4U    /*!< Slot Code position on SLOT_MARKER APn                     */

#define RFAL_NFCB_SLOTMARKER_SLOTCODE_MIN            1U    /*!< SLOT_MARKER Slot Code minimum   Digital 1.1  Table 37     */
#define RFAL_NFCB_SLOTMARKER_SLOTCODE_MAX            16U   /*!< SLOT_MARKER Slot Code maximum   Digital 1.1  Table 37     */

#define RFAL_NFCB_ACTIVATION_FWT                    (RFAL_NFCB_FWTSENSB + RFAL_NFCB_DTPOLL_20)  /*!< FWT(SENSB) + dTbPoll  Digital 2.0  7.9.1.3  */

/*! Advanced and Extended bit mask in Parameter of SENSB_REQ */
#define RFAL_NFCB_SENSB_REQ_PARAM                   (RFAL_NFCB_SENSB_REQ_ADV_FEATURE | RFAL_NFCB_SENSB_REQ_EXT_SENSB_RES_SUPPORTED)


/*! NFC-B commands definition */
enum {
  RFAL_NFCB_CMD_SENSB_REQ = 0x05,   /*!< SENSB_REQ (REQB) & SLOT_MARKER  Digital 1.1 Table 24 */
  RFAL_NFCB_CMD_SENSB_RES = 0x50,   /*!< SENSB_RES (ATQB) & SLOT_MARKER  Digital 1.1 Table 27 */
  RFAL_NFCB_CMD_SLPB_REQ  = 0x50,   /*!< SLPB_REQ (HLTB command)  Digital 1.1 Table 38        */
  RFAL_NFCB_CMD_SLPB_RES  = 0x00    /*!< SLPB_RES (HLTB Answer)   Digital 1.1 Table 39        */
};







/*
 ******************************************************************************
 * GLOBAL MACROS
 ******************************************************************************
 */

#define rfalNfcbNI2NumberOfSlots( ni )  (uint8_t)(1U << (ni))  /*!< Converts the Number of slots Identifier to slot number */

// timerPollTimeoutValue is necessary after timerCalculateTimeout so that system will wake up upon timer timeout.
#define nfcbTimerStart( timer, time_ms ) (timer) = timerCalculateTimer((uint16_t)(time_ms))            /*!< Configures and starts the RTOX timer            */
#define nfcbTimerisExpired( timer )      timerIsExpired( timer )                               /*!< Checks RTOX timer has expired                   */


/*
******************************************************************************
* GLOBAL TYPES
******************************************************************************
*/


/*
******************************************************************************
* LOCAL VARIABLES
******************************************************************************
*/


/*
******************************************************************************
* LOCAL FUNCTIONS
******************************************************************************
*/

/*******************************************************************************/
ReturnCode RfalNfcClass::rfalNfcbCheckSensbRes(const rfalNfcbSensbRes *sensbRes, uint8_t sensbResLen)
{
  /* Check response length */
  if (((sensbResLen != RFAL_NFCB_SENSB_RES_LEN) && (sensbResLen != RFAL_NFCB_SENSB_RES_EXT_LEN))) {
    return ERR_PROTO;
  }

  /* Check SENSB_RES and Protocol Type   Digital 1.1 7.6.2.19 */
  if (((sensbRes->protInfo.FsciProType & RFAL_NFCB_SENSB_RES_PROT_TYPE_RFU) != 0U) || (sensbRes->cmd != (uint8_t)RFAL_NFCB_CMD_SENSB_RES)) {
    return ERR_PROTO;
  }
  return ERR_NONE;
}

/*******************************************************************************/
/* This function is used internally during Collision Resolution.  Its          *
 * purpose is to block the state machine for minimmal time.                    *
 * Activity 2.1 does not enforce response checking or error handling.          */
ReturnCode RfalNfcClass::rfalNfcbPollerSleepTx(const uint8_t *nfcid0)
{
  ReturnCode      ret;
  rfalNfcbSlpbReq slpbReq;

  if (nfcid0 == NULL) {
    return ERR_PARAM;
  }

  /* Compute SLPB_REQ */
  slpbReq.cmd = RFAL_NFCB_CMD_SLPB_REQ;
  ST_MEMCPY(slpbReq.nfcid0, nfcid0, RFAL_NFCB_NFCID0_LEN);

  /* Send SLPB_REQ and ignore its response and FWT*/
  EXIT_ON_ERR(ret, rfalRfDev->rfalTransceiveBlockingTx((uint8_t *)&slpbReq, sizeof(rfalNfcbSlpbReq), NULL, 0, NULL, RFAL_TXRX_FLAGS_DEFAULT, RFAL_FDT_POLL_NFCB_POLLER));

  return ERR_NONE;
}

/*
******************************************************************************
* GLOBAL FUNCTIONS
******************************************************************************
*/

/*******************************************************************************/
ReturnCode RfalNfcClass::rfalNfcbPollerInitialize(void)
{
  ReturnCode ret;

  EXIT_ON_ERR(ret, rfalRfDev->rfalSetMode(RFAL_MODE_POLL_NFCB, RFAL_BR_106, RFAL_BR_106));
  rfalRfDev->rfalSetErrorHandling(ERRORHANDLING_NONE);

  rfalRfDev->rfalSetGT(RFAL_GT_NFCB);
  rfalRfDev->rfalSetFDTListen(RFAL_FDT_LISTEN_NFCB_POLLER);
  rfalRfDev->rfalSetFDTPoll(RFAL_FDT_POLL_NFCB_POLLER);

  gRfalNfcb.AFI    = RFAL_NFCB_AFI;
  gRfalNfcb.PARAM  = RFAL_NFCB_PARAM;

  return ERR_NONE;
}

/*******************************************************************************/
ReturnCode RfalNfcClass::rfalNfcbPollerInitializeWithParams(uint8_t AFI, uint8_t PARAM)
{
  ReturnCode ret;

  EXIT_ON_ERR(ret, rfalNfcbPollerInitialize());

  gRfalNfcb.AFI   = AFI;
  gRfalNfcb.PARAM = (PARAM & RFAL_NFCB_SENSB_REQ_PARAM);

  return ERR_NONE;
}


/*******************************************************************************/

ReturnCode RfalNfcClass::rfalNfcbPollerCheckPresence(rfalNfcbSensCmd cmd, rfalNfcbSlots slots, rfalNfcbSensbRes *sensbRes, uint8_t *sensbResLen)
{
  ReturnCode ret;

  EXIT_ON_ERR(ret, rfalNfcbPollerStartCheckPresence(cmd, slots, sensbRes, sensbResLen));
  rfalRunBlocking(ret, rfalNfcbPollerGetCheckPresenceStatus());

  return ret;
}
/*******************************************************************************/
ReturnCode RfalNfcClass::rfalNfcbPollerStartCheckPresence(rfalNfcbSensCmd cmd, rfalNfcbSlots slots, rfalNfcbSensbRes *sensbRes, uint8_t *sensbResLen)
{
  rfalNfcbSensbReq sensbReq;
  ReturnCode       ret;

  /* Check if the command requested and given the slot number are valid */
  if (((RFAL_NFCB_SENS_CMD_SENSB_REQ != cmd) && (RFAL_NFCB_SENS_CMD_ALLB_REQ != cmd)) ||
      (slots > RFAL_NFCB_SLOT_NUM_16) || (sensbRes == NULL) || (sensbResLen == NULL)) {
    return ERR_PARAM;
  }

  *sensbResLen = 0;
  ST_MEMSET(sensbRes, 0x00, sizeof(rfalNfcbSensbRes));

  /* Compute SENSB_REQ */
  sensbReq.cmd   = RFAL_NFCB_CMD_SENSB_REQ;
  sensbReq.AFI   = gRfalNfcb.AFI;
  sensbReq.PARAM = (((uint8_t)gRfalNfcb.PARAM & RFAL_NFCB_SENSB_REQ_PARAM) | (uint8_t)cmd | (uint8_t)slots);

  gRfalNfcb.DT.sensbRes    = sensbRes;
  gRfalNfcb.DT.sensbResLen = sensbResLen;

  /* Send SENSB_REQ */
  ret = rfalRfDev->rfalTransceiveBlockingTx((uint8_t *)&sensbReq, sizeof(rfalNfcbSensbReq), (uint8_t *)sensbRes, sizeof(rfalNfcbSensbRes), &gRfalNfcb.DT.rxLen, RFAL_TXRX_FLAGS_DEFAULT, RFAL_NFCB_FWTSENSB);
  return ret;
}

/*******************************************************************************/
ReturnCode RfalNfcClass::rfalNfcbPollerGetCheckPresenceStatus(void)
{
  ReturnCode ret;

  EXIT_ON_BUSY(ret, rfalRfDev->rfalGetTransceiveStatus());


  /* Convert bits to bytes (u8) */
  (*gRfalNfcb.DT.sensbResLen) = (uint8_t)rfalConvBitsToBytes(gRfalNfcb.DT.rxLen);

  /*  Check if a transmission error was detected */
  if ((ret == ERR_CRC) || (ret == ERR_FRAMING)) {
    /* Invalidate received frame as an error was detected (CollisionResolution checks if valid) */
    (*gRfalNfcb.DT.sensbResLen) = 0;
    return ERR_NONE;
  }

  if (ret == ERR_NONE) {
    return rfalNfcbCheckSensbRes(gRfalNfcb.DT.sensbRes, *gRfalNfcb.DT.sensbResLen);
  }

  return ret;
}

/*******************************************************************************/
ReturnCode RfalNfcClass::rfalNfcbPollerSleep(const uint8_t *nfcid0)
{
  uint16_t        rxLen;
  ReturnCode      ret;
  rfalNfcbSlpbReq slpbReq;
  rfalNfcbSlpbRes slpbRes;

  if (nfcid0 == NULL) {
    return ERR_PARAM;
  }

  /* Compute SLPB_REQ */
  slpbReq.cmd = RFAL_NFCB_CMD_SLPB_REQ;
  ST_MEMCPY(slpbReq.nfcid0, nfcid0, RFAL_NFCB_NFCID0_LEN);

  EXIT_ON_ERR(ret, rfalRfDev->rfalTransceiveBlockingTxRx((uint8_t *)&slpbReq, sizeof(rfalNfcbSlpbReq), (uint8_t *)&slpbRes, sizeof(rfalNfcbSlpbRes), &rxLen, RFAL_TXRX_FLAGS_DEFAULT, RFAL_NFCB_ACTIVATION_FWT));

  /* Check SLPB_RES */
  if ((rxLen != sizeof(rfalNfcbSlpbRes)) || (slpbRes.cmd != (uint8_t)RFAL_NFCB_CMD_SLPB_RES)) {
    return ERR_PROTO;
  }
  return ERR_NONE;
}


/*******************************************************************************/
ReturnCode RfalNfcClass::rfalNfcbPollerSlotMarker(uint8_t slotCode, rfalNfcbSensbRes *sensbRes, uint8_t *sensbResLen)
{
  ReturnCode ret;

  EXIT_ON_ERR(ret, rfalNfcbPollerStartSlotMarker(slotCode, sensbRes, sensbResLen));
  rfalRunBlocking(ret, rfalNfcbPollerGetSlotMarkerStatus());

  return ret;
}

/*******************************************************************************/
ReturnCode RfalNfcClass::rfalNfcbPollerStartSlotMarker(uint8_t slotCode, rfalNfcbSensbRes *sensbRes, uint8_t *sensbResLen)
{
  rfalNfcbSlotMarker slotMarker;

  /* Check parameters */
  if ((sensbRes == NULL) || (sensbResLen == NULL)    ||
      (slotCode < RFAL_NFCB_SLOTMARKER_SLOTCODE_MIN) ||
      (slotCode > RFAL_NFCB_SLOTMARKER_SLOTCODE_MAX)) {
    return ERR_PARAM;
  }
  /* Compose and send SLOT_MARKER with disabled AGC to detect collisions  */
  slotMarker.APn = ((slotCode << RFAL_NFCB_SLOT_MARKER_SC_SHIFT) | (uint8_t)RFAL_NFCB_CMD_SENSB_REQ);

  gRfalNfcb.DT.sensbRes    = sensbRes;
  gRfalNfcb.DT.sensbResLen = sensbResLen;

  return rfalRfDev->rfalTransceiveBlockingTx((uint8_t *)&slotMarker, sizeof(rfalNfcbSlotMarker), (uint8_t *)gRfalNfcb.DT.sensbRes, sizeof(rfalNfcbSensbRes), &gRfalNfcb.DT.rxLen, RFAL_TXRX_FLAGS_DEFAULT, RFAL_NFCB_FWTSENSB);
}

/*******************************************************************************/
ReturnCode RfalNfcClass::rfalNfcbPollerGetSlotMarkerStatus(void)
{
  ReturnCode ret;

  EXIT_ON_BUSY(ret, rfalRfDev->rfalGetTransceiveStatus());

  /* Convert bits to bytes (u8) */
  (*gRfalNfcb.DT.sensbResLen) = (uint8_t)rfalConvBitsToBytes(gRfalNfcb.DT.rxLen);

  /*  Check if a transmission error was detected */
  if ((ret == ERR_CRC) || (ret == ERR_FRAMING)) {
    return ERR_RF_COLLISION;
  }

  if (ret == ERR_NONE) {
    return rfalNfcbCheckSensbRes(gRfalNfcb.DT.sensbRes, *gRfalNfcb.DT.sensbResLen);
  }

  return ret;
}

/*******************************************************************************/
ReturnCode RfalNfcClass::rfalNfcbPollerTechnologyDetection(rfalComplianceMode compMode, rfalNfcbSensbRes *sensbRes, uint8_t *sensbResLen)
{
  return rfalNfcbPollerCheckPresence(((compMode == RFAL_COMPLIANCE_MODE_EMV) ? RFAL_NFCB_SENS_CMD_ALLB_REQ : RFAL_NFCB_SENS_CMD_SENSB_REQ), RFAL_NFCB_SLOT_NUM_1, sensbRes, sensbResLen);
}

/*******************************************************************************/
ReturnCode RfalNfcClass::rfalNfcbPollerStartTechnologyDetection(rfalComplianceMode compMode, rfalNfcbSensbRes *sensbRes, uint8_t *sensbResLen)
{
  return rfalNfcbPollerStartCheckPresence(((compMode == RFAL_COMPLIANCE_MODE_EMV) ? RFAL_NFCB_SENS_CMD_ALLB_REQ : RFAL_NFCB_SENS_CMD_SENSB_REQ), RFAL_NFCB_SLOT_NUM_1, sensbRes, sensbResLen);
}

/*******************************************************************************/
ReturnCode RfalNfcClass::rfalNfcbPollerGetTechnologyDetectionStatus(void)
{
  return rfalNfcbPollerGetCheckPresenceStatus();
}

/*******************************************************************************/
ReturnCode RfalNfcClass::rfalNfcbPollerCollisionResolution(rfalComplianceMode compMode, uint8_t devLimit, rfalNfcbListenDevice *nfcbDevList, uint8_t *devCnt)
{
  ReturnCode ret;

  EXIT_ON_ERR(ret, rfalNfcbPollerStartCollisionResolution(compMode, devLimit, nfcbDevList, devCnt));
  rfalRunBlocking(ret, rfalNfcbPollerGetCollisionResolutionStatus());

  return ret;
}

/*******************************************************************************/
ReturnCode RfalNfcClass::rfalNfcbPollerSlottedCollisionResolution(rfalComplianceMode compMode, uint8_t devLimit, rfalNfcbSlots initSlots, rfalNfcbSlots endSlots, rfalNfcbListenDevice *nfcbDevList, uint8_t *devCnt, bool *colPending)
{
  ReturnCode ret;

  EXIT_ON_ERR(ret, rfalNfcbPollerStartSlottedCollisionResolution(compMode, devLimit, initSlots, endSlots, nfcbDevList, devCnt, colPending));
  rfalRunBlocking(ret, rfalNfcbPollerGetCollisionResolutionStatus());

  return ret;
}

/*******************************************************************************/
ReturnCode RfalNfcClass::rfalNfcbPollerStartCollisionResolution(rfalComplianceMode compMode, uint8_t devLimit, rfalNfcbListenDevice *nfcbDevList, uint8_t *devCnt)
{
  return rfalNfcbPollerStartSlottedCollisionResolution(compMode, devLimit, RFAL_NFCB_SLOT_NUM_1, RFAL_NFCB_SLOT_NUM_16, nfcbDevList, devCnt, &gRfalNfcb.CR.colPend);
}

/*******************************************************************************/
ReturnCode RfalNfcClass::rfalNfcbPollerStartSlottedCollisionResolution(rfalComplianceMode compMode, uint8_t devLimit, rfalNfcbSlots initSlots, rfalNfcbSlots endSlots, rfalNfcbListenDevice *nfcbDevList, uint8_t *devCnt, bool *colPending)
{
  /* Check parameters. In ISO | Activity 1.0 mode the initial slots must be 1 as continuation of Technology Detection */
  if ((nfcbDevList == NULL) || (devCnt == NULL)  || (colPending == NULL) || (initSlots > RFAL_NFCB_SLOT_NUM_16) ||
      (endSlots > RFAL_NFCB_SLOT_NUM_16) || ((compMode == RFAL_COMPLIANCE_MODE_ISO) && (initSlots != RFAL_NFCB_SLOT_NUM_1))) {
    return ERR_PARAM;
  }

  (*devCnt)     = 0;
  (*colPending) = false;

  /* Store parameters */
  gRfalNfcb.CR.compMode    = compMode;
  gRfalNfcb.CR.devLimit    = devLimit;
  gRfalNfcb.CR.curSlots    = (uint8_t)initSlots;
  gRfalNfcb.CR.endSlots    = (uint8_t)endSlots;
  gRfalNfcb.CR.nfcbDevList = nfcbDevList;
  gRfalNfcb.CR.colPending  = colPending;
  gRfalNfcb.CR.devCnt      = devCnt;
  (*gRfalNfcb.CR.devCnt)   = 0U;
  gRfalNfcb.CR.curDevCnt   = 0U;
  gRfalNfcb.CR.curSlotNum  = 0U;
  gRfalNfcb.CR.tmr         = RFAL_TIMING_NONE;

  gRfalNfcb.CR.state = RFAL_NFCB_CR_SLOTS_TX;
  return ERR_NONE;
}

/*******************************************************************************/
ReturnCode RfalNfcClass::rfalNfcbPollerGetCollisionResolutionStatus(void)
{
  ReturnCode      ret;
  rfalNfcbSensCmd cmd;

  /* Check if operation is still not complete */
  if (gRfalNfcb.CR.tmr != RFAL_TIMING_NONE) {
    if (!nfcbTimerisExpired(gRfalNfcb.CR.tmr)) {
      return ERR_BUSY;
    }
  }

  switch (gRfalNfcb.CR.state) {
    /*******************************************************************************/
    case RFAL_NFCB_CR_SLOTS_TX:

      /* Check if it's the first iteration on ISO | Activity 1.0 mode */
      if ((gRfalNfcb.CR.compMode == RFAL_COMPLIANCE_MODE_ISO) && (gRfalNfcb.CR.curSlots == (uint8_t)RFAL_NFCB_SLOT_NUM_1)) {
        /* Do nothing in case Activity 1.0, where the previous SENSB_RES from technology detection should be used */
      }
      /* Send SENSB_REQ with number of slots if not the first   Activity 2.1  9.3.5.24  -  Symbol 23 */
      else if (gRfalNfcb.CR.curSlotNum == 0U) {
        /* Send ALLB_REQ   Activity 2.1   9.3.5.2 and 9.3.5.3  (Symbol 1 and 2) */
        cmd = ((gRfalNfcb.CR.curSlots == (uint8_t)RFAL_NFCB_SLOT_NUM_1) ? RFAL_NFCB_SENS_CMD_ALLB_REQ : RFAL_NFCB_SENS_CMD_SENSB_REQ);

        /* PRQA S 4342 1 # MISRA 10.5 - Layout of rfalNfcbSlots and the limited loop guarantee that no invalid enum values are created. */
        rfalNfcbPollerStartCheckPresence(cmd, (rfalNfcbSlots)gRfalNfcb.CR.curSlots, &gRfalNfcb.CR.nfcbDevList[*gRfalNfcb.CR.devCnt].sensbRes, &gRfalNfcb.CR.nfcbDevList[*gRfalNfcb.CR.devCnt].sensbResLen);
      } else {
        /* Activity 2.1  9.3.5.26  -  Symbol 25 */
        rfalNfcbPollerStartSlotMarker(gRfalNfcb.CR.curSlotNum, &gRfalNfcb.CR.nfcbDevList[*gRfalNfcb.CR.devCnt].sensbRes, &gRfalNfcb.CR.nfcbDevList[*gRfalNfcb.CR.devCnt].sensbResLen);
      }

      gRfalNfcb.CR.state = RFAL_NFCB_CR_SLOTS;
      return ERR_BUSY;


    /*******************************************************************************/
    case RFAL_NFCB_CR_SLOTS:

      EXIT_ON_BUSY(ret, rfalNfcbPollerGetSlotMarkerStatus());

      /*******************************************************************************/
      if (gRfalNfcb.CR.compMode == RFAL_COMPLIANCE_MODE_EMV) {
        /* Report (timeout) error immediately    EMVCo 3.0  9.6.1.3 */
        if (ret != ERR_NONE) {
          return ret;
        }

        /* Check if there was a transmission error on WUPB    EMVCo 3.0  9.3.3.1 */
        if (gRfalNfcb.CR.nfcbDevList->sensbResLen == 0U) {
          return ERR_FRAMING;
        }
      }


      /*******************************************************************************/
      /* Activity 2.1  9.3.5.7 and 9.3.5.8  -  Symbol 6 */
      if (ret != ERR_TIMEOUT) {
        /* Activity 2.1  9.3.5.8  -  Symbol 7 */
        if ((rfalNfcbCheckSensbRes(&gRfalNfcb.CR.nfcbDevList[*gRfalNfcb.CR.devCnt].sensbRes, gRfalNfcb.CR.nfcbDevList[*gRfalNfcb.CR.devCnt].sensbResLen) == ERR_NONE) && (ret == ERR_NONE)) {
          gRfalNfcb.CR.nfcbDevList[*gRfalNfcb.CR.devCnt].isSleep = false;

          if (gRfalNfcb.CR.compMode == RFAL_COMPLIANCE_MODE_EMV) {
            (*gRfalNfcb.CR.devCnt)++;
            return ret;
          } else if (gRfalNfcb.CR.compMode == RFAL_COMPLIANCE_MODE_ISO) {
            /* Activity 1.0  9.3.5.8  -  Symbol 7 */
            (*gRfalNfcb.CR.devCnt)++;
            gRfalNfcb.CR.curDevCnt++;

            /* Activity 1.0  9.3.5.10  -  Symbol 9 */
            if ((*gRfalNfcb.CR.devCnt >= gRfalNfcb.CR.devLimit) || (gRfalNfcb.CR.curSlotNum == (uint8_t)RFAL_NFCB_SLOT_NUM_1)) {
              return ret;
            }

            /* Activity 2.1  9.3.5.11  -  Symbol 10 */
            rfalNfcbPollerSleep(gRfalNfcb.CR.nfcbDevList[(*gRfalNfcb.CR.devCnt) - 1U].sensbRes.nfcid0);
            gRfalNfcb.CR.nfcbDevList[(*gRfalNfcb.CR.devCnt) - 1U].isSleep =  true;
          } else if (gRfalNfcb.CR.compMode == RFAL_COMPLIANCE_MODE_NFC) {
            /* Activity 2.1  9.3.5.10 and 9.3.5.11  -  Symbol 9 and Symbol 11*/
            if (gRfalNfcb.CR.curDevCnt != 0U) {
              rfalNfcbPollerSleepTx(gRfalNfcb.CR.nfcbDevList[(*gRfalNfcb.CR.devCnt) - (uint8_t)1U].sensbRes.nfcid0);
              gRfalNfcb.CR.nfcbDevList[(*gRfalNfcb.CR.devCnt) - (uint8_t)1U].isSleep = true;

              nfcbTimerStart(gRfalNfcb.CR.tmr, (uint16_t)rfalConv1fcToMs(RFAL_NFCB_ACTIVATION_FWT));
              ret = ERR_BUSY;
            }

            /* Activity 2.1  9.3.5.12  -  Symbol 11 */
            (*gRfalNfcb.CR.devCnt)++;
            gRfalNfcb.CR.curDevCnt++;

            /* Activity 2.1  9.3.5.6  -  Symbol 13 */
            if ((*gRfalNfcb.CR.devCnt >= gRfalNfcb.CR.devLimit) || (gRfalNfcb.CR.curSlots == (uint8_t)RFAL_NFCB_SLOT_NUM_1)) {
              gRfalNfcb.CR.state = RFAL_NFCB_CR_END;
              return ERR_BUSY;
            }
          } else {
            /* MISRA 15.7 - Empty else */
          }
        } else {
          /* If deviceLimit is set to 0 the NFC Forum Device is configured to perform collision detection only  Activity 1.0 and 1.1  9.3.5.5  - Symbol 4 */
          if ((gRfalNfcb.CR.devLimit == 0U) && (gRfalNfcb.CR.curSlotNum == (uint8_t)RFAL_NFCB_SLOT_NUM_1)) {
            return ERR_RF_COLLISION;
          }

          /* Activity 2.1  9.3.5.9  -  Symbol 8 */
          (*gRfalNfcb.CR.colPending) = true;
        }
      }

      /* Activity 2.1  9.3.5.15  -  Symbol 14 & 15*/
      if ((gRfalNfcb.CR.curSlotNum + 1U) < rfalNfcbNI2NumberOfSlots(gRfalNfcb.CR.curSlots)) {
        gRfalNfcb.CR.curSlotNum++;
        gRfalNfcb.CR.state = RFAL_NFCB_CR_SLOTS_TX;
      } else {
        /* Activity 2.1  9.3.5.17  -  Symbol 16 */
        if (!(*gRfalNfcb.CR.colPending)) {
          break;
        }

        /* Activity 1.1  9.3.5.18  -  Symbol 17 */
        if (gRfalNfcb.CR.curDevCnt == 0U) {
          /* Activity 2.1  9.3.5.19  -  Symbol 18 */
          if ((gRfalNfcb.CR.curSlotNum + 1U) >= rfalNfcbNI2NumberOfSlots(gRfalNfcb.CR.endSlots)) {
            break;
          }

          /* Activity 2.1  9.3.5.20  -  Symbol 19 */
          gRfalNfcb.CR.curSlots++;
        }

        gRfalNfcb.CR.state = RFAL_NFCB_CR_SLEEP;
      }

      return ERR_BUSY;


    /*******************************************************************************/
    case RFAL_NFCB_CR_SLEEP:

      /* Activity 2.1  9.3.5.23  -  Symbol 22 */
      if ((gRfalNfcb.CR.compMode == RFAL_COMPLIANCE_MODE_NFC) && (gRfalNfcb.CR.curDevCnt != 0U)) {
        rfalNfcbPollerSleepTx(gRfalNfcb.CR.nfcbDevList[((*gRfalNfcb.CR.devCnt) - (uint8_t)1U)].sensbRes.nfcid0);
        gRfalNfcb.CR.nfcbDevList[((*gRfalNfcb.CR.devCnt) - (uint8_t)1U)].isSleep = true;

        nfcbTimerStart(gRfalNfcb.CR.tmr, (uint16_t) rfalConv1fcToMs(RFAL_NFCB_ACTIVATION_FWT));
      }

      /* Activity 2.1  9.3.5.6  -  Symbol 5 */
      gRfalNfcb.CR.curSlotNum    = 0U;
      gRfalNfcb.CR.curDevCnt     = 0U;
      (*gRfalNfcb.CR.colPending) = false;

      gRfalNfcb.CR.state = RFAL_NFCB_CR_SLOTS_TX;
      return ERR_BUSY;

    /*******************************************************************************/
    case RFAL_NFCB_CR_END:
    default:
      /* MISRA 16.4: no empty default statement (a comment being enough) */
      break;
  }

  return ERR_NONE;
}

/*******************************************************************************/
uint32_t RfalNfcClass::rfalNfcbTR2ToFDT(uint8_t tr2Code)
{
  /*******************************************************************************/
  /* MISRA 8.9 An object should be defined at block scope if its identifier only appears in a single function */
  /*! TR2 Table according to Digital 1.1 Table 33 */
  const uint16_t rfalNfcbTr2Table[4] = { 1792, 3328, 5376, 9472 };
  /*******************************************************************************/

  return (uint32_t)rfalNfcbTr2Table[(tr2Code & RFAL_NFCB_SENSB_RES_PROTO_TR2_MASK) ];
}

#endif /* RFAL_FEATURE_NFCB */
