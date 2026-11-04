
/**
  ******************************************************************************
  * @file           : ndef_type_media.h
  * @brief          : NDEF MIME Media type header file
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

#ifndef NDEF_TYPE_MEDIA_H
#define NDEF_TYPE_MEDIA_H



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


/*! Media Type */
typedef struct {
  ndefConstBuffer8 bufType;    /*!< Media type    */
  ndefConstBuffer  bufPayload; /*!< Media payload */
} ndefTypeMedia;


/*
 ******************************************************************************
 * GLOBAL FUNCTION PROTOTYPES
 ******************************************************************************
 */


/***************
 * Media
 ***************
 */

/*!
 *****************************************************************************
 * Initialize a Media type
 *
 * \param[out] media:      Media type to initialize
 * \param[in]  bufType:    Type buffer
 * \param[in]  bufPayload: Payload buffer
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefMediaInit(ndefType *media, const ndefConstBuffer8 *bufType, const ndefConstBuffer *bufPayload);


/*!
 *****************************************************************************
 * Get Media type content
 *
 * \param[in]  media:      Media type to get information from
 * \param[out] bufType:    Type buffer
 * \param[out] bufPayload: Payload buffer
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefGetMedia(const ndefType *media, ndefConstBuffer8 *bufType, ndefConstBuffer *bufPayload);


/*!
 *****************************************************************************
 * Convert an NDEF record to a Media type
 *
 * \param[in]  record: Record to convert
 * \param[out] media:  The converted Media type
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRecordToMedia(const ndefRecord *record, ndefType *media);


/*!
 *****************************************************************************
 * Convert a Media type to an NDEF record
 *
 * \param[in]  media:  Type to convert
 * \param[out] record: The converted type
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefMediaToRecord(const ndefType *media, ndefRecord *record);



#endif /* NDEF_TYPE_MEDIA_H */

/**
  * @}
  *
  */
