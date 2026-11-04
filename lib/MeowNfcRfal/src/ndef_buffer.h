
/**
  ******************************************************************************
  * @file           : ndef_buffer.h
  * @brief          : NDEF buffer type structures
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


#ifndef NDEF_BUFFER_H
#define NDEF_BUFFER_H



/*
 ******************************************************************************
 * INCLUDES
 ******************************************************************************
 */

#include <stdint.h>


/*
 ******************************************************************************
 * GLOBAL DEFINES
 ******************************************************************************
 */


/*
 ******************************************************************************
 * GLOBAL TYPES
 ******************************************************************************
 */


/*! NDEF structure to handle const buffers */
typedef struct {
  const uint8_t *buffer; /*!< Pointer to const buffer */
  uint32_t       length; /*!< buffer length           */
} ndefConstBuffer;


/*! NDEF structure to handle buffers */
typedef struct {
  uint8_t *buffer; /*!< Pointer to buffer */
  uint32_t length; /*!< buffer length     */
} ndefBuffer;


/*! NDEF structure to handle const buffers limited to 256 bytes */
typedef struct {
  const uint8_t *buffer; /*!< Pointer to const buffer */
  uint8_t        length; /*!< buffer length           */
} ndefConstBuffer8;


/*! NDEF structure to handle buffers limited to 256 bytes */
typedef struct {
  uint8_t *buffer; /*!< Pointer to buffer */
  uint8_t  length; /*!< buffer length     */
} ndefBuffer8;



#endif /* NDEF_BUFFER_H */
