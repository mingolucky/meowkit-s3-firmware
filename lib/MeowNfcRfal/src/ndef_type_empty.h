
/**
  ******************************************************************************
  * @file           : ndef_type_empty.h
  * @brief          : NDEF Empty type header file
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


#ifndef NDEF_TYPE_EMPTY_H
#define NDEF_TYPE_EMPTY_H



/*
 ******************************************************************************
 * INCLUDES
 ******************************************************************************
 */

#include "ndef_record.h"
#include "ndef_buffer.h"


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


/*
 ******************************************************************************
 * GLOBAL FUNCTION PROTOTYPES
 ******************************************************************************
 */


/***************
 * Empty type
 ***************
 */

/*!
 *****************************************************************************
 * Initialize an Empty type
 *
 * \param[out] empty: Type to initialize
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefEmptyTypeInit(ndefType *empty);


/*!
 *****************************************************************************
 * Convert an NDEF record to an Empty type
 *
 * \param[in]  record: Record to convert
 * \param[out] empty:  The converted type
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRecordToEmptyType(const ndefRecord *record, ndefType *empty);


/*!
 *****************************************************************************
 * Convert an Empty type to an NDEF record
 *
 * \param[in]  empty:  Type to convert
 * \param[out] record: The converted type
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefEmptyTypeToRecord(const ndefType *empty, ndefRecord *record);



#endif /* NDEF_TYPE_EMPTY_H */

/**
  * @}
  *
  */
