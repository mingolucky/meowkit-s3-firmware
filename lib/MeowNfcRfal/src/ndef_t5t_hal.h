
/**
  ******************************************************************************
  * @file           : ndef_t5t_hal.h
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


#ifndef NDEF_T5T_HAL_H
#define NDEF_T5T_HAL_H



/*
 ******************************************************************************
 * INCLUDES
 ******************************************************************************
 */

#include "ndef_config.h"
#include "ndef_poller.h"
#include "ndef_t5t.h"


/*
 ******************************************************************************
 * GLOBAL DEFINES
 ******************************************************************************
 */


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


/*
 ******************************************************************************
 * GLOBAL FUNCTION PROTOTYPES
 ******************************************************************************
 */


/*!
 *****************************************************************************
 * \brief Return whether the device type is a STMicroelectronics device
 *
 * \param[in] dev: ndef Device
 *
 * \return true if it is a STMicroelectronics device
 *****************************************************************************
 */
bool ndefT5TisSTDevice(const ndefDevice *dev);


/*!
 *****************************************************************************
 * \brief Return whether the device type is a T5T device
 *
 * \param[in] dev: ndef Device
 *
 * \return true if it is a T5T device
 *****************************************************************************
 */
bool ndefT5TisT5TDevice(const ndefDevice *dev);


/*!
 *****************************************************************************
 * \brief Set T5T device access mode
 *
 * \param[in] ctx: ndef Context
 * \param[in] dev: ndef Device
 * \param[in] mode: Access mode
 *
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode ndefT5TPollerAccessMode(ndefContext *ctx, const ndefDevice *dev, ndefT5TAccessMode mode);


/*!
 *****************************************************************************
 * \brief Get Block length
 *
 * This function returns the block length, in bytes
 *
 * \param[in] ctx: ndef Context
 *
 * \return 0 if invalid parameter or the block length
 *****************************************************************************
 */
uint8_t ndefT5TGetBlockLength(ndefContext *ctx);


/*!
 *****************************************************************************
 * \brief Get the memory configuration
 *
 * This function provides the number of blocks and the block size in the
 * T5T system information structure.
 *
 * \param[in] ctx: ndef Context
 *
 * \return ERR_PARAM: Invalid parameter
 * \return ERR_NONE : No error
 *****************************************************************************
 */
ReturnCode ndefT5TGetMemoryConfig(ndefContext *ctx);


/*!
 *****************************************************************************
 * \brief Return whether the device supports Multiple block read
 *
 * \param[in] ctx: ndef Context
 *
 * \return true or false
 *****************************************************************************
 */
bool ndefT5TIsMultipleBlockReadSupported(ndefContext *ctx);


/*!
 *****************************************************************************
 * \brief Check Presence
 *
 * This method checks whether an NFC tag is still present in the operating field
 *
 * \param[in] ctx : ndef Context
 *
 * \return ERR_WRONG_STATE  : Library not initialized or mode not set
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_PROTO        : Protocol error
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode ndefT5TIsDevicePresent(ndefContext *ctx);


/*!
 *****************************************************************************
 * \brief This function locks the device
 *
 * \param[in] ctx: ndef Context
 *
 * \return ERR_PARAM      : Invalid parameter
 * \return ERR_NONE       : No error
 *****************************************************************************
 */
ReturnCode ndefT5TLockDevice(ndefContext *ctx);



#endif /* NDEF_T5T_HAL_H */

/**
  * @}
  *
  */
