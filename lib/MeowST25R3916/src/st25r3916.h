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
 *  \brief ST25R3916 declaration file
 *
 * API:
 * - Initialize ST25R3916 driver: #ST25R3916Initialize
 * - Deinitialize ST25R3916 driver: #ST25R3916Deinitialize
 *
 *
 * \addtogroup RFAL
 * @{
 *
 * \addtogroup RFAL-HAL
 * \brief RFAL Hardware Abstraction Layer
 * @{
 *
 * \addtogroup ST25R3916
 * \brief RFAL ST25R3916 Driver
 * @{
 *
 * \addtogroup ST25R3916_Driver
 * \brief RFAL ST25R3916 Driver
 * @{
 *
 */

#ifndef ST25R3916_H
#define ST25R3916_H
/*
******************************************************************************
* INCLUDES
******************************************************************************
*/
#include "st_errno.h"
#include "st25r3916_com.h"

/*
******************************************************************************
* GLOBAL DATATYPES
******************************************************************************
*/

/*! Struct to represent all regs on ST25R3916                                                             */
typedef struct {
  uint8_t RsA[(ST25R3916_REG_IC_IDENTITY + 1U)]; /*!< Registers contained on ST25R3916 space A (Rs-A)     */
  uint8_t RsB[ST25R3916_SPACE_B_REG_LEN];      /*!< Registers contained on ST25R3916 space B (Rs-B)     */
} t_st25r3916Regs;

/*! Parameters how the stream mode should work                                                            */
struct st25r3916StreamConfig {
  uint8_t useBPSK;                            /*!< 0: subcarrier, 1:BPSK                                */
  uint8_t din;                                /*!< Divider for the in subcarrier frequency: fc/2^din    */
  uint8_t dout;                               /*!< Divider for the in subcarrier frequency fc/2^dout    */
  uint8_t report_period_length;               /*!< Length of the reporting period 2^report_period_length*/
};


/*
******************************************************************************
* GLOBAL DEFINES
******************************************************************************
*/
/* ST25R3916 direct commands */
#define ST25R3916_CMD_SET_DEFAULT              0xC1U    /*!< Puts the chip in default state (same as after power-up) */
#define ST25R3916_CMD_STOP                     0xC2U    /*!< Stops all activities and clears FIFO                    */
#define ST25R3916_CMD_TRANSMIT_WITH_CRC        0xC4U    /*!< Transmit with CRC                                       */
#define ST25R3916_CMD_TRANSMIT_WITHOUT_CRC     0xC5U    /*!< Transmit without CRC                                    */
#define ST25R3916_CMD_TRANSMIT_REQA            0xC6U    /*!< Transmit REQA                                           */
#define ST25R3916_CMD_TRANSMIT_WUPA            0xC7U    /*!< Transmit WUPA                                           */
#define ST25R3916_CMD_INITIAL_RF_COLLISION     0xC8U    /*!< NFC transmit with Initial RF Collision Avoidance        */
#define ST25R3916_CMD_RESPONSE_RF_COLLISION_N  0xC9U    /*!< NFC transmit with Response RF Collision Avoidance       */
#define ST25R3916_CMD_GOTO_SENSE               0xCDU    /*!< Passive target logic to Sense/Idle state                */
#define ST25R3916_CMD_GOTO_SLEEP               0xCEU    /*!< Passive target logic to Sleep/Halt state                */
#define ST25R3916_CMD_MASK_RECEIVE_DATA        0xD0U    /*!< Mask receive data                                       */
#define ST25R3916_CMD_UNMASK_RECEIVE_DATA      0xD1U    /*!< Unmask receive data                                     */
#define ST25R3916_CMD_AM_MOD_STATE_CHANGE      0xD2U    /*!< AM Modulation state change                              */
#define ST25R3916_CMD_MEASURE_AMPLITUDE        0xD3U    /*!< Measure signal amplitude on RFI inputs                  */
#define ST25R3916_CMD_RESET_RXGAIN             0xD5U    /*!< Reset RX Gain                                           */
#define ST25R3916_CMD_ADJUST_REGULATORS        0xD6U    /*!< Adjust regulators                                       */
#define ST25R3916_CMD_CALIBRATE_DRIVER_TIMING  0xD8U    /*!< Starts the sequence to adjust the driver timing         */
#define ST25R3916_CMD_MEASURE_PHASE            0xD9U    /*!< Measure phase between RFO and RFI signal                */
#define ST25R3916_CMD_CLEAR_RSSI               0xDAU    /*!< Clear RSSI bits and restart the measurement             */
#define ST25R3916_CMD_CLEAR_FIFO               0xDBU    /*!< Clears FIFO, Collision and IRQ status                   */
#define ST25R3916_CMD_TRANSPARENT_MODE         0xDCU    /*!< Transparent mode                                        */
#ifdef ST25R3916
  #define ST25R3916_CMD_CALIBRATE_C_SENSOR       0xDDU    /*!< Calibrate the capacitive sensor                         */
  #define ST25R3916_CMD_MEASURE_CAPACITANCE      0xDEU    /*!< Measure capacitance                                     */
#endif /* ST25R3916 */
#define ST25R3916_CMD_MEASURE_VDD              0xDFU    /*!< Measure power supply voltage                            */
#define ST25R3916_CMD_START_GP_TIMER           0xE0U    /*!< Start the general purpose timer                         */
#define ST25R3916_CMD_START_WUP_TIMER          0xE1U    /*!< Start the wake-up timer                                 */
#define ST25R3916_CMD_START_MASK_RECEIVE_TIMER 0xE2U    /*!< Start the mask-receive timer                            */
#define ST25R3916_CMD_START_NO_RESPONSE_TIMER  0xE3U    /*!< Start the no-response timer                             */
#define ST25R3916_CMD_START_PPON2_TIMER        0xE4U    /*!< Start PPon2 timer                                       */
#define ST25R3916_CMD_STOP_NRT                 0xE8U    /*!< Stop No Response Timer                                  */
#ifdef ST25R3916B
  #define ST25R3916_CMD_RC_CAL                   0xEAU    /*!< Trigger RC calibration                                  */
#endif /* ST25R3916B */
#define ST25R3916_CMD_SPACE_B_ACCESS           0xFBU    /*!< Enable R/W access to the test registers                 */
#define ST25R3916_CMD_TEST_ACCESS              0xFCU    /*!< Enable R/W access to the test registers                 */


#define ST25R3916_THRESHOLD_DO_NOT_SET         0xFFU    /*!< Indicates not to change this Threshold                  */

#define ST25R3916_BR_DO_NOT_SET                0xFFU    /*!< Indicates not to change this Bit Rate                   */
#define ST25R3916_BR_106                       0x00U    /*!< ST25R3916 Bit Rate  106 kbit/s (fc/128)                 */
#define ST25R3916_BR_212                       0x01U    /*!< ST25R3916 Bit Rate  212 kbit/s (fc/64)                  */
#define ST25R3916_BR_424                       0x02U    /*!< ST25R3916 Bit Rate  424 kbit/s (fc/32)                  */
#define ST25R3916_BR_848                       0x03U    /*!< ST25R3916 Bit Rate  848 kbit/s (fc/16)                  */
#define ST25R3916_BR_1695                      0x04U    /*!< ST25R3916 Bit Rate 1696 kbit/s (fc/8)                   */
#define ST25R3916_BR_3390                      0x05U    /*!< ST25R3916 Bit Rate 3390 kbit/s (fc/4)                   */
#define ST25R3916_BR_6780                      0x07U    /*!< ST25R3916 Bit Rate 6780 kbit/s (fc/2)                   */

#define ST25R3916_FIFO_DEPTH                   512U     /*!< Depth of FIFO                                           */
#define ST25R3916_TOUT_OSC_STABLE              10U      /*!< Max timeout for Oscillator to get stable      DS: 700us */


/*
******************************************************************************
* GLOBAL MACROS
******************************************************************************
*/

/*! Enables the Transmitter (Field On) and Receiver                                          */
#define st25r3916TxRxOn()             st25r3916SetRegisterBits( ST25R3916_REG_OP_CONTROL, (ST25R3916_REG_OP_CONTROL_rx_en | ST25R3916_REG_OP_CONTROL_tx_en ) )

/*! Disables the Transmitter (Field Off) and Receiver                                         */
#define st25r3916TxRxOff()            st25r3916ClrRegisterBits( ST25R3916_REG_OP_CONTROL, (ST25R3916_REG_OP_CONTROL_rx_en | ST25R3916_REG_OP_CONTROL_tx_en ) )

/*! Disables the Transmitter (Field Off)                                         */
#define st25r3916TxOff()              st25r3916ClrRegisterBits( ST25R3916_REG_OP_CONTROL, ST25R3916_REG_OP_CONTROL_tx_en )

/*! Checks if General Purpose Timer is still running by reading gpt_on flag                  */
#define st25r3916IsGPTRunning( )      st25r3916CheckReg( ST25R3916_REG_NFCIP1_BIT_RATE, ST25R3916_REG_NFCIP1_BIT_RATE_gpt_on, ST25R3916_REG_NFCIP1_BIT_RATE_gpt_on )

/*! Checks if External Filed is detected by reading ST25R3916 External Field Detector output    */
#define st25r3916IsExtFieldOn()       st25r3916CheckReg( ST25R3916_REG_AUX_DISPLAY, ST25R3916_REG_AUX_DISPLAY_efd_o, ST25R3916_REG_AUX_DISPLAY_efd_o )

/*! Checks if Transmitter is enabled (Field On) */
#define st25r3916IsTxEnabled()        st25r3916CheckReg( ST25R3916_REG_OP_CONTROL, ST25R3916_REG_OP_CONTROL_tx_en, ST25R3916_REG_OP_CONTROL_tx_en )

/*! Checks if NRT is in EMV mode */
#define st25r3916IsNRTinEMV()         st25r3916CheckReg( ST25R3916_REG_TIMER_EMV_CONTROL, ST25R3916_REG_TIMER_EMV_CONTROL_nrt_emv, ST25R3916_REG_TIMER_EMV_CONTROL_nrt_emv_on )

/*! Checks if last FIFO byte is complete */
#define st25r3916IsLastFIFOComplete() st25r3916CheckReg( ST25R3916_REG_FIFO_STATUS2, ST25R3916_REG_FIFO_STATUS2_fifo_lb_mask, 0 )

/*! Checks if the Oscillator is enabled  */
#define st25r3916IsOscOn()            st25r3916CheckReg( ST25R3916_REG_OP_CONTROL, ST25R3916_REG_OP_CONTROL_en, ST25R3916_REG_OP_CONTROL_en )

/*! Checks if the AAT is enabled  */
#define st25r3916IsAATOn()            st25r3916CheckReg( ST25R3916_REG_IO_CONF2, ST25R3916_REG_IO_CONF2_aat_en, ST25R3916_REG_IO_CONF2_aat_en )

/*
******************************************************************************
* GLOBAL FUNCTION PROTOTYPES
******************************************************************************
*/


#endif /* ST25R3916_H */

/**
  * @}
  *
  * @}
  *
  * @}
  *
  * @}
  */

