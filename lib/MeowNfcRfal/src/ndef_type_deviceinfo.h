
/**
  ******************************************************************************
  * @file           : ndef_type_deviceinfo.h
  * @brief          : NDEF RTD Device Information type header file
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


#ifndef NDEF_TYPE_RTD_DEVICE_INFO_H
#define NDEF_TYPE_RTD_DEVICE_INFO_H



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


/*! Device Information defines */
#define NDEF_UUID_LENGTH                16U    /*!< Device Information UUID length */


/*
 ******************************************************************************
 * GLOBAL TYPES
 ******************************************************************************
 */

/*! RTD Device Information Record Type buffer */
extern const ndefConstBuffer8 bufRtdTypeDeviceInfo; /*! Device Information Record Type buffer               */


/*! RTD Device Information types */
#define NDEF_DEVICE_INFO_MANUFACTURER_NAME     0U /*!< Manufacturer name                      */
#define NDEF_DEVICE_INFO_MODEL_NAME            1U /*!< Model name                             */
#define NDEF_DEVICE_INFO_DEVICE_UNIQUE_NAME    2U /*!< Device Unique Name aka "Friendly Name" */
#define NDEF_DEVICE_INFO_UUID                  3U /*!< UUID                                   */
#define NDEF_DEVICE_INFO_FIRMWARE_VERSION      4U /*!< Firmware Version                       */
#define NDEF_DEVICE_INFO_TYPE_COUNT    5U /*!< Maximum Device Information types */


/*! RTD Device Information Entry */
typedef struct {
  uint8_t        type;      /*!< Device Information Type              */
  uint8_t        length;    /*!< Device Information length            */
  const uint8_t *buffer;    /*!< Device Information pointer to buffer */
} ndefDeviceInfoEntry;


/*! RTD Type Device Information */
typedef struct {
  ndefDeviceInfoEntry devInfo[NDEF_DEVICE_INFO_TYPE_COUNT]; /*!< Device Information entries */
} ndefTypeRtdDeviceInfo;


/*
 ******************************************************************************
 * GLOBAL FUNCTION PROTOTYPES
 ******************************************************************************
 */


/*********************
 * Device Information
 *********************
 */


/*!
 *****************************************************************************
 * Initialize a RTD Device Information type
 *
 * \param[out] devInfo:          Type to initialize
 * \param[in]  devInfoData:      Device Information data
 * \param[in]  devInfoDataCount: Number of Device Information data
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRtdDeviceInfoInit(ndefType *devInfo, const ndefDeviceInfoEntry *devInfoData, uint8_t devInfoDataCount);


/*!
 *****************************************************************************
 * Get RTD Device Information type content
 *
 * \param[in]  devInfo:     Type to get information from
 * \param[out] devInfoData: Device Information data
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefGetRtdDeviceInfo(const ndefType *devInfo, ndefTypeRtdDeviceInfo *devInfoData);


/*!
 *****************************************************************************
 * Convert an NDEF record to a Device Information RTD type
 *
 * \param[in]  record:  Record to convert
 * \param[out] devInfo: The converted type
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRecordToRtdDeviceInfo(const ndefRecord *record, ndefType *devInfo);


/*!
 *****************************************************************************
 * Convert a Device Information RTD type to an NDEF record
 *
 * \param[in]  devInfo: Type to convert
 * \param[out] record:  The converted type
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRtdDeviceInfoToRecord(const ndefType *devInfo, ndefRecord *record);



#endif /* NDEF_TYPE_RTD_DEVICE_INFO_H */

/**
  * @}
  *
  */
