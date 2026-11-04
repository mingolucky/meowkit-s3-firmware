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
 *  \brief Provides NFC-A T2T convenience methods and definitions
 *
 *  This module provides an interface to perform as a NFC-A Reader/Writer
 *  to handle a Type 2 Tag T2T
 *
 *
 * \addtogroup RFAL
 * @{
 *
 * \addtogroup RFAL-AL
 * \brief RFAL Abstraction Layer
 * @{
 *
 * \addtogroup T2T
 * \brief RFAL T2T Module
 * @{
 *
 */


#ifndef RFAL_T2T_H
#define RFAL_T2T_H

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

#ifndef RFAL_FEATURE_T2T
  #define RFAL_FEATURE_T2T   false    /* T2T module configuration missing. Disabled by default */
#endif

/*
 ******************************************************************************
 * GLOBAL DEFINES
 ******************************************************************************
 */

#define RFAL_T2T_BLOCK_LEN            4U                          /*!< T2T block length           */
#define RFAL_T2T_READ_DATA_LEN        (4U * RFAL_T2T_BLOCK_LEN)   /*!< T2T READ data length       */
#define RFAL_T2T_WRITE_DATA_LEN       RFAL_T2T_BLOCK_LEN          /*!< T2T WRITE data length      */

/*
******************************************************************************
* GLOBAL TYPES
******************************************************************************
*/


/*
******************************************************************************
* GLOBAL FUNCTION PROTOTYPES
******************************************************************************
*/


#endif /* RFAL_T2T_H */

/**
  * @}
  *
  * @}
  *
  * @}
  */
