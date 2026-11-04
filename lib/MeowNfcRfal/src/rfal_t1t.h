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
 *  \brief Provides NFC-A T1T convenience methods and definitions
 *
 *  This module provides an interface to perform as a NFC-A Reader/Writer
 *  to handle a Type 1 Tag T1T (Topaz)
 *
 *
 * \addtogroup RFAL
 * @{
 *
 * \addtogroup RFAL-AL
 * \brief RFAL Abstraction Layer
 * @{
 *
 * \addtogroup T1T
 * \brief RFAL T1T Module
 * @{
 *
 */


#ifndef RFAL_T1T_H
#define RFAL_T1T_H

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

#ifndef RFAL_FEATURE_T1T
  #define RFAL_FEATURE_T1T   false    /* T1T module configuration missing. Disabled by default */
#endif

/*
 ******************************************************************************
 * GLOBAL DEFINES
 ******************************************************************************
 */
#define RFAL_T1T_UID_LEN               4   /*!< T1T UID length of cascade level 1 only tag  */
#define RFAL_T1T_HR_LENGTH             2   /*!< T1T HR(Header ROM) length                   */

#define RFAL_T1T_HR0_NDEF_MASK      0xF0   /*!< T1T HR0 NDEF capability mask  T1T 1.2 2.2.2 */
#define RFAL_T1T_HR0_NDEF_SUPPORT   0x10   /*!< T1T HR0 NDEF capable value    T1T 1.2 2.2.2 */


/*! NFC-A T1T (Topaz) command set */
typedef enum {
  RFAL_T1T_CMD_RID      = 0x78,          /*!< T1T Read UID                                */
  RFAL_T1T_CMD_RALL     = 0x00,          /*!< T1T Read All                                */
  RFAL_T1T_CMD_READ     = 0x01,          /*!< T1T Read                                    */
  RFAL_T1T_CMD_WRITE_E  = 0x53,          /*!< T1T Write with erase (single byte)          */
  RFAL_T1T_CMD_WRITE_NE = 0x1A           /*!< T1T Write with no erase (single byte)       */
} rfalT1Tcmds;


/*
******************************************************************************
* GLOBAL TYPES
******************************************************************************
*/


/*! NFC-A T1T (Topaz) RID_RES  Digital 1.1  10.6.2 & Table 50 */
typedef struct {
  uint8_t hr0;                           /*!< T1T Header ROM: HR0                         */
  uint8_t hr1;                           /*!< T1T Header ROM: HR1                         */
  uint8_t uid[RFAL_T1T_UID_LEN];         /*!< T1T UID                                     */
} rfalT1TRidRes;

/*
******************************************************************************
* GLOBAL FUNCTION PROTOTYPES
******************************************************************************
*/


#endif /* RFAL_T1T_H */

/**
  * @}
  *
  * @}
  *
  * @}
  */
