
/**
  ******************************************************************************
  * @file           : ndef_type_flat.h
  * @brief          : NDEF Flat payload type header file
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


#ifndef NDEF_TYPE_FLAT_H
#define NDEF_TYPE_FLAT_H



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
 * Flat type
 ***************
 */

/*!
 *****************************************************************************
 * Initialize a flat payload type
 *
 * \param[out] type:       Type to initialize
 * \param[in]  bufPayload: Payload buffer
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefFlatPayloadTypeInit(ndefType *type, const ndefConstBuffer *bufPayload);


/*!
 *****************************************************************************
 * Initialize a flat payload type
 *
 * \param[out] type:       Type to get data from
 * \param[in]  bufPayload: Payload buffer
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefGetFlatPayloadType(const ndefType *type, ndefConstBuffer *bufPayload);


/*!
 *****************************************************************************
 * Convert an NDEF record to a flat payload type
 *
 * \param[in]  record: Record to convert
 * \param[out] type:  The converted type
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRecordToFlatPayloadType(const ndefRecord *record, ndefType *type);


/*!
 *****************************************************************************
 * Convert a flat payload type to an NDEF record
 *
 * \param[in]  type:   Type to convert
 * \param[out] record: The converted type
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefFlatPayloadTypeToRecord(const ndefType *type, ndefRecord *record);



#endif /* NDEF_TYPE_FLAT_H */

/**
  * @}
  *
  */
