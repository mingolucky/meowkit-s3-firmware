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
 *  \brief SW Timer implementation
 *
 *
 *   This module makes use of a System Tick in millisconds and provides
 *   an abstraction for SW timers
 *
 */

/*
******************************************************************************
* INCLUDES
******************************************************************************
*/
#include "rfal_rfst25r3916.h"


/*
******************************************************************************
* LOCAL DEFINES
******************************************************************************
*/

/*
******************************************************************************
* LOCAL VARIABLES
******************************************************************************
*/

/*
******************************************************************************
* GLOBAL FUNCTIONS
******************************************************************************
*/


/*******************************************************************************/
uint32_t RfalRfST25R3916Class::timerCalculateTimer(uint16_t time)
{
  return (millis() + time);
}


/*******************************************************************************/
bool RfalRfST25R3916Class::timerIsExpired(uint32_t timer)
{
  uint32_t uDiff;
  int32_t sDiff;

  uDiff = (timer - millis());   /* Calculate the diff between the timers */
  sDiff = uDiff;                            /* Convert the diff to a signed var      */
  /* Having done this has two side effects:
   * 1) all differences smaller than -(2^31) ms (~25d) will become positive
   *    Signaling not expired: acceptable!
   * 2) Time roll-over case will be handled correctly: super!
   */

  /* Check if the given timer has expired already */
  if (sDiff < 0) {
    return true;
  }

  return false;
}


/*******************************************************************************/
void RfalRfST25R3916Class::timerDelay(uint16_t tOut)
{
  uint32_t t;

  /* Calculate the timer and wait blocking until is running */
  t = timerCalculateTimer(tOut);
  while (!timerIsExpired(t)) delay(1);
}


/*******************************************************************************/
void RfalRfST25R3916Class::timerStopwatchStart(void)
{
  timerStopwatchTick = millis();
}


/*******************************************************************************/
uint32_t RfalRfST25R3916Class::timerStopwatchMeasure(void)
{
  return (uint32_t)(millis() - timerStopwatchTick);
}

