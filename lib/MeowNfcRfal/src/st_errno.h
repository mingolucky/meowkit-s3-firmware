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

/*! \file st_errno.h
 *
 *  \author SRA
 *
 *  \brief Main error codes
 *
 */

#ifndef ST_ERRNO_H
#define ST_ERRNO_H

/*
******************************************************************************
* INCLUDES
******************************************************************************
*/
#include "Arduino.h"

/*
******************************************************************************
* GLOBAL DATA TYPES
******************************************************************************
*/

typedef uint16_t      ReturnCode; /*!< Standard Return Code type from function. */

/*
******************************************************************************
* DEFINES
******************************************************************************
*/


/*
 * Error codes to be used within the application.
 * They are represented by an uint8_t
 */

#define ERR_NONE                           ((ReturnCode)0U)  /*!< no error occurred */
#define ERR_NOMEM                          ((ReturnCode)1U)  /*!< not enough memory to perform the requested operation */
#define ERR_BUSY                           ((ReturnCode)2U)  /*!< device or resource busy */
#define ERR_IO                             ((ReturnCode)3U)  /*!< generic IO error */
#define ERR_TIMEOUT                        ((ReturnCode)4U)  /*!< error due to timeout */
#define ERR_REQUEST                        ((ReturnCode)5U)  /*!< invalid request or requested function can't be executed at the moment */
#define ERR_NOMSG                          ((ReturnCode)6U)  /*!< No message of desired type */
#define ERR_PARAM                          ((ReturnCode)7U)  /*!< Parameter error */
#define ERR_SYSTEM                         ((ReturnCode)8U)  /*!< System error */
#define ERR_FRAMING                        ((ReturnCode)9U)  /*!< Framing error */
#define ERR_OVERRUN                        ((ReturnCode)10U) /*!< lost one or more received bytes */
#define ERR_PROTO                          ((ReturnCode)11U) /*!< protocol error */
#define ERR_INTERNAL                       ((ReturnCode)12U) /*!< Internal Error */
#define ERR_AGAIN                          ((ReturnCode)13U) /*!< Call again */
#define ERR_MEM_CORRUPT                    ((ReturnCode)14U) /*!< memory corruption */
#define ERR_NOT_IMPLEMENTED                ((ReturnCode)15U) /*!< not implemented */
#define ERR_PC_CORRUPT                     ((ReturnCode)16U) /*!< Program Counter has been manipulated or spike/noise trigger illegal operation */
#define ERR_SEND                           ((ReturnCode)17U) /*!< error sending*/
#define ERR_IGNORE                         ((ReturnCode)18U) /*!< indicates error detected but to be ignored */
#define ERR_SEMANTIC                       ((ReturnCode)19U) /*!< indicates error in state machine (unexpected cmd) */
#define ERR_SYNTAX                         ((ReturnCode)20U) /*!< indicates error in state machine (unknown cmd) */
#define ERR_CRC                            ((ReturnCode)21U) /*!< crc error */
#define ERR_NOTFOUND                       ((ReturnCode)22U) /*!< transponder not found */
#define ERR_NOTUNIQUE                      ((ReturnCode)23U) /*!< transponder not unique - more than one transponder in field */
#define ERR_NOTSUPP                        ((ReturnCode)24U) /*!< requested operation not supported */
#define ERR_WRITE                          ((ReturnCode)25U) /*!< write error */
#define ERR_FIFO                           ((ReturnCode)26U) /*!< fifo over or underflow error */
#define ERR_PAR                            ((ReturnCode)27U) /*!< parity error */
#define ERR_DONE                           ((ReturnCode)28U) /*!< transfer has already finished */
#define ERR_RF_COLLISION                   ((ReturnCode)29U) /*!< collision error (Bit Collision or during RF Collision avoidance ) */
#define ERR_HW_OVERRUN                     ((ReturnCode)30U) /*!< lost one or more received bytes */
#define ERR_RELEASE_REQ                    ((ReturnCode)31U) /*!< device requested release */
#define ERR_SLEEP_REQ                      ((ReturnCode)32U) /*!< device requested sleep */
#define ERR_WRONG_STATE                    ((ReturnCode)33U) /*!< incorrect state for requested operation */
#define ERR_MAX_RERUNS                     ((ReturnCode)34U) /*!< blocking procedure reached maximum runs */
#define ERR_DISABLED                       ((ReturnCode)35U) /*!< operation aborted due to disabled configuration */
#define ERR_HW_MISMATCH                    ((ReturnCode)36U) /*!< expected hw do not match  */
#define ERR_LINK_LOSS                      ((ReturnCode)37U) /*!< Other device's field didn't behave as expected: turned off by Initiator in Passive mode, or AP2P did not turn on field */
#define ERR_INVALID_HANDLE                 ((ReturnCode)38U) /*!< invalid or not initialized device handle */

#define ERR_INCOMPLETE_BYTE                ((ReturnCode)40U) /*!< Incomplete byte rcvd         */
#define ERR_INCOMPLETE_BYTE_01             ((ReturnCode)41U) /*!< Incomplete byte rcvd - 1 bit */
#define ERR_INCOMPLETE_BYTE_02             ((ReturnCode)42U) /*!< Incomplete byte rcvd - 2 bit */
#define ERR_INCOMPLETE_BYTE_03             ((ReturnCode)43U) /*!< Incomplete byte rcvd - 3 bit */
#define ERR_INCOMPLETE_BYTE_04             ((ReturnCode)44U) /*!< Incomplete byte rcvd - 4 bit */
#define ERR_INCOMPLETE_BYTE_05             ((ReturnCode)45U) /*!< Incomplete byte rcvd - 5 bit */
#define ERR_INCOMPLETE_BYTE_06             ((ReturnCode)46U) /*!< Incomplete byte rcvd - 6 bit */
#define ERR_INCOMPLETE_BYTE_07             ((ReturnCode)47U) /*!< Incomplete byte rcvd - 7 bit */

/* General Sub-category number */
#define ERR_GENERIC_GRP                     (0x0000)  /*!< Reserved value for generic error no */
#define ERR_WARN_GRP                        (0x0100)  /*!< Errors which are not expected in normal operation */
#define ERR_PROCESS_GRP                     (0x0200)  /*!< Processes management errors */
#define ERR_SIO_GRP                         (0x0800)  /*!< SIO errors due to logging */
#define ERR_RINGBUF_GRP                     (0x0900)  /*!< Ring Buffer errors */
#define ERR_MQ_GRP                          (0x0A00)  /*!< MQ errors */
#define ERR_TIMER_GRP                       (0x0B00)  /*!< Timer errors */
#define ERR_RFAL_GRP                        (0x0C00)  /*!< RFAL errors */
#define ERR_UART_GRP                        (0x0D00)  /*!< UART errors */
#define ERR_SPI_GRP                         (0x0E00)  /*!< SPI errors */
#define ERR_I2C_GRP                         (0x0F00)  /*!< I2c errors */


#define ERR_INSERT_SIO_GRP(x)               (ERR_SIO_GRP     | (x))  /*!< Insert the SIO grp */
#define ERR_INSERT_RINGBUF_GRP(x)           (ERR_RINGBUF_GRP | (x))  /*!< Insert the Ring Buffer grp */
#define ERR_INSERT_RFAL_GRP(x)              (ERR_RFAL_GRP    | (x))  /*!< Insert the RFAL grp */
#define ERR_INSERT_SPI_GRP(x)               (ERR_SPI_GRP     | (x))  /*!< Insert the spi grp */
#define ERR_INSERT_I2C_GRP(x)               (ERR_I2C_GRP     | (x))  /*!< Insert the i2c grp */
#define ERR_INSERT_UART_GRP(x)              (ERR_UART_GRP    | (x))  /*!< Insert the uart grp */
#define ERR_INSERT_TIMER_GRP(x)             (ERR_TIMER_GRP   | (x))  /*!< Insert the timer grp */
#define ERR_INSERT_MQ_GRP(x)                (ERR_MQ_GRP      | (x))  /*!< Insert the mq grp */
#define ERR_INSERT_PROCESS_GRP(x)           (ERR_PROCESS_GRP | (x))  /*!< Insert the process grp */
#define ERR_INSERT_WARN_GRP(x)              (ERR_WARN_GRP    | (x))  /*!< Insert the i2c grp */
#define ERR_INSERT_GENERIC_GRP(x)           (ERR_GENERIC_GRP | (x))  /*!< Insert the generic grp */


/*
******************************************************************************
* GLOBAL MACROS
******************************************************************************
*/

#define ERR_NO_MASK(x)                      ((uint16_t)(x) & 0x00FFU)    /*!< Mask the error number */

/*! Common code to exit a function with the error if function f return error */
#define EXIT_ON_ERR(r, f) \
    (r) = (f);            \
    if (ERR_NONE != (r))  \
    {                     \
        return (r);       \
    }

/*! Common code to exit a function if process/function f has not concluded */
#define EXIT_ON_BUSY(r, f) \
    (r) = (f);             \
    if (ERR_BUSY == (r))   \
    {                      \
        return (r);        \
    }



#endif /* ST_ERRNO_H */

