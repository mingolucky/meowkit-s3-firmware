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
 *  \brief ST25R3916 Interrupt handling
 *
 */

/*
******************************************************************************
* INCLUDES
******************************************************************************
*/
#include "rfal_rfst25r3916.h"
#include "st25r3916_interrupt.h"
#include "st25r3916_com.h"
#include "st25r3916.h"
#include "st_errno.h"
#include "nfc_utils.h"

/*
******************************************************************************
* GLOBAL DEFINES
******************************************************************************
*/

/*! Length of the interrupt registers       */
#define ST25R3916_INT_REGS_LEN          ( (ST25R3916_REG_IRQ_TARGET - ST25R3916_REG_IRQ_MAIN) + 1U )

#ifdef ST25R3916_SHARED_IRQ_LINE
  /*! Upper bound on IRQ-register reads per st25r3916Isr() call.  Only reached
  *  when another device on the shared IRQ line is holding it asserted, which
  *  this IC cannot clear.  Rationale in st25r3916CheckForReceivedInterrupts(). */
  #ifndef ST25R3916_ISR_MAX_REG_READS
    #define ST25R3916_ISR_MAX_REG_READS     8U
  #endif

  /*! Interval at which st25r3916WaitForInterruptsTimed() polls the IRQ registers
  *  instead of trusting a pin edge to have filled in the interrupt status. */
  #ifndef ST25R3916_IRQ_POLL_INTERVAL_US
    #define ST25R3916_IRQ_POLL_INTERVAL_US  200U
  #endif
#endif /* ST25R3916_SHARED_IRQ_LINE */

/*
 ******************************************************************************
 * LOCAL DATA TYPES
 ******************************************************************************
 */


/*
******************************************************************************
* GLOBAL VARIABLES
******************************************************************************
*/


/*
******************************************************************************
* GLOBAL FUNCTIONS
******************************************************************************
*/
void RfalRfST25R3916Class::st25r3916InitInterrupts(void)
{
  st25r3916interrupt.callback     = NULL;
  st25r3916interrupt.prevCallback = NULL;
  st25r3916interrupt.status       = ST25R3916_IRQ_MASK_NONE;
  st25r3916interrupt.mask         = ST25R3916_IRQ_MASK_NONE;
}


/*******************************************************************************/
void RfalRfST25R3916Class::st25r3916Isr(void)
{
  st25r3916CheckForReceivedInterrupts();

  // Check if callback is set and run it
  if (NULL != st25r3916interrupt.callback) {
    st25r3916interrupt.callback();
  }
}


/*******************************************************************************/
void RfalRfST25R3916Class::st25r3916CheckForReceivedInterrupts(void)
{
  // M5Stack's I2C implementation reads ERROR_WUP before MAIN: reading MAIN
  // can clear error status on this interface. Preserve RFAL's byte ordering
  // and latch every successful read, even if a later transaction fails.
  uint32_t irqStatus = ST25R3916_IRQ_MASK_NONE;
  unsigned reads = 0;
  do {
    uint8_t iregs[ST25R3916_INT_REGS_LEN] = {};
    if (i2c_enabled) {
      if (st25r3916ReadRegister(ST25R3916_REG_IRQ_ERROR_WUP, &iregs[2]) != ERR_NONE) break;
      irqStatus |= (uint32_t)iregs[2] << 16;
      if (st25r3916ReadMultipleRegisters(ST25R3916_REG_IRQ_MAIN, iregs, 2) != ERR_NONE) break;
      irqStatus |= (uint32_t)iregs[0] | ((uint32_t)iregs[1] << 8);
      if (st25r3916ReadRegister(ST25R3916_REG_IRQ_TARGET, &iregs[3]) != ERR_NONE) break;
      irqStatus |= (uint32_t)iregs[3] << 24;
    } else {
      if (st25r3916ReadMultipleRegisters(ST25R3916_REG_IRQ_MAIN, iregs,
                                       ST25R3916_INT_REGS_LEN) != ERR_NONE) break;
      irqStatus |= (uint32_t)iregs[0] | ((uint32_t)iregs[1] << 8) |
                   ((uint32_t)iregs[2] << 16) | ((uint32_t)iregs[3] << 24);
    }
  } while (int_pin >= 0 && digitalRead(int_pin) == HIGH && ++reads < 8);
  st25r3916interrupt.status |= irqStatus;
}


/*******************************************************************************/
void RfalRfST25R3916Class::st25r3916ModifyInterrupts(uint32_t clr_mask, uint32_t set_mask)
{
  uint8_t  i;
  uint32_t old_mask;
  uint32_t new_mask;


  old_mask = st25r3916interrupt.mask;
  new_mask = ((~old_mask & set_mask) | (old_mask & clr_mask));
  st25r3916interrupt.mask &= ~clr_mask;
  st25r3916interrupt.mask |= set_mask;

  for (i = 0; i < ST25R3916_INT_REGS_LEN; i++) {
    if (((new_mask >> (8U * i)) & 0xFFU) == 0U) {
      continue;
    }

    st25r3916WriteRegister(ST25R3916_REG_IRQ_MASK_MAIN + i, (uint8_t)((st25r3916interrupt.mask >> (8U * i)) & 0xFFU));
  }
  return;
}


/*******************************************************************************/
uint32_t RfalRfST25R3916Class::st25r3916WaitForInterruptsTimed(uint32_t mask, uint16_t tmo)
{
  // A disconnected module must never trap the application in an endless wait.
  const uint32_t deadline = timerCalculateTimer(tmo ? tmo : 1000);
  uint32_t status;
  do {
    st25r3916CheckForReceivedInterrupts();
    status = st25r3916interrupt.status & mask;
    if (i2c_enabled && !dev_i2c->ok()) break;
    if (!status) delay(1);
  } while (!status && !timerIsExpired(deadline));
  st25r3916interrupt.status &= ~status;
  return status;
}


/*******************************************************************************/
uint32_t RfalRfST25R3916Class::st25r3916GetInterrupt(uint32_t mask)
{
  if (int_pin < 0) st25r3916CheckForReceivedInterrupts();
  uint32_t irqs;

  irqs = (st25r3916interrupt.status & mask);
  if (irqs != ST25R3916_IRQ_MASK_NONE) {
    st25r3916interrupt.status &= ~irqs;
  }

  return irqs;
}


/*******************************************************************************/
void RfalRfST25R3916Class::st25r3916ClearAndEnableInterrupts(uint32_t mask)
{
  st25r3916GetInterrupt(mask);
  st25r3916EnableInterrupts(mask);
}


/*******************************************************************************/
void RfalRfST25R3916Class::st25r3916EnableInterrupts(uint32_t mask)
{
  st25r3916ModifyInterrupts(mask, 0);
}


/*******************************************************************************/
void RfalRfST25R3916Class::st25r3916DisableInterrupts(uint32_t mask)
{
  st25r3916ModifyInterrupts(0, mask);
}

/*******************************************************************************/
void RfalRfST25R3916Class::st25r3916ClearInterrupts(void)
{
  uint8_t iregs[ST25R3916_INT_REGS_LEN];

  st25r3916ReadMultipleRegisters(ST25R3916_REG_IRQ_MAIN, iregs, ST25R3916_INT_REGS_LEN);

  st25r3916interrupt.status = ST25R3916_IRQ_MASK_NONE;

  return;
}

/*******************************************************************************/
void RfalRfST25R3916Class::st25r3916IRQCallbackSet(void (*cb)(void))
{
  st25r3916interrupt.prevCallback = st25r3916interrupt.callback;
  st25r3916interrupt.callback     = cb;
}

/*******************************************************************************/
void RfalRfST25R3916Class::st25r3916IRQCallbackRestore(void)
{
  st25r3916interrupt.callback     = st25r3916interrupt.prevCallback;
  st25r3916interrupt.prevCallback = NULL;
}
