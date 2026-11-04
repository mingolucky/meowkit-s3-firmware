
/**
  ******************************************************************************
  * @file           : ndef_config.h
  * @brief          : NDEF config header file
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


#ifndef NDEF_CONFIG_H
#define NDEF_CONFIG_H



/*
 ******************************************************************************
 * INCLUDES
 ******************************************************************************
 */

#include <stdint.h>
#include <stdbool.h>
#include <st_errno.h>
#if __has_include("ndef_custom_config.h")
  #include "ndef_custom_config.h"
#else
  #include "ndef_default_config.h"
#endif



#endif /* NDEF_CONFIG_H */
