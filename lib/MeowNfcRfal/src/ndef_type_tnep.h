
/**
  ******************************************************************************
  * @file           : ndef_type_tnep.h
  * @brief          : NDEF TNEP (Tag NDEF Exchange Protocol record) types header file
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

#ifndef NDEF_TYPES_TNEP_H
#define NDEF_TYPES_TNEP_H



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


/*! RTD TNEP defines */
#define TNEP_VERSION_V1_0                       0x10U   /*!< TNEP version                              */
#define TNEP_COMMUNICATION_MODE_SINGLE_RESPONSE 0x00U   /*!< Single Response Communication mode        */
#define TNEP_COMMUNICATION_MODE_SPECIFIC        0xFEU   /*!< Specific Communication mode               */
#define TNEP_STATUS_TYPE_SUCCESS                0U      /*!< Status type Success                       */
#define TNEP_STATUS_TYPE_PROTOCOL_ERROR         1U      /*!< Status type Protocol Error                */


/*
 ******************************************************************************
 * GLOBAL TYPES
 ******************************************************************************
 */


/*! RTD TNEP Service Parameter */
typedef struct {
  ndefConstBuffer bufServiceNameUri;            /*!< Service Name URI string buffer            */
  uint8_t         tnepVersion;                  /*!< TNEP version                              */
  uint8_t         communicationMode;            /*!< TNEP communication mode                   */
  uint8_t         minimumWaitingTime;           /*!< Minimum Waiting Time WT_INT               */
  uint8_t         maximumWaitingTimeExtensions; /*!< Maximum number of waiting time extensions */
  uint8_t         maximumNdefMessageSize[2];    /*!< Maximum NDEF message size (Big Endian)    */
} ndefTypeRtdTnepServiceParameter;


/*! RTD TNEP Service Select */
typedef struct {
  ndefConstBuffer bufServiceNameUri;            /*!< Service Name URI string buffer         */
} ndefTypeRtdTnepServiceSelect;


/*! RTD TNEP Status */
typedef struct {
  uint8_t         statusType;                   /*!< Status type */
} ndefTypeRtdTnepStatus;


/*! RTD TNEP Record Type buffers */
extern const ndefConstBuffer8 bufRtdTypeTnepServiceParameter; /*! TNEP Service Parameter buffer  */
extern const ndefConstBuffer8 bufRtdTypeTnepServiceSelect;    /*! TNEP Service Select buffer     */
extern const ndefConstBuffer8 bufRtdTypeTnepStatus;           /*! TNEP Status buffer             */


/*
 ******************************************************************************
 * GLOBAL FUNCTION PROTOTYPES
 ******************************************************************************
 */


/*************************
 * TNEP Service Parameter
 *************************
 */


/*!
 *****************************************************************************
 * Computes the TNEP Service Parameter WT_INT from the Twait value (in ms)
 *
 * \param[in] twait: minimum waiting time between last TNEP write and next TNEP read
 *
 * \return WT_INT value corresponding to the Twait
 *****************************************************************************
 */
uint8_t ndefRtdTnepServiceParameterComputeWtInt(float twait);


/*!
 *****************************************************************************
 * Computes the TNEP Service Parameter Twait (in ms) from the WT_INT value
 *
 * \param[in] wtInt: NDEF Record field WT_INT defining the Twait
 *
 * \return Twait minimum waiting time between last TNEP write and next TNEP read
 *****************************************************************************
 */
float ndefRtdTnepServiceParameterComputeTwait(uint8_t wtInt);


/*!
 *****************************************************************************
 * Initialize a TNEP Service Parameter RTD type
 *
 * \param[out] type:           Type to initialize
 * \param[in]  tnepVersion:    TNEP version
 * \param[in]  bufServiceUri:  TNEP Service Name URI string buffer
 * \param[in]  comMode:        TNEP Communication Mode
 * \param[in]  minWaitingTime: Minimum Waiting Time
 * \param[in]  maxExtensions:  Maximum Waiting Time Extensions
 * \param[in]  maxMessageSize: Maximum NDEF Message Size (in bytes)
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRtdTnepServiceParameterInit(ndefType *type, uint8_t tnepVersion, const ndefConstBuffer *bufServiceUri, uint8_t comMode, uint8_t minWaitingTime, uint8_t maxExtensions, uint16_t maxMessageSize);


/*!
 *****************************************************************************
 * Get TNEP Service Parameter RTD type content
 *
 * \param[in]   type:           Type to get information from
 * \param[out]  tnepVersion:    TNEP version
 * \param[out]  bufServiceUri:  TNEP Service Name URI string buffer
 * \param[out]  comMode:        TNEP Communication Mode
 * \param[out]  minWaitingTime: Minimum Waiting Time
 * \param[out]  maxExtensions:  Maximum Waiting Time Extensions
 * \param[out]  maxMessageSize: Maximum NDEF Message Size (in bytes)
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefGetRtdTnepServiceParameter(const ndefType *type, uint8_t *tnepVersion, ndefConstBuffer *bufServiceUri, uint8_t *comMode, uint8_t *minWaitingTime, uint8_t *maxExtensions, uint16_t *maxMessageSize);


/*!
 *****************************************************************************
 * Convert an NDEF record to a TNEP Service Parameter RTD type
 *
 * \param[in]  record: Record to convert
 * \param[out] type:   The converted type
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRecordToRtdTnepServiceParameter(const ndefRecord *record, ndefType *type);


/*!
 *****************************************************************************
 * Convert a TNEP Service Parameter RTD type to an NDEF record
 *
 * \param[in]  type:   Type to convert
 * \param[out] record: The converted type
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRtdTnepServiceParameterToRecord(const ndefType *type, ndefRecord *record);


/**********************
 * TNEP Service Select
 **********************
 */


/*!
 *****************************************************************************
 * Initialize a TNEP Service Select RTD type
 *
 * \param[out] type:          Type to initialize
 * \param[in]  bufServiceUri: TNEP Service Name URI string buffer
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRtdTnepServiceSelectInit(ndefType *type, const ndefConstBuffer *bufServiceUri);


/*!
 *****************************************************************************
 * Get TNEP Service Select RTD type content
 *
 * \param[in]   type:          Type to get information from
 * \param[out]  bufServiceUri: TNEP Service Name URI string buffer
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefGetRtdTnepServiceSelect(const ndefType *type, ndefConstBuffer *bufServiceUri);


/*!
 *****************************************************************************
 * Convert an NDEF record to a TNEP Service Select RTD type
 *
 * \param[in]  record: Record to convert
 * \param[out] type:   The converted type
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRecordToRtdTnepServiceSelect(const ndefRecord *record, ndefType *type);


/*!
 *****************************************************************************
 * Convert a TNEP Service Select RTD type to an NDEF record
 *
 * \param[in]  type:   Type to convert
 * \param[out] record: The converted type
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRtdTnepServiceSelectToRecord(const ndefType *type, ndefRecord *record);


/***************
 * TNEP Status
 ***************
 */


/*!
 *****************************************************************************
 * Initialize a TNEP Status RTD type
 *
 * \param[out] type:       Type to initialize
 * \param[in]  statusType: TNEP status type
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRtdTnepStatusInit(ndefType *type, uint8_t statusType);


/*!
 *****************************************************************************
 * Get TNEP Status RTD type content
 *
 * \param[in]   type:       Type to get information from
 * \param[out]  statusType: TNEP status type
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefGetRtdTnepStatus(const ndefType *type, uint8_t *statusType);


/*!
 *****************************************************************************
 * Convert an NDEF record to a TNEP Status RTD type
 *
 * \param[in]  record: Record to convert
 * \param[out] type:   The converted type
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRecordToRtdTnepStatus(const ndefRecord *record, ndefType *type);


/*!
 *****************************************************************************
 * Convert a TNEP Status RTD type to an NDEF record
 *
 * \param[in]  type:   Type to convert
 * \param[out] record: The converted type
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRtdTnepStatusToRecord(const ndefType *type, ndefRecord *record);



#endif /* NDEF_TYPES_TNEP_H */

/**
  * @}
  *
  */
