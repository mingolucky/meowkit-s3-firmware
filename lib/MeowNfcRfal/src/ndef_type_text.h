
/**
  ******************************************************************************
  * @file           : ndef_type_text.h
  * @brief          : NDEF RTD Text type header file
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

#ifndef NDEF_TYPE_RTD_TEXT_H
#define NDEF_TYPE_RTD_TEXT_H



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

/*! RTD Text Record Type buffer */
extern const ndefConstBuffer8 bufRtdTypeText;       /*! Text Record Type buffer                             */


/*! RTD Type Text Encoding */
#define TEXT_ENCODING_UTF8               0U    /*!< UTF8  text encoding           */
#define TEXT_ENCODING_UTF16              1U    /*!< UTF16 text encoding           */

#define NDEF_TEXT_ENCODING_MASK       0x80U    /*!< Text encoding mask            */
#define NDEF_TEXT_ENCODING_SHIFT         7U    /*!< Text encoding bit shift       */


/*! RTD Type Text */
typedef struct {
  uint8_t          status;          /*!< Status byte                   */
  ndefConstBuffer8 bufLanguageCode; /*!< ISO/IANA language code buffer */
  ndefConstBuffer  bufSentence;     /*!< Sentence buffer               */
} ndefTypeRtdText;


/*
 ******************************************************************************
 * GLOBAL FUNCTION PROTOTYPES
 ******************************************************************************
 */


/***************
 * Text
 ***************
 */


/*!
 *****************************************************************************
 * Initialize a Text RTD type
 *
 * \param[out] text:            Type to initialize
 * \param[out] utfEncoding:     UTF-8/UTF-16
 * \param[in]  bufLanguageCode: ISO/IANA language code buffer
 * \param[in]  bufSentence:     Actual text buffer
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRtdTextInit(ndefType *text, uint8_t utfEncoding, const ndefConstBuffer8 *bufLanguageCode, const ndefConstBuffer *bufSentence);


/*!
 *****************************************************************************
 * Get RTD Text type content
 *
 * \param[in]  text:            Type to get information from
 * \param[out] utfEncoding:     UTF-8/UTF-16
 * \param[out] bufLanguageCode: ISO/IANA language code buffer
 * \param[out] bufSentence:     Actual text buffer
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefGetRtdText(const ndefType *text, uint8_t *utfEncoding, ndefConstBuffer8 *bufLanguageCode, ndefConstBuffer *bufSentence);


/*!
 *****************************************************************************
 * Convert an NDEF record to a Text type
 *
 * \param[in]  record: Record to convert
 * \param[out] text:   The converted type
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRecordToRtdText(const ndefRecord *record, ndefType *text);


/*!
 *****************************************************************************
 * Convert a Text RTD type to an NDEF record
 *
 * \param[in]  text:   Type to convert
 * \param[out] record: The converted type
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRtdTextToRecord(const ndefType *text, ndefRecord *record);



#endif /* NDEF_TYPE_RTD_TEXT_H */

/**
  * @}
  *
  */
