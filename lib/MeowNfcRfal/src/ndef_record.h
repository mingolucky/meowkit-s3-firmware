
/**
  ******************************************************************************
  * @file           : ndef_record.h
  * @brief          : NDEF record header file
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


#ifndef NDEF_RECORD_H
#define NDEF_RECORD_H



/*
 ******************************************************************************
 * INCLUDES
 ******************************************************************************
 */

#include "ndef_config.h"
#include "ndef_buffer.h"


/*
 ******************************************************************************
 * GLOBAL DEFINES
 ******************************************************************************
 */

#define NDEF_RECORD_HEADER_LEN       7U    /*!< Record header length (sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint8_t) */

#define NDEF_SHORT_RECORD_LENGTH_MAX 255U  /*!< Short record maximum length */


/*! Type Name Format aka TNF types */
#define NDEF_TNF_EMPTY               0U    /*!< TNF Empty             */
#define NDEF_TNF_RTD_WELL_KNOWN_TYPE 1U    /*!< TNF Well-known Type   */
#define NDEF_TNF_MEDIA_TYPE          2U    /*!< TNF Media Type        */
#define NDEF_TNF_URI                 3U    /*!< TNF URI               */
#define NDEF_TNF_RTD_EXTERNAL_TYPE   4U    /*!< TNF External Type     */
#define NDEF_TNF_UNKNOWN             5U    /*!< TNF Unknown           */
#define NDEF_TNF_UNCHANGED           6U    /*!< TNF Unchanged         */
#define NDEF_TNF_RESERVED            7U    /*!< TNF Reserved          */

#define NDEF_TNF_MASK                7U    /*!< Type Name Format mask */


/*
 ******************************************************************************
 * GLOBAL TYPES
 ******************************************************************************
 */

/*! Build the record header byte, made of MB, ME, CF, SR, IL bits and TNF type */
#define ndefHeader(MB, ME, CF, SR, IL, TNF)  ((((MB) & 1U) << 7U) | (((ME) & 1U) << 6U) | (((CF) & 1U) << 5U) | (((SR) & 1U) << 4U) | (((IL) & 1U) << 3U) | ((uint8_t)(TNF) & NDEF_TNF_MASK) )   /*< Build the record header byte, made of MB, ME, CF, SR, IL bits and TNF type */

/*! Read bits in header byte */
#define ndefHeaderMB(record)             ( ((record)->header & 0x80U) >> 7 )    /*!< Return the MB bit from the record header byte */
#define ndefHeaderME(record)             ( ((record)->header & 0x40U) >> 6 )    /*!< Return the ME bit from the record header byte */
#define ndefHeaderCF(record)             ( ((record)->header & 0x20U) >> 5 )    /*!< Return the CF bit from the record header byte */
#define ndefHeaderSR(record)             ( ((record)->header & 0x10U) >> 4 )    /*!< Return the SR bit from the record header byte */
#define ndefHeaderIL(record)             ( ((record)->header & 0x08U) >> 3 )    /*!< Return the IL bit from the record header byte */
#define ndefHeaderTNF(record)            (  (record)->header & NDEF_TNF_MASK )  /*!< Return the TNF type from the record header byte */

/*! Set bits in header byte */
#define ndefHeaderSetMB(record)          ( (record)->header |= (1U << 7) )      /*!< Set the MB bit in the record header byte */
#define ndefHeaderSetME(record)          ( (record)->header |= (1U << 6) )      /*!< Set the ME bit in the record header byte */
#define ndefHeaderSetCF(record)          ( (record)->header |= (1U << 5) )      /*!< Set the CF bit in the record header byte */
#define ndefHeaderSetSR(record)          ( (record)->header |= (1U << 4) )      /*!< Set the SR bit in the record header byte */
#define ndefHeaderSetIL(record)          ( (record)->header |= (1U << 3) )      /*!< Set the IL bit in the record header byte */
#define ndefHeaderSetTNF(record, value)  ( (record)->header |= (uint8_t)(value) & NDEF_TNF_MASK )  /*!< Set the TNF type in the record header byte */

/*! Clear bits in header byte */
#define ndefHeaderClearMB(record)          ( (record)->header &= 0x7FU )        /*!< Clear the MB bit in the record header byte */
#define ndefHeaderClearME(record)          ( (record)->header &= 0xBFU )        /*!< Clear the ME bit in the record header byte */
#define ndefHeaderClearCF(record)          ( (record)->header &= 0xDFU )        /*!< Clear the CF bit in the record header byte */
#define ndefHeaderClearSR(record)          ( (record)->header &= 0xEFU )        /*!< Clear the SR bit in the record header byte */
#define ndefHeaderClearIL(record)          ( (record)->header &= 0xF7U )        /*!< Clear the IL bit in the record header byte */
#define ndefHeaderClearTNF(record, value)  ( (record)->header &= 0xF8U )        /*!< Clear the TNF type in the record header byte */

/*! Set or Clear the MB/ME bit in header byte */
#define ndefHeaderSetValueMB(record, value)    do{ (record)->header &= 0x7FU; (record)->header |= (((uint8_t)(value)) & 1U) << 7; }while(0)   /*!< Write the value to the MB bit in the record header byte */
#define ndefHeaderSetValueME(record, value)    do{ (record)->header &= 0xBFU; (record)->header |= (((uint8_t)(value)) & 1U) << 6; }while(0)   /*!< Write the value to the ME bit in the record header byte */
#define ndefHeaderSetValueSR(record, value)    do{ (record)->header &= 0xEFU; (record)->header |= (((uint8_t)(value)) & 1U) << 4; }while(0)   /*!< Write the value to the SR bit in the record header byte */

/*! Test bit in header byte */
#define ndefHeaderIsSetMB(record)        ( ndefHeaderMB(record) == 1U )         /*!< Return true if the Message Begin bit is set */
#define ndefHeaderIsSetSR(record)        ( ndefHeaderSR(record) == 1U )         /*!< Return true if the Short Record bit is set  */
#define ndefHeaderIsSetIL(record)        ( ndefHeaderIL(record) == 1U )         /*!< Return true if the Id Length bit is set     */


typedef struct ndefTypeStruct ndefType;       /*!< Forward declaration */
typedef struct ndefMessageStruct ndefMessage; /*!< Forward declaration */

/*! Record type */
typedef struct ndefRecordStruct {
  uint8_t  header;               /*!< Header byte made of MB:1 ME:1 CF:1 SR:1 IL:1 TNF:3 => 8 bits */
  uint8_t  typeLength;           /*!< Type length in bytes */
  uint8_t  idLength;             /*!< Id Length, presence depends on the IL bit */
  const uint8_t *type;           /*!< Type follows the structure implied by the value of the TNF field */
  const uint8_t *id;             /*!< Id (middle and terminating record chunks MUST NOT have an ID field) */
  ndefConstBuffer bufPayload;    /*!< Payload buffer. Payload length depends on the SR bit (either coded on 1 or 4 bytes) */

  const ndefType *ndeftype;      /*!< Well-known type data */

  struct ndefRecordStruct *next; /*!< Pointer to the next record, if any */
} ndefRecord;


/*
 ******************************************************************************
 * GLOBAL FUNCTION PROTOTYPES
 ******************************************************************************
 */

/*!
 *****************************************************************************
 * Reset an NDEF record
 *
 * This function clears every record field
 *
 * \param[in,out] record to reset
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRecordReset(ndefRecord *record);


/*!
 *****************************************************************************
 * Initialize an NDEF record
 *
 * This function initializes all record fields
 *
 * \param[in,out] record:     Record to initialize
 * \param[in]     tnf:        TNF type
 * \param[in]     bufType:    Type buffer
 * \param[in]     bufId:      Id buffer
 * \param[in]     bufPayload: Payload buffer
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRecordInit(ndefRecord *record, uint8_t tnf, const ndefConstBuffer8 *bufType, const ndefConstBuffer8 *bufId, const ndefConstBuffer *bufPayload);


/*!
 *****************************************************************************
 * Get NDEF record header length
 *
 * Return the length of header for the given record
 *
 * \param[in] record
 *
 * \return header length in bytes
 *****************************************************************************
 */
uint32_t ndefRecordGetHeaderLength(const ndefRecord *record);


/*!
 *****************************************************************************
 * Get NDEF record length
 *
 * Return the length of the given record, needed to store it as a raw buffer.
 * It includes the header length.
 *
 * \param[in] record
 *
 * \return record length in bytes
 *****************************************************************************
 */
uint32_t ndefRecordGetLength(const ndefRecord *record);


/*!
 *****************************************************************************
 * Set NDEF record type
 *
 * Set the type for the given record
 *
 * \param[in,out] record:  Record to set the type
 * \param[in]     tnf:     TNF type
 * \param[in]     bufType: Type buffer
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRecordSetType(ndefRecord *record, uint8_t tnf, const ndefConstBuffer8 *bufType);


/*!
 *****************************************************************************
 * Get NDEF record type
 *
 * Return the type for the given record
 *
 * \param[in]  record:  Record to get the type from
 * \param[out] tnf:     Pointer to the TNF type
 * \param[out] bufType: Type string buffer
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRecordGetType(const ndefRecord *record, uint8_t *tnf, ndefConstBuffer8 *bufType);


/*!
 *****************************************************************************
 * Check the record type matches a given type
 *
 * \param[in]  record:  Record to get the type from
 * \param[out] tnf:     the TNF type to compare with
 * \param[out] bufType: Type string buffer to compare with
 *
 * \return true or false
 *****************************************************************************
 */
bool ndefRecordTypeMatch(const ndefRecord *record, uint8_t tnf, const ndefConstBuffer8 *bufType);


/*!
 *****************************************************************************
 * Set NDEF record Id
 *
 * Set the Id for the given NDEF record
 *
 * \param[in]  record: Record to set the Id
 * \param[out] bufId:  Id buffer
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRecordSetId(ndefRecord *record, const ndefConstBuffer8 *bufId);


/*!
 *****************************************************************************
 * Get NDEF record Id
 *
 * Return the Id for the given NDEF record
 *
 * \param[in]  record: Record to get the Id from
 * \param[out] bufId:  Id buffer
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRecordGetId(const ndefRecord *record, ndefConstBuffer8 *bufId);


/*!
 *****************************************************************************
 * Set NDEF record payload
 *
 * Set the payload for the given record, update the SR bit accordingly
 *
 * \param[in,out] record:     Record to set the payload
 * \param[in]     bufPayload: Payload buffer
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRecordSetPayload(ndefRecord *record, const ndefConstBuffer *bufPayload);


/*!
 *****************************************************************************
 * Get NDEF record payload
 *
 * Return the payload for the given record
 *
 * \param[in]  record:     Record to get the payload from
 * \param[out] bufPayload: Payload buffer
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRecordGetPayload(const ndefRecord *record, ndefConstBuffer *bufPayload);


/*!
 *****************************************************************************
 * Decode a raw buffer to create an NDEF record
 *
 * Convert a raw buffer to a record
 *
 * \param[in]  bufPayload: Payload buffer to convert into record
 * \param[out] record:     Record created from the raw buffer
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRecordDecode(const ndefConstBuffer *bufPayload, ndefRecord *record);


#if NDEF_FEATURE_FULL_API
  /*!
  *****************************************************************************
  * Encode an NDEF record header to a raw buffer
  *
  * Convert a record header to a raw buffer. It is made of:
  * "header byte" (1 byte), type length (1 byte),
  * payload length (4 bytes), Id length (1 byte).
  * Total 7 bytes.
  *
  * \param[in]     record:    Record header to convert
  * \param[in,out] bufHeader: Output buffer to store the converted record header
  *                           The input length provides the output buffer allocated
  *                           length, used for parameter check to avoid overflow.
  *                           In case the buffer provided is too short, it is
  *                           updated with the required buffer length.
  *                           On success, it is updated with the actual buffer
  *                           length used to contain the converted record.
  *
  * \return ERR_NONE if successful or a standard error code
  *****************************************************************************
  */
  ReturnCode ndefRecordEncodeHeader(const ndefRecord *record, ndefBuffer *bufHeader);


  /*!
  *****************************************************************************
  * Encode an NDEF record to a raw buffer
  *
  * Convert a record to a raw buffer
  *
  * \param[in]     record:    Record to convert
  * \param[in,out] bufRecord: Output buffer to store the converted record
  *                           The input length provides the output buffer allocated
  *                           length, used for parameter check to avoid overflow.
  *                           In case the buffer provided is too short, it is
  *                           updated with the required buffer length.
  *                           On success, it is updated with the actual buffer
  *                           length used to contain the converted record.
  *
  * \return ERR_NONE if successful or a standard error code
  *****************************************************************************
  */
  ReturnCode ndefRecordEncode(const ndefRecord *record, ndefBuffer *bufRecord);
#endif


/*!
 *****************************************************************************
 * Get NDEF record payload length
 *
 * Return the length of payload for the given record
 *
 * \param[in] record
 *
 * \return payload length in bytes
 *****************************************************************************
 */
uint32_t ndefRecordGetPayloadLength(const ndefRecord *record);


/*!
 *****************************************************************************
 * Return a payload elementary item needed to build the complete record payload
 *
 * Call this function to get either the first payload item, or the next one.
 * Returns the next payload item, call it until it returns NULL.
 *
 * \param[in]  record:         Record
 * \param[out] bufPayloadItem: The payload item returned
 * \param[in]  begin:          Tell to return the first payload item or the next one
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
const uint8_t *ndefRecordGetPayloadItem(const ndefRecord *record, ndefConstBuffer *bufPayloadItem, bool begin);



#endif /* NDEF_RECORD_H */

/**
  * @}
  *
  */
