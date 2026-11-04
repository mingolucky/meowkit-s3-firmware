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
 *  \brief Implementation of NFC-F Poller (FeliCa PCD) device
 *
 *  The definitions and helpers methods provided by this module are
 *  aligned with NFC-F (FeliCa - JIS X6319-4)
 *
 *
 * \addtogroup RFAL
 * @{
 *
 * \addtogroup RFAL-AL
 * \brief RFAL Abstraction Layer
 * @{
 *
 * \addtogroup NFC-F
 * \brief RFAL NFC-F Module
 * @{
 *
 */


#ifndef RFAL_NFCF_H
#define RFAL_NFCF_H

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

#ifndef RFAL_FEATURE_NFCF
  #define RFAL_FEATURE_NFCF   false    /* NFC-F module configuration missing. Disabled by default */
#endif

/*
 ******************************************************************************
 * GLOBAL DEFINES
 ******************************************************************************
 */

#define RFAL_NFCF_NFCID2_LEN                    8U       /*!< NFCID2 (FeliCa IDm) length                        */
#define RFAL_NFCF_SENSF_RES_LEN_MIN             16U      /*!< SENSF_RES minimum length                          */
#define RFAL_NFCF_SENSF_RES_LEN_MAX             18U      /*!< SENSF_RES maximum length                          */
#define RFAL_NFCF_SENSF_RES_PAD0_LEN            2U       /*!< SENSF_RES PAD0 length                             */
#define RFAL_NFCF_SENSF_RES_PAD1_LEN            3U       /*!< SENSF_RES PAD1 length                             */
#define RFAL_NFCF_SENSF_RES_RD_LEN              2U       /*!< SENSF_RES Request Data length                     */
#define RFAL_NFCF_SENSF_RES_BYTE1               1U       /*!< SENSF_RES first byte value                        */
#define RFAL_NFCF_SENSF_SC_LEN                  2U       /*!< Felica SENSF_REQ System Code length               */
#define RFAL_NFCF_SENSF_PARAMS_SC1_POS          0U       /*!< System Code byte1 position in the SENSF_REQ       */
#define RFAL_NFCF_SENSF_PARAMS_SC2_POS          1U       /*!< System Code byte2 position in the SENSF_REQ       */
#define RFAL_NFCF_SENSF_PARAMS_RC_POS           2U       /*!< Request Code position in the SENSF_REQ            */
#define RFAL_NFCF_SENSF_PARAMS_TSN_POS          3U       /*!< Time Slot Number position in the SENSF_REQ        */
#define RFAL_NFCF_POLL_MAXCARDS                 16U      /*!< Max number slots/cards 16                         */

#define RFAL_NFCF_CMD_POS                        0U      /*!< Command/Response code length                      */
#define RFAL_NFCF_CMD_LEN                        1U      /*!< Command/Response code length                      */
#define RFAL_NFCF_LENGTH_LEN                     1U      /*!< LEN field length                                  */
#define RFAL_NFCF_HEADER_LEN                     (RFAL_NFCF_LENGTH_LEN + RFAL_NFCF_CMD_LEN) /*!< Header length  */

#define RFAL_NFCF_NOS_LEN                        1U      /*!< Number of Services length                         */
#define RFAL_NFCF_NOB_LEN                        1U      /*!< Number of Blocks length                           */

#define RFAL_NFCF_SENSF_NFCID2_BYTE1_POS         0U      /*!< NFCID2 byte1 position                             */
#define RFAL_NFCF_SENSF_NFCID2_BYTE2_POS         1U      /*!< NFCID2 byte2 position                             */

#define RFAL_NFCF_SENSF_NFCID2_PROT_TYPE_LEN     2U      /*!< NFCID2 length for byte 1 and byte 2 indicating NFC-DEP or T3T support */
#define RFAL_NFCF_SENSF_NFCID2_BYTE1_NFCDEP      0x01U   /*!< NFCID2 byte1 NFC-DEP support            Digital 1.0 Table 44 */
#define RFAL_NFCF_SENSF_NFCID2_BYTE2_NFCDEP      0xFEU   /*!< NFCID2 byte2 NFC-DEP support            Digital 1.0 Table 44 */

#define RFAL_NFCF_SYSTEMCODE                     0xFFFFU /*!< SENSF_RES Default System Code            Digital 2.3 8.6.1.5 */
#define RFAL_NFCF_SYSTEMCODE_LEN                 2U      /*!< SENSF_RES System Code length             Digital 2.3 8.6.1   */

#define RFAL_NFCF_BLOCK_LEN                      16U     /*!< NFCF T3T Block size                        T3T 1.0  4.1      */
#define RFAL_NFCF_CHECKUPDATE_RES_ST1_POS        9U      /*!< Check|Update Res Status Flag 1 position    T3T 1.0  Table 8  */
#define RFAL_NFCF_CHECKUPDATE_RES_ST2_POS        10U     /*!< Check|Update Res Status Flag 2 position    T3T 1.0  Table 8  */
#define RFAL_NFCF_CHECKUPDATE_RES_NOB_POS        11U     /*!< Check|Update Res Number of Blocks position T3T 1.0  Table 8  */

#define RFAL_NFCF_STATUS_FLAG_SUCCESS            0x00U   /*!< Check response Number of Blocks position   T3T 1.0  Table 11 */
#define RFAL_NFCF_STATUS_FLAG_ERROR              0xFFU   /*!< Check response Number of Blocks position   T3T 1.0  Table 11 */

#define RFAL_NFCF_BLOCKLISTELEM_MAX_LEN          3U      /*!< Block List Element max Length (3 bytes)        T3T 1.0 5.6.1 */
#define RFAL_NFCF_BLOCKLISTELEM_LEN_BIT          0x80U   /*!< Block List Element Length bit (2|3 bytes)      T3T 1.0 5.6.1 */

#define RFAL_NFCF_SERVICECODE_RDONLY           0x000BU   /*!< NDEF Service Code as Read-Only                 T3T 1.0 7.2.1 */
#define RFAL_NFCF_SERVICECODE_RDWR             0x0009U   /*!< NDEF Service Code as Read and Write            T3T 1.0 7.2.1 */

#define RFAL_NFCF_TEST_LB_CMD0                   0xD8U /*!< T3T loopback CMD0                 ETSI TS 102 695-1  5.6.4.4.2 */
#define RFAL_NFCF_TEST_LB_CMD1                   0x00U /*!< T3T loopback CMD1                 ETSI TS 102 695-1  5.6.4.4.2 */

/*! NFC-F Felica command set   JIS X6319-4  9.1 */
enum {
  RFAL_NFCF_CMD_POLLING                      = 0x00, /*!< SENSF_REQ (Felica Poll/REQC command to identify a card )       */
  RFAL_NFCF_CMD_POLLING_RES                  = 0x01, /*!< SENSF_RES (Felica Poll/REQC command response )                 */
  RFAL_NFCF_CMD_REQUEST_SERVICE              = 0x02, /*!< verify the existence of Area and Service                       */
  RFAL_NFCF_CMD_REQUEST_RESPONSE             = 0x04, /*!< verify the existence of a card                                 */
  RFAL_NFCF_CMD_READ_WITHOUT_ENCRYPTION      = 0x06, /*!< read Block Data from a Service that requires no authentication */
  RFAL_NFCF_CMD_READ_WITHOUT_ENCRYPTION_RES  = 0x07, /*!< read Block Data response from a Service with no authentication */
  RFAL_NFCF_CMD_WRITE_WITHOUT_ENCRYPTION     = 0x08, /*!< write Block Data to a Service that requires no authentication  */
  RFAL_NFCF_CMD_WRITE_WITHOUT_ENCRYPTION_RES = 0x09, /*!< write Block Data response to a Service with no authentication  */
  RFAL_NFCF_CMD_REQUEST_SYSTEM_CODE          = 0x0c, /*!< acquire the System Code registered to a card                   */
  RFAL_NFCF_CMD_AUTHENTICATION1              = 0x10, /*!< authenticate a card                                            */
  RFAL_NFCF_CMD_AUTHENTICATION2              = 0x12, /*!< allow a card to authenticate a Reader/Writer                   */
  RFAL_NFCF_CMD_READ                         = 0x14, /*!< read Block Data from a Service that requires authentication    */
  RFAL_NFCF_CMD_WRITE                        = 0x16, /*!< write Block Data to a Service that requires authentication     */
};

/*
 ******************************************************************************
 * GLOBAL MACROS
 ******************************************************************************
 */

/*! Checks if the given NFC-F device indicates NFC-DEP support */
#define rfalNfcfIsNfcDepSupported( dev )  ( (((rfalNfcfListenDevice*)(dev))->sensfRes.NFCID2[RFAL_NFCF_SENSF_NFCID2_BYTE1_POS] == RFAL_NFCF_SENSF_NFCID2_BYTE1_NFCDEP) && \
                                            (((rfalNfcfListenDevice*)(dev))->sensfRes.NFCID2[RFAL_NFCF_SENSF_NFCID2_BYTE2_POS] == RFAL_NFCF_SENSF_NFCID2_BYTE2_NFCDEP)    )


/*
******************************************************************************
* GLOBAL TYPES
******************************************************************************
*/


/*! NFC-F SENSF_RES format  Digital 1.1  8.6.2 */
typedef struct {
  uint8_t CMD;                                /*!< Command Code: 01h  */
  uint8_t NFCID2[RFAL_NFCF_NFCID2_LEN];       /*!< NFCID2             */
  uint8_t PAD0[RFAL_NFCF_SENSF_RES_PAD0_LEN]; /*!< PAD0               */
  uint8_t PAD1[RFAL_NFCF_SENSF_RES_PAD1_LEN]; /*!< PAD1               */
  uint8_t MRTIcheck;                          /*!< MRTIcheck          */
  uint8_t MRTIupdate;                         /*!< MRTIupdate         */
  uint8_t PAD2;                               /*!< PAD2               */
  uint8_t RD[RFAL_NFCF_SENSF_RES_RD_LEN];     /*!< Request Data       */
} rfalNfcfSensfRes;


/*! NFC-F poller device (PCD) struct  */
typedef struct {
  uint8_t NFCID2[RFAL_NFCF_NFCID2_LEN];       /*!< NFCID2             */
} rfalNfcfPollDevice;

/*! NFC-F listener device (PICC) struct  */
typedef struct {
  uint8_t           sensfResLen;              /*!< SENF_RES length    */
  rfalNfcfSensfRes  sensfRes;                 /*!< SENF_RES           */
} rfalNfcfListenDevice;

typedef  uint16_t rfalNfcfServ;                 /*!< NFC-F Service Code */

/*! NFC-F Block List Element (2 or 3 bytes element)       T3T 1.0 5.6.1 */
typedef struct {
  uint8_t  conf;               /*!<  Access Mode | Serv Code List Order */
  uint16_t blockNum;         /*!<  Block Number                       */
} rfalNfcfBlockListElem;

/*! Check Update Service list and Block list parameter */
typedef struct {
  uint8_t               numServ;              /*!< Number of Services */
  rfalNfcfServ          *servList;            /*!< Service Code List  */
  uint8_t               numBlock;             /*!< Number of Blocks   */
  rfalNfcfBlockListElem *blockList;           /*!< Block Number List  */
} rfalNfcfServBlockListParam;


/*! Structure/Buffer to hold the SENSF_RES with LEN byte prepended                                 */
typedef struct {
  uint8_t           LEN;                                /*!< NFC-F LEN byte                      */
  rfalNfcfSensfRes  SENSF_RES;                          /*!< SENSF_RES                           */
} rfalNfcfSensfResBuf;
/*! Greedy collection for NFCF GRE_POLL_F  Activity 1.0 Table 10                                   */
typedef struct {
  uint8_t              pollFound;                       /*!< Number of devices found by the Poll */
  uint8_t              pollCollision;                   /*!< Number of collisions detected       */
  rfalFeliCaPollRes    POLL_F[RFAL_NFCF_POLL_MAXCARDS]; /*!< GRE_POLL_F   Activity 1.0 Table 10  */
} rfalNfcfGreedyF;
/*! NFC-F SENSF_REQ format  Digital 1.1  8.6.1                     */
typedef struct {
  uint8_t  CMD;                          /*!< Command code: 00h  */
  uint8_t  SC[RFAL_NFCF_SENSF_SC_LEN];   /*!< System Code        */
  uint8_t  RC;                           /*!< Request Code       */
  uint8_t  TSN;                          /*!< Time Slot Number   */
} rfalNfcfSensfReq;

/*
******************************************************************************
* GLOBAL FUNCTION PROTOTYPES
******************************************************************************
*/


#endif /* RFAL_NFCF_H */

/**
  * @}
  *
  * @}
  *
  * @}
  */
