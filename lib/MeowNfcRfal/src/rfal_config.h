
/********************************************************************************
  * @file    rfal_config.h
  * @author  MMY Application Team
  * @brief   RF Abstraction Layer (RFAL) Config file
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


#ifndef RFAL_CONFIG_H
#define RFAL_CONFIG_H


/*
******************************************************************************
* INCLUDES
******************************************************************************
*/

#if __has_include("rfal_custom_config.h")
  #include "rfal_custom_config.h"
#else
  #include "rfal_default_config.h"
#endif

#endif  /* RFAL_CONFIG_H */


/**
  * @}
  *
  */
