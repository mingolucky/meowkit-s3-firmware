
/**
  ******************************************************************************
  * @file           : ndef_type_wlc.h
  * @brief          : NDEF WLC (Wireless Charging) types header file
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


#ifndef NDEF_TYPE_WLC_H
#define NDEF_TYPE_WLC_H



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


#define NDEF_WLC_STATUSINFO_CONTROLBYTE1_BATTERY_LEVEL_MASK        0x01U /*!< WLC Status and Info Control byte 1: Battery Level mask */
#define NDEF_WLC_STATUSINFO_CONTROLBYTE1_RECEIVE_POWER_MASK        0x02U /*!< WLC Status and Info Control byte 1: Receive Power mask */
#define NDEF_WLC_STATUSINFO_CONTROLBYTE1_RECEIVE_VOLTAGE_MASK      0x04U /*!< WLC Status and Info Control byte 1: Receive Voltage mask */
#define NDEF_WLC_STATUSINFO_CONTROLBYTE1_RECEIVE_CURRENT_MASK      0x08U /*!< WLC Status and Info Control byte 1: Receive Current mask */
#define NDEF_WLC_STATUSINFO_CONTROLBYTE1_TEMPERATURE_BATTERY_MASK  0x10U /*!< WLC Status and Info Control byte 1: Temperature Battery mask */
#define NDEF_WLC_STATUSINFO_CONTROLBYTE1_TEMPERATURE_WLCL_MASK     0x20U /*!< WLC Status and Info Control byte 1: Temperature WLCL mask */
#define NDEF_WLC_STATUSINFO_CONTROLBYTE1_RFU_MASK                  0x40U /*!< WLC Status and Info Control byte 1: RFU mask */
#define NDEF_WLC_STATUSINFO_CONTROLBYTE1_CONTROL_BYTE_2_MASK       0x80U /*!< WLC Status and Info Control byte 1: Control byte 2 mask */


/*
 ******************************************************************************
 * GLOBAL TYPES
 ******************************************************************************
 */


/*! Structure to store WLC Capability */
typedef struct {
  uint8_t wlcProtocolVersion;     /*!< WLC Protocol Version */
  uint8_t wlcConfigModeReq;       /*!< WLC Config: MODE_REQ */
  uint8_t wlcConfigWaitTimeRetry; /*!< WLC Config: WaitTimeRetry */
  uint8_t wlcConfigNegoWait;      /*!< WLC Config: NEGO_WAIT */
  uint8_t wlcConfigRdConf;        /*!< WLC Config: RD_CONF */
  uint8_t capWtIntRfu;            /*!< Cap Wt Int RFU */
  uint8_t capWtInt;               /*!< Cap Wt Int */
  uint8_t ndefRdWt;               /*!< NDEF Rd Wt */
  uint8_t ndefWriteToInt;         /*!< NDEF Write To Int */
  uint8_t ndefWriteWtInt;         /*!< NDEF Write Wt Int */
} ndefTypeRtdWlcCapability;


/*! Structure to store WLC Status and Info */
typedef struct {
  uint8_t controlByte1;       /*!< Control byte 1 */
  uint8_t batteryLevel;       /*!< Battery level */
  uint8_t receivePower;       /*!< Receive power */
  uint8_t receiveVoltage;     /*!< Receive voltage */
  uint8_t receiveCurrent;     /*!< Receive current */
  uint8_t temperatureBattery; /*!< Battery temperature */
  uint8_t temperatureWlcl;    /*!< WLCL temperature */
  uint8_t rfu;                /*!< RFU */
  uint8_t controlByte2;       /*!< Control byte 2 */
} ndefTypeRtdWlcStatusInfo;


/*! Structure to store WLC Poll Information */
typedef struct {
  uint8_t pTx;               /*!< P Tx, Transmit Power Level */
  uint8_t wlcPCap;           /*!< WLC_P Capability */
  uint8_t powerClass;        /*!< Power Class */
  uint8_t totPowerSteps;     /*!< Tot Power Steps */
  uint8_t curPowerStep;      /*!< Current Power Step */
  uint8_t nextMinStepInc;    /*!< Next Min Step Inc */
  uint8_t nextMinStepDec;    /*!< Next Min Step Dec */
} ndefTypeRtdWlcPollInfo;


/*! Structure to store WLC Listen Control */
typedef struct {
  uint8_t statusInfoErrorFlag;     /*!< Status information: ERROR_FLG */
  uint8_t statusInfoBatteryStatus; /*!< Status information: BATTERY_STATUS */
  uint8_t statusInfoCnt;           /*!< Status information: CNT */
  uint8_t wptConfigWptReq;         /*!< WPT Config: WPT_REQ */
  uint8_t wptConfigWptDuration;    /*!< WPT Config: WPT_DURATION */
  uint8_t wptConfigInfoReq;        /*!< WPT Config: INFO_REQ */
  uint8_t powerAdjReq;             /*!< Power Adjust Req */
  uint8_t batteryLevel;            /*!< Battery level */
  uint8_t drvInfoFlag;             /*!< Drv info: DRV_FLAG */
  uint8_t drvInfoInt;              /*!< Drv info: DRV_INT */
  uint8_t holdOffWtInt;            /*!< Hold off Wt Int */
  uint8_t errorInfoError;          /*!< [Error info]* if ERROR_FLG set: WLC_INFO_ERROR */
  uint8_t errorInfoTemperature;    /*!< [Error info]* if ERROR_FLG set: TEMPERATURE_ERROR */
} ndefTypeRtdWlcListenCtl;


/*! WLC Record Type buffers */
extern const ndefConstBuffer8 bufTypeRtdWlcCapability; /*!< WLC Capability Type Record buffer             */
extern const ndefConstBuffer8 bufTypeRtdWlcStatusInfo; /*!< WLC Status and Information Type Record buffer */
extern const ndefConstBuffer8 bufTypeRtdWlcPollInfo;   /*!< WLC Poll Information Type Record buffer       */
extern const ndefConstBuffer8 bufTypeRtdWlcListenCtl;  /*!< WLC Listen Control Type Record buffer         */


/*! WLC MODE_REQ */
typedef enum {
  NDEF_RTD_WLC_CAPABILITY_MODE_STATIC,
  NDEF_RTD_WLC_CAPABILITY_MODE_NEGOTIATED,
  NDEF_RTD_WLC_CAPABILITY_MODE_BATTERY_FULL,
  NDEF_RTD_WLC_CAPABILITY_MODE_RFU
} ndefRtdWlcReqMode;


/*
 ******************************************************************************
 * GLOBAL FUNCTION PROTOTYPES
 ******************************************************************************
 */


/***************************
 * WLC - Wireless Charging
 ***************************
 */


/* WLC Capability */


/*!
 *****************************************************************************
 * Initialize a WLC Capability type
 *
 * \param[out] type:          NDEF type to initialize
 * \param[in]  wlcCapability: WLC Capability
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRtdWlcCapabilityInit(ndefType *type, const ndefTypeRtdWlcCapability *wlcCapability);


/*!
 *****************************************************************************
 * Get WLC Capability type content
 *
 * \param[in]  type:          NDEF type to get information from
 * \param[out] wlcCapability: WLC Capability
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefGetRtdWlcCapability(const ndefType *type, ndefTypeRtdWlcCapability *wlcCapability);


/*!
 *****************************************************************************
 * Convert an NDEF record to a WLC Capability type
 *
 * \param[in]  record: Record to convert
 * \param[out] type:   The converted NDEF type
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRecordToRtdWlcCapability(const ndefRecord *record, ndefType *type);


/*!
 *****************************************************************************
 * Convert a WLC Capability type to an NDEF record
 *
 * \param[in]  type:   NDEF type to convert
 * \param[out] record: The converted type
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRtdWlcCapabilityToRecord(const ndefType *type, ndefRecord *record);


/* WLC Status and Information */


/*!
 *****************************************************************************
 * Initialize a WLC Status and Information type
 *
 * \param[out] type:          NDEF type to initialize
 * \param[in]  wlcStatusInfo: WLC Status and Information
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRtdWlcStatusInfoInit(ndefType *type, const ndefTypeRtdWlcStatusInfo *wlcStatusInfo);


/*!
 *****************************************************************************
 * Get WLC Status and Information type content
 *
 * \param[in]  type:          NDEF type to get information from
 * \param[out] wlcStatusInfo: WLC Status and Information
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefGetRtdWlcStatusInfo(const ndefType *type, ndefTypeRtdWlcStatusInfo *wlcStatusInfo);


/*!
 *****************************************************************************
 * Convert an NDEF record to a WLC Status and Information type
 *
 * \param[in]  record: Record to convert
 * \param[out] type:   The converted NDEF type
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRecordToRtdWlcStatusInfo(const ndefRecord *record, ndefType *type);


/*!
 *****************************************************************************
 * Convert a WLC Status and Information type to an NDEF record
 *
 * \param[in]  type:   NDEF type to convert
 * \param[out] record: The converted type
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRtdWlcStatusInfoToRecord(const ndefType *type, ndefRecord *record);


/* WLC Poll Information */


/*!
 *****************************************************************************
 * Initialize a WLC Poll Information type
 *
 * \param[out] type:        NDEF type to initialize
 * \param[in]  wlcPollInfo: WLC Poll Info
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRtdWlcPollInfoInit(ndefType *type, const ndefTypeRtdWlcPollInfo *wlcPollInfo);


/*!
 *****************************************************************************
 * Get WLC Poll type content
 *
 * \param[in]  type:        NDEF type to get information from
 * \param[out] wlcPollInfo: WLC Poll Info
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefGetRtdWlcPollInfo(const ndefType *type, ndefTypeRtdWlcPollInfo *wlcPollInfo);


/*!
 *****************************************************************************
 * Convert an NDEF record to a WLC Poll Info type
 *
 * \param[in]  record: Record to convert
 * \param[out] type:   The converted type
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRecordToRtdWlcPollInfo(const ndefRecord *record, ndefType *type);


/*!
 *****************************************************************************
 * Convert a WLC Poll Info type to an NDEF record
 *
 * \param[in]  type:   NDEF Type to convert
 * \param[out] record: The converted type
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRtdWlcPollInfoToRecord(const ndefType *type, ndefRecord *record);


/* WLC Listen Control */


/*!
 *****************************************************************************
 * Initialize a WLC Listen Control type
 *
 * \param[out] type:         NDEF Type to initialize
 * \param[in]  wlcListenCtl: WLC Listen Control
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRtdWlcListenCtlInit(ndefType *type, const ndefTypeRtdWlcListenCtl *wlcListenCtl);


/*!
 *****************************************************************************
 * Get WLC Listen Control type content
 *
 * \param[in]  type:         NDEF type to get information from
 * \param[out] wlcListenCtl: WLC Listen Control
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefGetRtdWlcListenCtl(const ndefType *type, ndefTypeRtdWlcListenCtl *wlcListenCtl);


/*!
 *****************************************************************************
 * Convert an NDEF record to a WLC Listen Control type
 *
 * \param[in]  record: Record to convert
 * \param[out] type:   The converted type
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRecordToRtdWlcListenCtl(const ndefRecord *record, ndefType *type);


/*!
 *****************************************************************************
 * Convert a WLC Listen Control type to an NDEF record
 *
 * \param[in]  type:   NDEF type to convert
 * \param[out] record: The converted type
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRtdWlcListenCtlToRecord(const ndefType *type, ndefRecord *record);



#endif

/**
  * @}
  *
  */
