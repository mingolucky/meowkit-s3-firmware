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
 *  \brief Implementation of ST25TB interface
 *
 *
 * \addtogroup RFAL
 * @{
 *
 * \addtogroup RFAL-AL
 * \brief RFAL Abstraction Layer
 * @{
 *
 * \addtogroup ST25TB
 * \brief RFAL ST25TB Module
 * @{
 *
 */


#ifndef RFAL_ST25TB_H
#define RFAL_ST25TB_H

/*
 ******************************************************************************
 * INCLUDES
 ******************************************************************************
 */
#include "st_errno.h"
#include "rfal_rf.h"
#include "rfal_nfcb.h"

/*
******************************************************************************
* ENABLE SWITCH
******************************************************************************
*/
#ifndef RFAL_FEATURE_ST25TB
  #define RFAL_FEATURE_ST25TB   false    /* ST25TB module configuration missing. Disabled by default */
#endif

/*
 ******************************************************************************
 * GLOBAL DEFINES
 ******************************************************************************
 */

#define RFAL_ST25TB_CHIP_ID_LEN      1U       /*!< ST25TB chip ID length       */
#define RFAL_ST25TB_CRC_LEN          2U       /*!< ST25TB CRC length           */
#define RFAL_ST25TB_UID_LEN          8U       /*!< ST25TB Unique ID length     */
#define RFAL_ST25TB_BLOCK_LEN        4U       /*!< ST25TB Data Block length    */

/*
******************************************************************************
* GLOBAL MACROS
******************************************************************************
*/



/*
******************************************************************************
* GLOBAL TYPES
******************************************************************************
*/
typedef uint8_t rfalSt25tbUID[RFAL_ST25TB_UID_LEN];        /*!< ST25TB UID type          */
typedef uint8_t rfalSt25tbBlock[RFAL_ST25TB_BLOCK_LEN];    /*!< ST25TB Block type        */


/*! ST25TB listener device (PICC) struct  */
typedef struct {
  uint8_t           chipID;                              /*!< Device's session Chip ID */
  rfalSt25tbUID     UID;                                 /*!< Device's UID             */
  bool              isDeselected;                        /*!< Device deselect flag     */
} rfalSt25tbListenDevice;


/*
******************************************************************************
* GLOBAL FUNCTION PROTOTYPES
******************************************************************************
*/


#endif /* RFAL_ST25TB_H */

/**
  * @}
  *
  * @}
  *
  * @}
  */

