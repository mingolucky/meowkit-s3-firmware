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
 *  \brief Provides convenience methods and definitions for T4T (ISO7816-4)
 *
 *  This module provides an interface to exchange T4T APDUs according to
 *  NFC Forum T4T and ISO7816-4
 *
 *  This implementation was based on the following specs:
 *    - ISO/IEC 7816-4  3rd Edition 2013-04-15
 *    - NFC Forum T4T Technical Specification 1.0 2017-08-28
 *
 * \addtogroup RFAL
 * @{
 *
 * \addtogroup RFAL-AL
 * \brief RFAL Abstraction Layer
 * @{
 *
 * \addtogroup T4T
 * \brief RFAL T4T Module
 * @{
 *
 */


#ifndef RFAL_T4T_H
#define RFAL_T4T_H

/*
 ******************************************************************************
 * INCLUDES
 ******************************************************************************
 */
#include "st_errno.h"
#include "rfal_rf.h"
#include "rfal_isoDep.h"

/*
******************************************************************************
* ENABLE SWITCH
******************************************************************************
*/

#ifndef RFAL_FEATURE_T4T
  #define RFAL_FEATURE_T4T   false    /* T4T module configuration missing. Disabled by default */
#endif


/*
 ******************************************************************************
 * GLOBAL DEFINES
 ******************************************************************************
 */

#define RFAL_T4T_MAX_CAPDU_PROLOGUE_LEN                          4U                          /*!< Command-APDU prologue length (CLA INS P1 P2)                    */
#define RFAL_T4T_LE_LEN                                          1U                          /*!< Le Expected Response Length (short field coding)                */
#define RFAL_T4T_LC_LEN                                          1U                          /*!< Lc Data field length  (short field coding)                      */
#define RFAL_T4T_MAX_RAPDU_SW1SW2_LEN                            2U                          /*!< SW1 SW2 length                                                  */
#define RFAL_T4T_CLA                                          0x00U                          /*!< Class byte (contains 00h because secure message are not used)   */

#define RFAL_T4T_ISO7816_P1_SELECT_BY_DF_NAME                 0x04U                          /*!< P1 value for Select by name                                     */
#define RFAL_T4T_ISO7816_P1_SELECT_BY_FILEID                  0x00U                          /*!< P1 value for Select by file identifier                          */
#define RFAL_T4T_ISO7816_P2_SELECT_FIRST_OR_ONLY_OCCURENCE    0x00U                          /*!<      b2b1 P2 value for First or only occurrence                  */
#define RFAL_T4T_ISO7816_P2_SELECT_RETURN_FCI_TEMPLATE        0x00U                          /*!< b4b3      P2 value for Return FCI template                      */
#define RFAL_T4T_ISO7816_P2_SELECT_NO_RESPONSE_DATA           0x0CU                          /*!< b4b3      P2 value for No response data                         */

#define RFAL_T4T_ISO7816_STATUS_COMPLETE                      0x9000U                        /*!< Command completed \ Normal processing - No further qualification*/


/*
******************************************************************************
* GLOBAL VARIABLES
******************************************************************************
*/


/*
******************************************************************************
* GLOBAL TYPES
******************************************************************************
*/
/*! NFC-A T4T Command-APDU structure */
typedef struct {
  uint8_t                  CLA;                              /*!< Class byte                                         */
  uint8_t                  INS;                              /*!< Instruction byte                                   */
  uint8_t                  P1;                               /*!< Parameter byte 1                                   */
  uint8_t                  P2;                               /*!< Parameter byte 2                                   */
  uint8_t                  Lc;                               /*!< Data field length                                  */
  bool                     LcFlag;                           /*!< Lc flag (append Lc when true)                      */
  uint8_t                  Le;                               /*!< Expected Response Length                           */
  bool                     LeFlag;                           /*!< Le flag (append Le when true)                      */

  rfalIsoDepApduBufFormat  *cApduBuf;                        /*!< Command-APDU buffer  (Tx)                          */
  uint16_t                 *cApduLen;                        /*!< Command-APDU Length                                */
} rfalT4tCApduParam;

/*! NFC-A T4T Response-APDU structure */
typedef struct {
  rfalIsoDepApduBufFormat  *rApduBuf;                        /*!< Response-APDU buffer (Rx)                          */
  uint16_t                 rcvdLen;                          /*!< Full response length                               */
  uint16_t                 rApduBodyLen;                     /*!< Response body length                               */
  uint16_t                 statusWord;                       /*!< R-APDU Status Word SW1|SW2                         */
} rfalT4tRApduParam;



/*! NFC-A T4T command set    T4T 1.0 & ISO7816-4 2013 Table 4 */
typedef enum {
  RFAL_T4T_INS_SELECT           = 0xA4U,                     /*!< T4T Select                                         */
  RFAL_T4T_INS_READBINARY       = 0xB0U,                     /*!< T4T ReadBinary                                     */
  RFAL_T4T_INS_UPDATEBINARY     = 0xD6U,                     /*!< T4T UpdateBinay                                    */
  RFAL_T4T_INS_READBINARY_ODO   = 0xB1U,                     /*!< T4T ReadBinary using ODO                           */
  RFAL_T4T_INS_UPDATEBINARY_ODO = 0xD7U                      /*!< T4T UpdateBinay using ODO                          */
} rfalT4tCmds;

/*
******************************************************************************
* GLOBAL FUNCTION PROTOTYPES
******************************************************************************
*/


#endif /* RFAL_T4T_H */

/**
  * @}
  *
  * @}
  *
  * @}
  */
