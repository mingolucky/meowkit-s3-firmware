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
 *  \brief NFC-V ST25 NFC-V Tag specific features
 *
 *  This module provides support for ST's specific features available on
 *  NFC-V (ISO15693) tag families: ST25D, ST25TV, M24LR
 *
 *
 * \addtogroup RFAL
 * @{
 *
 * \addtogroup RFAL-AL
 * \brief RFAL Abstraction Layer
 * @{
 *
 * \addtogroup ST25xV
 * \brief RFAL ST25xV Module
 * @{
 *
 */

#ifndef RFAL_ST25xV_H
#define RFAL_ST25xV_H

/*
 ******************************************************************************
 * INCLUDES
 ******************************************************************************
 */
#include "st_errno.h"
#include "rfal_nfc.h"
#include "rfal_rf.h"

/*
******************************************************************************
* ENABLE SWITCH
******************************************************************************
*/

#ifndef RFAL_FEATURE_ST25xV
  #define RFAL_FEATURE_ST25xV   false    /* ST25xV module configuration missing. Disabled by default */
#endif

/*
 ******************************************************************************
 * GLOBAL DEFINES
 ******************************************************************************
 */


#define RFAL_NFCV_BLOCKNUM_M24LR_LEN       2U      /*!< Block Number length of MR24LR tags: 16 bits                */



#endif /* RFAL_ST25xV_H */

/**
  * @}
  *
  * @}
  *
  * @}
  */

