
/**
  ******************************************************************************
  * @file           : ndef_message.h
  * @brief          : NDEF message header file
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


#ifndef NDEF_MESSAGE_H
#define NDEF_MESSAGE_H



/*
 ******************************************************************************
 * INCLUDES
 ******************************************************************************
 */

#include "ndef_record.h"
#include "ndef_config.h"


/*
 ******************************************************************************
 * GLOBAL DEFINES
 ******************************************************************************
 */

/*! Message scanning macros */
#define ndefMessageGetFirstRecord(message)    (((message) == NULL) ? NULL : (message)->record)  /*!< Get first record */
#define ndefMessageGetNextRecord(record)      (((record)  == NULL) ? NULL : (record)->next)     /*!< Get next record  */

/*
 ******************************************************************************
 * GLOBAL TYPES
 ******************************************************************************
 */


/*! Message information */
typedef struct {
  uint32_t length;      /*!< Message length in bytes          */
  uint32_t recordCount; /*!< Number of records in the message */
} ndefMessageInfo;


/*! NDEF message */
struct ndefMessageStruct {
  ndefRecord     *record; /*!< Pointer to a record */
  ndefMessageInfo info;   /*!< Message information, e.g. length in bytes, record count */
};


/*
 ******************************************************************************
 * GLOBAL FUNCTION PROTOTYPES
 ******************************************************************************
 */


/*!
 *****************************************************************************
 * Initialize an empty NDEF message
 *
 * \param[in,out] message to initialize
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefMessageInit(ndefMessage *message);


/*!
 *****************************************************************************
 * Get NDEF message information
 *
 * Return the message information
 *
 * \param[in]  message to get info from
 * \param[out] info: e.g. message length in bytes, number of records
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefMessageGetInfo(const ndefMessage *message, ndefMessageInfo *info);


/*!
 *****************************************************************************
 * Get the number of NDEF message records
 *
 * Return the number of records in the given message
 *
 * \param[in] message
 *
 * \return number of records
 *****************************************************************************
 */
uint32_t ndefMessageGetRecordCount(const ndefMessage *message);


/*!
 *****************************************************************************
 * Append a record to an NDEF message
 *
 * \param[in]     record:  Record to append
 * \param[in,out] message: Message to be appended with the given record
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefMessageAppend(ndefMessage *message, ndefRecord *record);


/*!
 *****************************************************************************
 * Decode a raw buffer to an NDEF message
 *
 * Convert a raw buffer to a message
 *
 * \param[in]  bufPayload: Payload buffer to convert into message
 * \param[out] message:    Message created from the raw buffer
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefMessageDecode(const ndefConstBuffer *bufPayload, ndefMessage *message);


#if NDEF_FEATURE_FULL_API
  /*!
  *****************************************************************************
  * Encode an NDEF message to a raw buffer
  *
  * Convert a message to a raw buffer
  *
  * \param[in]     message:    Message to convert
  * \param[in,out] bufPayload: Output buffer to store the converted message
  *                            The input length provides the output buffer allocated
  *                            length, used for parameter check to avoid overflow.
  *                            In case the buffer provided is too short, it is
  *                            updated with the required buffer length.
  *                            On success, it is updated with the actual buffer
  *                            length used to contain the converted message.
  *
  * \return ERR_NONE if successful or a standard error code
  *****************************************************************************
  */
  ReturnCode ndefMessageEncode(const ndefMessage *message, ndefBuffer *bufPayload);
#endif


#if NDEF_FEATURE_FULL_API
  /*!
  *****************************************************************************
  * Look for a given record type in an NDEF message
  *
  * Parses an NDEF message, looking for a record of given type
  *
  * \param[in] message: Message to parse
  * \param[in] tnf:     TNF type to match
  * \param[in] bufType: Type buffer to match
  *
  * \return the record matching the type if successful or NULL
  *****************************************************************************
  */
  ndefRecord *ndefMessageFindRecordType(ndefMessage *message, uint8_t tnf, const ndefConstBuffer8 *bufType);
#endif



#endif /* NDEF_MESSAGE_H */

/**
  * @}
  *
  */
