
/**
  ******************************************************************************
  * @file           : ndef_type_bluetooth.h
  * @brief          : NDEF Bluetooth type header file
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


#ifndef NDEF_TYPE_BLUETOOTH_H
#define NDEF_TYPE_BLUETOOTH_H



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

#define NDEF_BT_EIR_DEVICE_ADDRESS_TYPE_OFFSET    6U    /*!< Device Address Type (Public, Random) offset */


/*!< Number of EIRs that can be decoded simultaneously
     Allow to control the number of allocated memory space to store decoded EIRs
     Should be lower or equal to the number of known EIRs (currently 26)
*/
#define NDEF_BT_EIR_COUNT                         8U    /*!< Number of EIRs that can be decoded */


/*
 ******************************************************************************
 * GLOBAL TYPES
 ******************************************************************************
 */


/*! Bluetooth LE address types */
typedef enum {
  NDEF_BLE_PUBLIC_ADDRESS_TYPE = 0x0U, /*!< Public Device Address */
  NDEF_BLE_RANDOM_ADDRESS_TYPE = 0x1U, /*!< Random Device Address */
  NDEF_BLE_UNDEF_ADDRESS_TYPE  = 0xFFU /*!< Device Address is undefined */
} ndefBluetoothLEAddressType;


/*! Bluetooth LE roles */
typedef enum {
  NDEF_BLE_ROLE_PERIPH_ONLY       = 0x0U, /*!< Only Peripheral Role supported */
  NDEF_BLE_ROLE_CENTRAL_ONLY      = 0x1U, /*!< Only Central Role supported */
  NDEF_BLE_ROLE_PERIPH_PREFERRED  = 0x2U, /*!< Peripheral and Central Role supported, Peripheral Role preferred for connection establishment */
  NDEF_BLE_ROLE_CENTRAL_PREFERRED = 0x3U, /*!< Peripheral and Central Role supported, Central Role preferred for connection establishment */
  NDEF_BLE_ROLE_UNDEF             = 0xFFU /*!< LE Role is undefined */
} ndefBluetoothLERole;


/*! Extended Inquiry Responses, as defined in the Bluetooth v4.0 Core Specification */
#define NDEF_BT_EIR_FLAGS                              0x01U   /*!< Bluetooth flags:\n
                                                                    b0: LE limited Discoverable Mode,
                                                                    b1: LE general Discoverable Mode,
                                                                    b2: initiatorBitRates/EDR not supported,
                                                                    b3: Simultaneous LE & initiatorBitRates/EDR Controller,
                                                                    b4: Simultaneous LE & initiatorBitRates/EDR Host */
#define NDEF_BT_EIR_SERVICE_CLASS_UUID_PARTIAL_16      0x02U   /*!< Bluetooth service UUID on 16-bits (partial list) */
#define NDEF_BT_EIR_SERVICE_CLASS_UUID_COMPLETE_16     0x03U   /*!< Bluetooth service UUID on 16-bits (complete list) */
#define NDEF_BT_EIR_SERVICE_CLASS_UUID_PARTIAL_32      0x04U   /*!< Bluetooth service UUID on 32-bits (partial list) */
#define NDEF_BT_EIR_SERVICE_CLASS_UUID_COMPLETE_32     0x05U   /*!< Bluetooth service UUID on 32-bits (complete list) */
#define NDEF_BT_EIR_SERVICE_CLASS_UUID_PARTIAL_128     0x06U   /*!< Bluetooth service UUID on 128-bits (partial list) */
#define NDEF_BT_EIR_SERVICE_CLASS_UUID_COMPLETE_128    0x07U   /*!< Bluetooth service UUID on 128-bits (complete list) */
#define NDEF_BT_EIR_SHORT_LOCAL_NAME                   0x08U   /*!< Shortened local name */
#define NDEF_BT_EIR_COMPLETE_LOCAL_NAME                0x09U   /*!< Complete local name */
#define NDEF_BT_EIR_TX_POWER_LEVEL                     0x0AU   /*!< TX Power Level (1 byte): 0xXX:-127 to +127dBm */
#define NDEF_BT_EIR_DEVICE_CLASS                       0x0DU   /*!< Class of device, Format defined in Assigned Numbers */
#define NDEF_BT_EIR_SIMPLE_PAIRING_HASH                0x0EU   /*!< Simple Pairing Hash C (16 octets), Format defined in [Vol. 2], Part H Section 7.2.2 */
#define NDEF_BT_EIR_SIMPLE_PAIRING_RANDOMIZER          0x0FU   /*!< Simple Pairing Randomizer R (16 octets), Format defined in[Vol. 2], Part H Section 7.2.2 */
#define NDEF_BT_EIR_SECURITY_MANAGER_TK_VALUE          0x10U   /*!< TK Value: Value as used in pairing over LE Physical channel. Format defined in [Vol. 3], Part H Section 2.3 */
#define NDEF_BT_EIR_SECURITY_MANAGER_FLAGS             0x11U   /*!< Flags (1 octet):\n
                                                                    b0: OOB Flags Field (0 = OOB data not present, 1 = OOB data present),
                                                                    b1: LE supported (Host) (i.e. bit 65 of LMP Extended Feature bits Page 1),
                                                                    b2: Simultaneous LE and initiatorBitRates/EDR to Same Device Capable (Host) (i.e. bit 66 of LMP Extended Feature bits Page 1)
                                                                    b3: Address type (0 = Public Address, 1 = Random Address) */
#define NDEF_BT_EIR_SLAVE_CONNECTION_INTERVAL_RANGE    0x12U   /*!< Slave Connection Interval Range: The first 2 octets define the minimum value for the connection interval, The second 2 octets define the maximum value for the connection interval */
#define NDEF_BT_EIR_SERVICE_SOLICITATION_16            0x14U   /*!< Service UUIDs: List of 16 bit Service UUIDs */
#define NDEF_BT_EIR_SERVICE_SOLICITATION_128           0x15U   /*!< Service UUIDs: List of 128 bit Service UUIDs */
#define NDEF_BT_EIR_SERVICE_DATA                       0x16U   /*!< Service Data (2 or more octets): The first 2 octets contain the 16 bit Service UUID followed by additional service data */
#define NDEF_BT_EIR_APPEARANCE                         0x19U   /*!< UUID for `Appearance`: The Appearance characteristic value shall be the enumerated value as defined by Bluetooth Assigned Numbers document */
#define NDEF_BT_EIR_LE_DEVICE_ADDRESS                  0x1BU   /*!< 6 LSB bytes: Device address, 7th byte: Address type (Public/Random) */
#define NDEF_BT_EIR_LE_ROLE                            0x1CU   /*!< Device Role: Periph only, Central only, Periph preferred, Central preferred */
#define NDEF_BT_EIR_LE_SECURE_CONN_CONFIRMATION_VALUE  0x22U   /*!< Secure Connection Confirmation value */
#define NDEF_BT_EIR_LE_SECURE_CONN_RANDOM_VALUE        0x23U   /*!< Secure Connection Random value */
#define NDEF_BT_EIR_MANUFACTURER_DATA                  0xFFU   /*!< Manufacturer Specific Data (2 or more octets): The first 2 octets contain the Company Identifier Code followed by additional manufacturer specific data */


/*! Bluetooth Out-Of-Band data structure
  * Some fields are shared by both Br/Edr & LE Bluetooth, some are specific.
  */
typedef struct {
  /* Mandatory fields:
      bufDeviceAddress
     Optional common fields:
      NDEF_BT_EIR_FLAGS
      NDEF_BT_EIR_SERVICE_CLASS_UUID_PARTIAL_16
      NDEF_BT_EIR_SERVICE_CLASS_UUID_COMPLETE_16
      NDEF_BT_EIR_SERVICE_CLASS_UUID_PARTIAL_32
      NDEF_BT_EIR_SERVICE_CLASS_UUID_COMPLETE_32
      NDEF_BT_EIR_SERVICE_CLASS_UUID_PARTIAL_128
      NDEF_BT_EIR_SERVICE_CLASS_UUID_COMPLETE_128
      NDEF_BT_EIR_SHORT_LOCAL_NAME
      NDEF_BT_EIR_COMPLETE_LOCAL_NAME
      NDEF_BT_EIR_TX_POWER_LEVEL
      NDEF_BT_EIR_DEVICE_CLASS
      NDEF_BT_EIR_SECURITY_MANAGER_FLAGS
      NDEF_BT_EIR_SLAVE_CONNECTION_INTERVAL_RANGE
      NDEF_BT_EIR_SERVICE_SOLICITATION_16
      NDEF_BT_EIR_SERVICE_SOLICITATION_128
      NDEF_BT_EIR_SERVICE_DATA
     For Br/Edr only:
      NDEF_BT_EIR_SIMPLE_PAIRING_HASH
      NDEF_BT_EIR_SIMPLE_PAIRING_RANDOMIZER
      BLE mandatory fields:
      NDEF_BT_EIR_LE_ROLE
     BLE optional fields:
      NDEF_BT_EIR_LE_SECURE_CONN_CONFIRMATION_VALUE
      NDEF_BT_EIR_LE_SECURE_CONN_RANDOM_VALUE
      NDEF_BT_EIR_SECURITY_MANAGER_TK_VALUE
      NDEF_BT_EIR_APPEARANCE
      NDEF_BT_EIR_MANUFACTURER_DATA */

  ndefConstBuffer bufDeviceAddress;         /*!< Device address, for initiatorBitRates/EDR only */

  const uint8_t *eir[NDEF_BT_EIR_COUNT];    /*!< Array containing pointer to each EIR */

} ndefTypeBluetooth;


/*! Bluetooth Record Type buffers */
extern const ndefConstBuffer8 bufMediaTypeBluetoothBrEdr;       /*! Bluetooth initiatorBitRates/EDR Out-Of-Band Record Type buffer */
extern const ndefConstBuffer8 bufMediaTypeBluetoothLe;          /*! Bluetooth Low Energy Record Type buffer         */
extern const ndefConstBuffer8 bufMediaTypeBluetoothSecureBrEdr; /*! Bluetooth Secure initiatorBitRates/EDR Record Type buffer      */
extern const ndefConstBuffer8 bufMediaTypeBluetoothSecureLe;    /*! Bluetooth Secure Low Energy Record Type buffer  */


/*
 ******************************************************************************
 * GLOBAL FUNCTION PROTOTYPES
 ******************************************************************************
 */


/***************
 * Bluetooth
 ***************
 */


/*!
 *****************************************************************************
 * Get Bluetooth EIR length
 *
 * \param[in] eir: Extended Inquiry Response
 *
 * \return the length of the EIR passed as parameter
 *****************************************************************************
 */
uint8_t ndefBluetoothEirLength(const uint8_t *eir);


/*!
 *****************************************************************************
 * Get Bluetooth EIR data length
 *
 * \param[in] eir: Extended Inquiry Response
 *
 * \return the length of the data contained in the EIR passed as parameter
 *****************************************************************************
 */
uint8_t ndefBluetoothEirDataLength(const uint8_t *eir);


/*!
 *****************************************************************************
 * Get Bluetooth EIR type
 *
 * \param[in] eir: Extended Inquiry Response
 *
 * \return the type of the EIR passed as parameter
 *****************************************************************************
 */
uint8_t ndefBluetoothEirType(const uint8_t *eir);

/*!
 *****************************************************************************
 * Get a pointer to a Bluetooth EIR data
 *
 * \param[in] eir: Extended Inquiry Response
 *
 * \return a pointer to the data of the EIR passed as parameter
 *****************************************************************************
 */
const uint8_t *ndefBluetoothEirData(const uint8_t *eir);


/*!
 *****************************************************************************
 * Convert a Bluetooth EIR to an ndefBuffer structure
 *
 * \param[in]     eir:    Extended Inquiry Response
 * \param[in,out] bufEir: pointer to the Extended Inquiry Response data
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefBluetoothEirDataToBuffer(const uint8_t *eir, ndefConstBuffer *bufEir);


/*!
 *****************************************************************************
 * Add a Bluetooth EIR to the bluetooth type
 *
 * \param[in] bluetooth: Bluetooth type
 * \param[in] eir:       EIR type to add
 *
 * Add eir after any EIR already listed in the bluetooth type,
 * or replace in case an EIR with matching type is already there.
 *
 * \return ERR_NONE if successful, ERR_NOMEM or a standard error code
 *****************************************************************************
 */
ReturnCode ndefBluetoothSetEir(ndefTypeBluetooth *bluetooth, const uint8_t *eir);


/*!
 *****************************************************************************
 * Get a pointer to a Bluetooth EIR
 *
 * \param[in] bluetooth: Bluetooth type
 * \param[in] eirType:   EIR type to find
 *
 * \return a pointer to the EIR matching the type, or NULL
 *****************************************************************************
 */
const uint8_t *ndefBluetoothGetEir(const ndefTypeBluetooth *bluetooth, uint8_t eirType);


/*!
 *****************************************************************************
 * Return an EIR content
 *
 * \param[in]  bluetooth: Bluetooth type
 * \param[in]  eirType:   EIR type to retrieve
 * \param[out] bufData:   NDEF buffer pointing to the EIR data
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefBluetoothGetEirData(const ndefTypeBluetooth *bluetooth, uint8_t eirType, ndefConstBuffer *bufData);


/*!
 *****************************************************************************
 * Copy an EIR content with bytes in reversed order in the ndefBuffer
 *
 * \param[in]  bluetooth:       Bluetooth type
 * \param[in]  eirType:         EIR type to retrieve
 * \param[out] bufDataReversed: NDEF buffer to store the reversed EIR
 *
 * In case the NDEF buffer is not long enough, its length is updated with
 * the actual length required and the function returns ERR_NOMEM.
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefBluetoothGetEirDataReversed(const ndefTypeBluetooth *bluetooth, uint8_t eirType, ndefBuffer *bufDataReversed);


/*!
 *****************************************************************************
 * Reset a Bluetooth type
 *
 * \param[in] bluetooth: Bluetooth type
 *
 * This function resets each field of the ndefTypeBluetooth structure
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefBluetoothReset(ndefTypeBluetooth *bluetooth);


/*!
 *****************************************************************************
 * Initialize a Bluetooth Basic Rate/Enhanced Data Rate type
 *
 * \param[out] type:      Type to initialize
 * \param[in]  bluetooth: Bluetooth type
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefBluetoothBrEdrInit(ndefType *type, const ndefTypeBluetooth *bluetooth);


/*!
 *****************************************************************************
 * Initialize a Bluetooth Low Energy type
 *
 * \param[out] type:      Type to initialize
 * \param[in]  bluetooth: Bluetooth type
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefBluetoothLeInit(ndefType *type, const ndefTypeBluetooth *bluetooth);


/*!
 *****************************************************************************
 * Initialize a Bluetooth Secure initiatorBitRates/EDR type
 *
 * \param[out] type:      Type to initialize
 * \param[in]  bluetooth: Bluetooth type
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefBluetoothSecureBrEdrInit(ndefType *type, const ndefTypeBluetooth *bluetooth);


/*!
 *****************************************************************************
 * Initialize a Bluetooth Secure Low Energy type
 *
 * \param[out] type:      Type to initialize
 * \param[in]  bluetooth: Bluetooth type
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefBluetoothSecureLeInit(ndefType *type, const ndefTypeBluetooth *bluetooth);


/*!
 *****************************************************************************
 * Get Bluetooth type content
 *
 * \param[in]  type:      Type to get information from
 * \param[out] bluetooth: Bluetooth type
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefGetBluetooth(const ndefType *type, ndefTypeBluetooth *bluetooth);


/*!
 *****************************************************************************
 * Convert an NDEF record to a Bluetooth type
 *
 * \param[in]  record: Record to convert
 * \param[out] type:   The converted type
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRecordToBluetooth(const ndefRecord *record, ndefType *type);


/*!
 *****************************************************************************
 * Convert a Bluetooth type to an NDEF record
 *
 * \param[in]  type:   Type to convert
 * \param[out] record: The converted type
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefBluetoothToRecord(const ndefType *type, ndefRecord *record);



#endif /* NDEF_TYPE_BLUETOOTH_H */

/**
  * @}
  *
  */
