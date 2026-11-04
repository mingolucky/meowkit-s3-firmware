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
 *
 *  An usage example is provided here: \ref exampleRfalNfca.cpp
 *  \example exampleRfalNfca.cpp
 *
 *
 * \addtogroup RFAL
 * @{
 *
 * \addtogroup RFAL-AL
 * \brief RFAL Abstraction Layer
 * @{
 *
 * \addtogroup NFC-A
 * \brief RFAL NFC-A Module
 * @{
 *
 */


#ifndef RFAL_NFCA_H
#define RFAL_NFCA_H

/*
 ******************************************************************************
 * INCLUDES
 ******************************************************************************
 */
#include "st_errno.h"
#include "rfal_rf.h"
#include "rfal_t1t.h"

/*
******************************************************************************
* ENABLE SWITCH
******************************************************************************
*/

#ifndef RFAL_FEATURE_NFCA
  #define RFAL_FEATURE_NFCA   false    /* NFC-A module configuration missing. Disabled by default */
#endif


/*
 ******************************************************************************
 * GLOBAL DEFINES
 ******************************************************************************
 */

#define RFAL_NFCA_CASCADE_1_UID_LEN                           4U    /*!< UID length of cascade level 1 only tag                            */
#define RFAL_NFCA_CASCADE_2_UID_LEN                           7U    /*!< UID length of cascade level 2 only tag                            */
#define RFAL_NFCA_CASCADE_3_UID_LEN                           10U   /*!< UID length of cascade level 3 only tag                            */

#define RFAL_NFCA_SENS_RES_PLATFORM_MASK                      0x0FU /*!< SENS_RES (ATQA) platform configuration mask  Digital 1.1 Table 10 */
#define RFAL_NFCA_SENS_RES_PLATFORM_T1T                       0x0CU /*!< SENS_RES (ATQA) T1T platform configuration  Digital 1.1 Table 10  */

#define RFAL_NFCA_SEL_RES_CONF_MASK                           0x60U /*!< SEL_RES (SAK) platform configuration mask  Digital 1.1 Table 19   */
#define RFAL_NFCA_SEL_RES_CONF_T2T                            0x00U /*!< SEL_RES (SAK) T2T configuration  Digital 1.1 Table 19             */
#define RFAL_NFCA_SEL_RES_CONF_T4T                            0x20U /*!< SEL_RES (SAK) T4T configuration  Digital 1.1 Table 19             */
#define RFAL_NFCA_SEL_RES_CONF_NFCDEP                         0x40U /*!< SEL_RES (SAK) NFC-DEP configuration  Digital 1.1 Table 19         */
#define RFAL_NFCA_SEL_RES_CONF_T4T_NFCDEP                     0x60U /*!< SEL_RES (SAK) T4T and NFC-DEP configuration  Digital 1.1 Table 19 */


/*! NFC-A minimum FDT(listen) = ((n * 128 + (84)) / fc) with n_min = 9      Digital 1.1  6.10.1
 *                            = (1236)/fc
 * Relax with 3etu: (3*128)/fc as with multiple NFC-A cards, response may take longer (JCOP cards)
 *                            = (1236 + 384)/fc = 1620 / fc                                      */
#define RFAL_NFCA_FDTMIN          1620U
/*
 ******************************************************************************
 * GLOBAL MACROS
 ******************************************************************************
 */

/*! Checks if device is a T1T given its SENS_RES */
#define rfalNfcaIsSensResT1T( sensRes )          ((((rfalNfcaSensRes*)(sensRes))->platformInfo & RFAL_NFCA_SENS_RES_PLATFORM_MASK) == RFAL_NFCA_SENS_RES_PLATFORM_T1T )

/*! Checks if device is a T2T given its SENS_RES */
#define rfalNfcaIsSelResT2T( selRes )            ((((rfalNfcaSelRes*)(selRes))->sak & RFAL_NFCA_SEL_RES_CONF_MASK) == RFAL_NFCA_SEL_RES_CONF_T2T )

/*! Checks if device is a T4T given its SENS_RES */
#define rfalNfcaIsSelResT4T( selRes )            ((((rfalNfcaSelRes*)(selRes))->sak & RFAL_NFCA_SEL_RES_CONF_MASK) == RFAL_NFCA_SEL_RES_CONF_T4T )

/*! Checks if device supports NFC-DEP protocol given its SENS_RES */
#define rfalNfcaIsSelResNFCDEP( selRes )         ((((rfalNfcaSelRes*)(selRes))->sak & RFAL_NFCA_SEL_RES_CONF_MASK) == RFAL_NFCA_SEL_RES_CONF_NFCDEP )

/*! Checks if device supports ISO-DEP and NFC-DEP protocol given its SENS_RES */
#define rfalNfcaIsSelResT4TNFCDEP( selRes )      ((((rfalNfcaSelRes*)(selRes))->sak & RFAL_NFCA_SEL_RES_CONF_MASK) == RFAL_NFCA_SEL_RES_CONF_T4T_NFCDEP )

/*! Checks if a NFC-A listener device supports multiple protocols (ISO-DEP and NFC-DEP) */
#define rfalNfcaLisDevIsMultiProto( lisDev )     (((rfalNfcaListenDevice*)(lisDev))->type == RFAL_NFCA_T4T_NFCDEP )

/*
******************************************************************************
* GLOBAL TYPES
******************************************************************************
*/

/*! NFC-A Listen device types */
typedef enum {
  RFAL_NFCA_T1T        = 0x01,                                  /* Device configured for T1T  Digital 1.1 Table 9                               */
  RFAL_NFCA_T2T        = 0x00,                                  /* Device configured for T2T  Digital 1.1 Table 19                              */
  RFAL_NFCA_T4T        = 0x20,                                  /* Device configured for T4T  Digital 1.1 Table 19                              */
  RFAL_NFCA_NFCDEP     = 0x40,                                  /* Device configured for NFC-DEP  Digital 1.1 Table 19                          */
  RFAL_NFCA_T4T_NFCDEP = 0x60                                   /* Device configured for NFC-DEP and T4T  Digital 1.1 Table 19                  */
} rfalNfcaListenDeviceType;


/*! SENS_RES (ATQA) format  Digital 1.1  6.6.3 & Table 7 */
typedef struct {
  uint8_t      anticollisionInfo;                               /*!< SENS_RES Anticollision Information                                         */
  uint8_t      platformInfo;                                    /*!< SENS_RES Platform Information                                              */
} rfalNfcaSensRes;


/*! SDD_REQ (Anticollision) format   Digital 1.1  6.7.1 & Table 11 */
typedef struct {
  uint8_t      selCmd;                                          /*!< SDD_REQ SEL_CMD: cascade Level                                             */
  uint8_t      selPar;                                          /*!< SDD_REQ SEL_PAR: Byte Count[4b] | Bit Count[4b] (NVB: Number of Valid Bits)*/
} rfalNfcaSddReq;


/*! SDD_RES (UID CLn) format   Digital 1.1  6.7.2 & Table 15 */
typedef struct {
  uint8_t      nfcid1[RFAL_NFCA_CASCADE_1_UID_LEN];             /*!< NFCID1 cascade level NFCID                                                 */
  uint8_t      bcc;                                             /*!< BCC Exclusive-OR over first 4 bytes of SDD_RES                             */
} rfalNfcaSddRes;


/*! SEL_REQ (Select) format   Digital 1.1  6.8.1 & Table 17 */
typedef struct {
  uint8_t      selCmd;                                          /*!< SDD_REQ SEL_CMD: cascade Level                                             */
  uint8_t      selPar;                                          /*!< SDD_REQ SEL_PAR: Byte Count[4b] | Bit Count[4b] (NVB: Number of Valid Bits)*/
  uint8_t      nfcid1[RFAL_NFCA_CASCADE_1_UID_LEN];             /*!< NFCID1 data                                                                */
  uint8_t      bcc;                                             /*!< Checksum calculated as exclusive-OR over the 4 bytes of NFCID1 CLn         */
} rfalNfcaSelReq;


/*! SEL_RES (SAK) format   Digital 1.1  6.8.2 & Table 19 */
typedef struct {
  uint8_t      sak;                                             /*!< Select Acknowledge                                                         */
} rfalNfcaSelRes;


/*! NFC-A listener device (PICC) struct  */
typedef struct {
  rfalNfcaListenDeviceType type;                                /*!< NFC-A Listen device type                                                   */
  rfalNfcaSensRes          sensRes;                             /*!< SENS_RES (ATQA)                                                            */
  rfalNfcaSelRes           selRes;                              /*!< SEL_RES  (SAK)                                                             */
  uint8_t                  nfcId1Len;                           /*!< NFCID1 Length                                                              */
  uint8_t                  nfcId1[RFAL_NFCA_CASCADE_3_UID_LEN]; /*!< NFCID1   (UID)                                                             */
#ifdef RFAL_FEATURE_T1T
  rfalT1TRidRes            ridRes;                              /*!< RID_RES                                                                    */
#endif /* RFAL_FEATURE_T1T */
  bool                     isSleep;                             /*!< Device sleeping flag                                                       */
} rfalNfcaListenDevice;

/*
******************************************************************************
* GLOBAL FUNCTION PROTOTYPES
******************************************************************************
*/

#endif /* RFAL_NFCA_H */

/**
  * @}
  *
  * @}
  *
  * @}
  */
