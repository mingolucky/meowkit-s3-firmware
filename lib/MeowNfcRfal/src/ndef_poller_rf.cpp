
/**
  ******************************************************************************
  * @file           : ndef_poller_rf.cpp
  * @brief          : Provides NDEF methods and definitions to access NFC Forum Tags
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/*
 ******************************************************************************
 * INCLUDES
 ******************************************************************************
 */
#include "ndef_poller.h"

/*
 ******************************************************************************
 * ENABLE SWITCH
 ******************************************************************************
 */

/*
 ******************************************************************************
 * GLOBAL DEFINES
 ******************************************************************************
 */

/*
 *****************************************************************************
 * GLOBAL TYPES
 ******************************************************************************
 */

/*
 ******************************************************************************
 * GLOBAL MACROS
 ******************************************************************************
 */

/*
 ******************************************************************************
 * LOCAL VARIABLES
 ******************************************************************************
 */

/*
 ******************************************************************************
 * LOCAL FUNCTION PROTOTYPES
 ******************************************************************************
 */

/*
 ******************************************************************************
 * GLOBAL FUNCTIONS
 ******************************************************************************
 */

/*******************************************************************************/
ndefDeviceType ndefGetDeviceType(const ndefDevice *dev)
{
  ndefDeviceType type = NDEF_DEV_NONE;

  if (dev != NULL) {
    switch (dev->type) {
      case RFAL_NFC_LISTEN_TYPE_NFCA:
        switch (dev->dev.nfca.type) {
          case RFAL_NFCA_T1T:
            type = NDEF_DEV_T1T;
            break;
          case RFAL_NFCA_T2T:
            type = NDEF_DEV_T2T;
            break;
          case RFAL_NFCA_T4T:
            type = NDEF_DEV_T4T;
            break;
          default:
            break;
        }
        break;
      case RFAL_NFC_LISTEN_TYPE_NFCB:
        type = NDEF_DEV_T4T;
        break;
      case RFAL_NFC_LISTEN_TYPE_NFCF:
        type = NDEF_DEV_T3T;
        break;
      case RFAL_NFC_LISTEN_TYPE_NFCV:
        type = NDEF_DEV_T5T;
        break;
      default:
        break;
    }
  }

  return type;
}
