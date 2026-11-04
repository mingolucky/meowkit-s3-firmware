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
 *  \brief ST25R3916 Interrupt header file
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
 * \addtogroup ST25R3916_Interrupt
 * \brief RFAL ST25R3916 Interrupt
 * @{
 *
 */

#ifndef ST25R3916_INTERRUPT_H
#define ST25R3916_INTERRUPT_H

/*
******************************************************************************
* INCLUDES
******************************************************************************
*/


/*
******************************************************************************
* GLOBAL DEFINES
******************************************************************************
*/

#define ST25R3916_IRQ_MASK_ALL             (uint32_t)(0xFFFFFFFFUL)  /*!< All ST25R3916 interrupt sources                             */
#define ST25R3916_IRQ_MASK_NONE            (uint32_t)(0x00000000UL)  /*!< No ST25R3916 interrupt source                               */

/* Main interrupt register */
#define ST25R3916_IRQ_MASK_OSC             (uint32_t)(0x00000080U)   /*!< ST25R3916 oscillator stable interrupt                       */
#define ST25R3916_IRQ_MASK_FWL             (uint32_t)(0x00000040U)   /*!< ST25R3916 FIFO water level interrupt                        */
#define ST25R3916_IRQ_MASK_RXS             (uint32_t)(0x00000020U)   /*!< ST25R3916 start of receive interrupt                        */
#define ST25R3916_IRQ_MASK_RXE             (uint32_t)(0x00000010U)   /*!< ST25R3916 end of receive interrupt                          */
#define ST25R3916_IRQ_MASK_TXE             (uint32_t)(0x00000008U)   /*!< ST25R3916 end of transmission interrupt                     */
#define ST25R3916_IRQ_MASK_COL             (uint32_t)(0x00000004U)   /*!< ST25R3916 bit collision interrupt                           */
#define ST25R3916_IRQ_MASK_RX_REST         (uint32_t)(0x00000002U)   /*!< ST25R3916 automatic reception restart interrupt             */
#define ST25R3916_IRQ_MASK_RFU             (uint32_t)(0x00000001U)   /*!< ST25R3916 RFU interrupt                                     */

/* Timer and NFC interrupt register */
#define ST25R3916_IRQ_MASK_DCT             (uint32_t)(0x00008000U)   /*!< ST25R3916 termination of direct command interrupt.          */
#define ST25R3916_IRQ_MASK_NRE             (uint32_t)(0x00004000U)   /*!< ST25R3916 no-response timer expired interrupt               */
#define ST25R3916_IRQ_MASK_GPE             (uint32_t)(0x00002000U)   /*!< ST25R3916 general purpose timer expired interrupt           */
#define ST25R3916_IRQ_MASK_EON             (uint32_t)(0x00001000U)   /*!< ST25R3916 external field on interrupt                       */
#define ST25R3916_IRQ_MASK_EOF             (uint32_t)(0x00000800U)   /*!< ST25R3916 external field off interrupt                      */
#define ST25R3916_IRQ_MASK_CAC             (uint32_t)(0x00000400U)   /*!< ST25R3916 collision during RF collision avoidance interrupt */
#define ST25R3916_IRQ_MASK_CAT             (uint32_t)(0x00000200U)   /*!< ST25R3916 minimum guard time expired interrupt              */
#define ST25R3916_IRQ_MASK_NFCT            (uint32_t)(0x00000100U)   /*!< ST25R3916 initiator bit rate recognised interrupt           */

/* Error and wake-up interrupt register */
#define ST25R3916_IRQ_MASK_CRC             (uint32_t)(0x00800000U)   /*!< ST25R3916 CRC error interrupt                               */
#define ST25R3916_IRQ_MASK_PAR             (uint32_t)(0x00400000U)   /*!< ST25R3916 parity error interrupt                            */
#define ST25R3916_IRQ_MASK_ERR2            (uint32_t)(0x00200000U)   /*!< ST25R3916 soft framing error interrupt                      */
#define ST25R3916_IRQ_MASK_ERR1            (uint32_t)(0x00100000U)   /*!< ST25R3916 hard framing error interrupt                      */
#define ST25R3916_IRQ_MASK_WT              (uint32_t)(0x00080000U)   /*!< ST25R3916 wake-up interrupt                                 */
#define ST25R3916_IRQ_MASK_WAM             (uint32_t)(0x00040000U)   /*!< ST25R3916 wake-up due to amplitude interrupt                */
#define ST25R3916_IRQ_MASK_WPH             (uint32_t)(0x00020000U)   /*!< ST25R3916 wake-up due to phase interrupt                    */
#if defined(ST25R3916)
  #define ST25R3916_IRQ_MASK_WCAP            (uint32_t)(0x00010000U)   /*!< ST25R3916 wake-up due to capacitance measurement            */
#elif defined(ST25R3916B)
  #define ST25R3916_IRQ_MASK_WCAP            ST25R3916_IRQ_MASK_NONE   /*!< ST25R3916B disable capacitive WU                            */
#endif /* ST25R3916 */

/* Passive Target Interrupt Register */
#define ST25R3916_IRQ_MASK_PPON2           (uint32_t)(0x80000000U)   /*!< ST25R3916 PPON2 Field on waiting Timer interrupt            */
#define ST25R3916_IRQ_MASK_SL_WL           (uint32_t)(0x40000000U)   /*!< ST25R3916 Passive target slot number water level interrupt  */
#define ST25R3916_IRQ_MASK_APON            (uint32_t)(0x20000000U)   /*!< ST25R3916 Anticollision done and Field On interrupt         */
#define ST25R3916_IRQ_MASK_RXE_PTA         (uint32_t)(0x10000000U)   /*!< ST25R3916 RXE with an automatic response interrupt          */
#define ST25R3916_IRQ_MASK_WU_F            (uint32_t)(0x08000000U)   /*!< ST25R3916 212/424b/s Passive target interrupt: Active       */
#define ST25R3916_IRQ_MASK_RFU2            (uint32_t)(0x04000000U)   /*!< ST25R3916 RFU2 interrupt                                    */
#define ST25R3916_IRQ_MASK_WU_A_X          (uint32_t)(0x02000000U)   /*!< ST25R3916 106kb/s Passive target state interrupt: Active*   */
#define ST25R3916_IRQ_MASK_WU_A            (uint32_t)(0x01000000U)   /*!< ST25R3916 106kb/s Passive target state interrupt: Active    */


/*! Holds current and previous interrupt callback pointer as well as current Interrupt status and mask */
typedef struct {
  void (*prevCallback)(void);      /*!< call back function for ST25R3916 interrupt          */
  void (*callback)(void);          /*!< call back function for ST25R3916 interrupt          */
  uint32_t  status;                /*!< latest interrupt status                             */
  uint32_t  mask;                  /*!< Interrupt mask. Negative mask = ST25R3916 mask regs */
} st25r3916Interrupt;

/*
******************************************************************************
* GLOBAL FUNCTION PROTOTYPES
******************************************************************************
*/


#endif /* ST25R3916_ISR_H */

/**
  * @}
  *
  * @}
  *
  * @}
  *
  * @}
  */
