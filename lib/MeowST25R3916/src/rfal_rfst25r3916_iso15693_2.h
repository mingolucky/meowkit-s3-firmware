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
 *  \brief Implementation of ISO-15693-2
 *
 */
/*!
 *
 */

#ifndef RFAL_ISO_15693_2_H
#define RFAL_ISO_15693_2_H

/*
******************************************************************************
* INCLUDES
******************************************************************************
*/
#include "st_errno.h"

/*
******************************************************************************
* ENABLE SWITCH
******************************************************************************
*/

#ifndef RFAL_FEATURE_NFCV
  #define RFAL_FEATURE_NFCV   false    /* NFC-V module configuration missing. Disabled by default */
#endif

/*
******************************************************************************
* GLOBAL DATATYPES
******************************************************************************
*/
/*! Enum holding possible VCD codings  */
typedef enum {
  ISO15693_VCD_CODING_1_4,
  ISO15693_VCD_CODING_1_256
} rfalIso15693VcdCoding_t;

/*! Enum holding possible VICC datarates */

/*! Configuration parameter used by #iso15693PhyConfigure  */
typedef struct {
  rfalIso15693VcdCoding_t coding;           /*!< desired VCD coding                                       */
  uint32_t                speedMode;    /*!< 0: normal mode, 1: 2^1 = x2 Fast mode, 2 : 2^2 = x4 mode, 3 : 2^3 = x8 mode - all rx pulse numbers and times are divided by 1,2,4,8 */
} rfalIso15693PhyConfig_t;

/*! Parameters how the stream mode should work */
struct rfalIso15693StreamConfig {
  uint8_t useBPSK;              /*!< 0: subcarrier, 1:BPSK */
  uint8_t din;                  /*!< the divider for the in subcarrier frequency: fc/2^din  */
  uint8_t dout;                 /*!< the divider for the in subcarrier frequency fc/2^dout */
  uint8_t report_period_length; /*!< the length of the reporting period 2^report_period_length*/
};
/*
******************************************************************************
* GLOBAL CONSTANTS
******************************************************************************
*/

#define ISO15693_REQ_FLAG_TWO_SUBCARRIERS 0x01U   /*!< Flag indication that communication uses two subcarriers */
#define ISO15693_REQ_FLAG_HIGH_DATARATE   0x02U   /*!< Flag indication that communication uses high bitrate    */
#define ISO15693_MASK_FDT_LISTEN         (65)     /*!< t1min = 308,2us = 4192/fc = 65.5 * 64/fc                */

/*! t1max = 323,3us = 4384/fc = 68.5 * 64/fc
 *         12 = 768/fc unmodulated time of single subcarrior SoF */
#define ISO15693_FWT (69 + 12)


#endif /* RFAL_ISO_15693_2_H */
