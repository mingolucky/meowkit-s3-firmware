
/**
  ******************************************************************************
  * @file           : ndef_type_wpcwlc.h
  * @brief          : NDEF RTD Wireless Power Consortium WLC Record (WPCWLC) type header file
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


#ifndef NDEF_TYPE_WPCWLC_H
#define NDEF_TYPE_WPCWLC_H



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

#define NDEF_KI_APPLICATION_PROFILE           03U   /*!< Ki Application Profile */

#define NDEF_KI_V10_PAYLOAD_LENGTH            16U   /*!< Ki v1.0 Payload length */

#define NDEF_KI_APPLICATION_PROFILE_OFFSET  0x00U   /*!< Ki Application Profile offset             */
#define NDEF_KI_VERSION_OFFSET              0x01U   /*!< Ki Major|Minor Version Offset             */
#define NDEF_KI_ALIVE_FDT_OFFSET            0x02U   /*!< Ki Alive FDT Offset                       */
#define NDEF_KI_READ_ADDRESS_OFFSET         0x03U   /*!< Ki Read Data Buffer Start Address Offset  */
#define NDEF_KI_WRITE_ADDRESS_OFFSET        0x04U   /*!< Ki Write Data Buffer Start Address Offset */
#define NDEF_KI_READ_SIZE_OFFSET            0x05U   /*!< Ki Read Data Buffer Size Offset           */
#define NDEF_KI_WRITE_SIZE_OFFSET           0x06U   /*!< Ki Write Data Buffer Size Offset          */
#define NDEF_KI_READ_CMD_OFFSET             0x07U   /*!< Ki Read Command Code Offset               */
#define NDEF_KI_WRITE_CMD_OFFSET            0x08U   /*!< Ki Write Commande Code Offset             */
#define NDEF_KI_MAX_T_SLOT_FOD_OFFSET       0x09U   /*!< Ki Maximum T_SLOT FOD Offset              */
#define NDEF_KI_MIN_T_POWER_OFFSET          0x0AU   /*!< Ki Minimum T_POWER Offset                 */
#define NDEF_KI_T_SUSPEND_OFFSET            0x0BU   /*!< Ki T_SUSPEND Offset                       */
#define NDEF_KI_COMM_LAG_MAX_OFFSET         0x0CU   /*!< Ki T_COMM_LAG,MAX Offset                  */
#define NDEF_KI_WRITE_SEQ_LENGTH_OFFSET     0x0DU   /*!< Ki Write Sequence Length Offset           */
#define NDEF_KI_MIN_POWER_OFFSET            0x0EU   /*!< Ki Minimum Power Offset                   */
#define NDEF_KI_MAX_POWER_OFFSET            0x0FU   /*!< Ki Maximum Power Offset                   */


/*
 ******************************************************************************
 * GLOBAL TYPES
 ******************************************************************************
 */

/*! RTD Record Type buffers */
extern const ndefConstBuffer8 bufRtdTypeWpcWlc;        /*! WPCWLC (Wireless Power Consortium WLC) Record Type buffer */


/*! RTD Wireless Power Consortium WLC Record External Type */
typedef struct {
  ndefConstBuffer  bufPayload; /*!< WPCWLC payload */
} ndefTypeRtdWpcWlc;


/*
 ******************************************************************************
 * GLOBAL FUNCTION PROTOTYPES
 ******************************************************************************
 */


/**********************
 * WPCWLC External Type
 **********************
 */

/*!
 *****************************************************************************
 * Initialize an RTD Wireless Power Consortium WLC Record External type
 *
 * \param[out] wpcWlc:     Type to initialize
 * \param[in]  bufPayload: Payload buffer
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRtdWpcWlcInit(ndefType *wpcWlc, const ndefConstBuffer *bufPayload);


/*!
 *****************************************************************************
 * Get RTD Wireless Power Consortium WLC Record type content
 *
 * \param[in]  wpcWlc:    Type to get information from
 * \param[out] bufWpcWlc: WPCWLC payload buffer
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefGetRtdWpcWlc(const ndefType *wpcWlc, ndefConstBuffer *bufWpcWlc);


/*!
 *****************************************************************************
 * Convert an NDEF record to an RTD Wireless Power Consortium WLC Record External type
 *
 * \param[in]  record: Record to convert
 * \param[out] wpcWlc: The converted WPCWLC type
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRecordToRtdWpcWlc(const ndefRecord *record, ndefType *wpcWlc);


/*!
 *****************************************************************************
 * Convert an RTD Wireless Power Consortium WLC Record External type to an NDEF record
 *
 * \param[in]  wpcWlc: WPCWLC type to convert
 * \param[out] record: The converted type
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRtdWpcWlcToRecord(const ndefType *wpcWlc, ndefRecord *record);



#endif /* NDEF_TYPE_WPCWLC_H */

/**
  * @}
  *
  */
