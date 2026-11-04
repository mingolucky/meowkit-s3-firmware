
/**
  ******************************************************************************
  * @file           : ndef_type_bluetooth.cpp
  * @brief          : NDEF Bluetooth type
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

/*
 ******************************************************************************
 * INCLUDES
 ******************************************************************************
 */

#include "ndef_record.h"
#include "ndef_types.h"
#include "ndef_type_bluetooth.h"
#include "nfc_utils.h"


#if NDEF_TYPE_BLUETOOTH_SUPPORT


/*
 ******************************************************************************
 * GLOBAL DEFINES
 ******************************************************************************
 */

/*! Bluetooth Payload minimal length */
#define NDEF_BLUETOOTH_BREDR_PAYLOAD_LENGTH_MIN            8U
#define NDEF_BLUETOOTH_SECURE_LE_PAYLOAD_LENGTH_MIN        2U
#define NDEF_BLUETOOTH_PAYLOAD_LENGTH_MIN                  (MIN(NDEF_BLUETOOTH_BREDR_PAYLOAD_LENGTH_MIN, NDEF_BLUETOOTH_SECURE_LE_PAYLOAD_LENGTH_MIN))


/*! EIR length */
#define NDEF_BT_EIR_DEVICE_ADDRESS_SIZE                    6U
#define NDEF_BT_EIR_BLE_DEVICE_ADDRESS_SIZE                6U
#define NDEF_BT_EIR_DEVICE_CLASS_SIZE                      3U
#define NDEF_BT_EIR_SIMPLE_PAIRING_HASH_SIZE              16U
#define NDEF_BT_EIR_SIMPLE_PAIRING_RANDOMIZER_SIZE        16U
#define NDEF_BT_EIR_SECURE_CO_CONFIRMATION_VALUE_SIZE     16U
#define NDEF_BT_EIR_SECURE_CO_RANDOM_VALUE_SIZE           16U
#define NDEF_BT_EIR_SECURITY_MANAGER_TK_SIZE              16U
#define NDEF_BT_EIR_SLAVE_CONNECTION_INTERVAL_RANGE_SIZE  (2U * sizeof(uint16_t))


/*! Enable EIR length check while decoding payload */
#define NDEF_BLUETOOTH_CHECK_REFERENCE_LENGTH

/*! Encode 0-length data EIRs */
#define NDEF_BLUETOOTH_ENCODE_EMPTY_DATA_EIR

/*! EIR Length-Type-Data fields offsets */
#define NDEF_BT_EIR_LENGTH_OFFSET                   0U
#define NDEF_BT_EIR_TYPE_OFFSET                     1U
#define NDEF_BT_EIR_DATA_OFFSET                     2U


/*
 ******************************************************************************
 * LOCAL TYPES
 ******************************************************************************
 */


/*
 ******************************************************************************
 * LOCAL VARIABLES
 ******************************************************************************
 */


/*! Bluetooth Type strings */
static const uint8_t ndefTypeBluetoothBrEdr[]       = "application/vnd.bluetooth.ep.oob";        /*!< Bluetooth initiatorBitRates/EDR Out-Of-Band Record Type            */
static const uint8_t ndefTypeBluetoothLe[]          = "application/vnd.bluetooth.le.oob";        /*!< Bluetooth Low Energy Out-Of-Band Record Type        */
static const uint8_t ndefTypeBluetoothSecureBrEdr[] = "application/vnd.bluetooth.secure.ep.oob"; /*!< Bluetooth Secure initiatorBitRates/EDR Out-Of-Band Record Type     */
static const uint8_t ndefTypeBluetoothSecureLe[]    = "application/vnd.bluetooth.secure.le.oob"; /*!< Bluetooth Secure Low Energy Out-Of-Band Record type */

const ndefConstBuffer8 bufMediaTypeBluetoothBrEdr       = { ndefTypeBluetoothBrEdr,       sizeof(ndefTypeBluetoothBrEdr) - 1U };       /*!< Bluetooth initiatorBitRates/EDR Record Type buffer            */
const ndefConstBuffer8 bufMediaTypeBluetoothLe          = { ndefTypeBluetoothLe,          sizeof(ndefTypeBluetoothLe) - 1U };          /*!< Bluetooth Low Energy Record Type buffer        */
const ndefConstBuffer8 bufMediaTypeBluetoothSecureBrEdr = { ndefTypeBluetoothSecureBrEdr, sizeof(ndefTypeBluetoothSecureBrEdr) - 1U }; /*!< Bluetooth Secure initiatorBitRates/EDR Record Type buffer     */
const ndefConstBuffer8 bufMediaTypeBluetoothSecureLe    = { ndefTypeBluetoothSecureLe,    sizeof(ndefTypeBluetoothSecureLe) - 1U };    /*!< Bluetooth Secure Low Energy Record Type buffer */


/*
 ******************************************************************************
 * LOCAL FUNCTION PROTOTYPES
 ******************************************************************************
 */


/*
 ******************************************************************************
 * GLOBAL FUNCTIONS
 ******************************************************************************
 */


/*
 * Bluetooth OOB
 */


/* EIR Helper functions */


/*****************************************************************************/
uint8_t ndefBluetoothEirLength(const uint8_t *eir)
{
  if (eir == NULL) {
    return 0;
  }

  uint8_t length = eir[NDEF_BT_EIR_LENGTH_OFFSET];
  /* Check the EIR contains something */
  if (length != 0U) {
    /* Add the Length byte that is not included in the EIR length */
    length += (uint8_t)sizeof(uint8_t);
  }

  return length;
}


/*****************************************************************************/
uint8_t ndefBluetoothEirDataLength(const uint8_t *eir)
{
  if (eir == NULL) {
    return 0;
  }

  uint8_t dataLength = eir[NDEF_BT_EIR_LENGTH_OFFSET];
  /* Check the EIR contains a type */
  if (dataLength > 0U) {
    dataLength -= (uint8_t)sizeof(uint8_t); /* Remove the EIR Type byte */
  }

  return dataLength;
}


/*****************************************************************************/
uint8_t ndefBluetoothEirType(const uint8_t *eir)
{
  uint8_t type = 0;

  if (ndefBluetoothEirLength(eir) != 0U) {
    type = eir[NDEF_BT_EIR_TYPE_OFFSET];
  }

  return type;
}


/*****************************************************************************/
const uint8_t *ndefBluetoothEirData(const uint8_t *eir)
{
  const uint8_t *data = NULL;

  if (ndefBluetoothEirDataLength(eir) != 0U) {
    data = &eir[NDEF_BT_EIR_DATA_OFFSET];
  }

  return data;
}


/*****************************************************************************/
ReturnCode ndefBluetoothEirDataToBuffer(const uint8_t *eir, ndefConstBuffer *bufEir)
{
  if ((eir == NULL) || (bufEir == NULL)) {
    return ERR_PARAM;
  }

  bufEir->buffer = ndefBluetoothEirData(eir);
  bufEir->length = ndefBluetoothEirDataLength(eir);

  return ERR_NONE;
}


/*****************************************************************************/
ReturnCode ndefBluetoothSetEir(ndefTypeBluetooth *bluetooth, const uint8_t *eir)
{
  if ((bluetooth == NULL) || (eir == NULL)) {
    return ERR_PARAM;
  }

  /* Find first free EIR */
  for (uint32_t i = 0; i < (uint32_t)SIZEOF_ARRAY(bluetooth->eir); i++) {
    /* Append it */
    if (bluetooth->eir[i] == NULL) {
      bluetooth->eir[i] = eir;
      return ERR_NONE;
    }
    /* Or update existing one */
    else if (ndefBluetoothEirType(bluetooth->eir[i]) == ndefBluetoothEirType(eir)) {
      bluetooth->eir[i] = eir;
      return ERR_NONE;
    } else {
      /* MISRA 15.7 - Empty else */
    }
  }

  return ERR_NOMEM;
}


/*****************************************************************************/
const uint8_t *ndefBluetoothGetEir(const ndefTypeBluetooth *bluetooth, uint8_t eirType)
{
  if (bluetooth == NULL) {
    return NULL;
  }

  /* Find EIR with this type */
  for (uint32_t i = 0; i < (uint32_t)SIZEOF_ARRAY(bluetooth->eir); i++) {
    if (ndefBluetoothEirType(bluetooth->eir[i]) == eirType) {
      return bluetooth->eir[i];
    }
  }

  return NULL;
}


/*****************************************************************************/
/* This function copies an array, changing its endianness, useful to convert data to BLE endianness */
static uint8_t *NDEF_BluetoothReverse(uint8_t *dst, const uint8_t *src, uint32_t length)
{
  if ((dst == NULL) || (src == NULL)) {
    return NULL;
  }

  for (uint32_t i = 0; i < length; i++) {
    dst[i] = src[length - i - 1U];
  }

  return dst;
}


/*****************************************************************************/
ReturnCode ndefBluetoothGetEirData(const ndefTypeBluetooth *bluetooth, uint8_t eirType, ndefConstBuffer *bufData)
{
  if ((bluetooth == NULL) || (bufData == NULL)) {
    return ERR_PARAM;
  }

  const uint8_t *eir = ndefBluetoothGetEir(bluetooth, eirType);

  return ndefBluetoothEirDataToBuffer(eir, bufData);
}


/*****************************************************************************/
ReturnCode ndefBluetoothGetEirDataReversed(const ndefTypeBluetooth *bluetooth, uint8_t eirType, ndefBuffer *bufDataReversed)
{
  if ((bluetooth == NULL) || (bufDataReversed == NULL)) {
    return ERR_PARAM;
  }

  const uint8_t *eir = ndefBluetoothGetEir(bluetooth, eirType);

  uint32_t data_length = ndefBluetoothEirDataLength(eir);
  if (data_length > bufDataReversed->length) {
    bufDataReversed->length = data_length;
    return ERR_NOMEM;
  }
  bufDataReversed->length = data_length;

  const uint8_t *eir_data = ndefBluetoothEirData(eir);
  (void)NDEF_BluetoothReverse(bufDataReversed->buffer, eir_data, data_length);

  return ERR_NONE;
}


/*****************************************************************************/
static uint32_t ndefBluetoothPayloadGetLength(const ndefType *type)
{
  const ndefTypeBluetooth *ndefData;
  uint32_t length = 0;

  if ((type == NULL) ||
      ((type->id != NDEF_TYPE_ID_BLUETOOTH_BREDR)        &&
       (type->id != NDEF_TYPE_ID_BLUETOOTH_LE)           &&
       (type->id != NDEF_TYPE_ID_BLUETOOTH_SECURE_BREDR) &&
       (type->id != NDEF_TYPE_ID_BLUETOOTH_SECURE_LE))) {
    return 0;
  }

  ndefData = &type->data.bluetooth;

  /* For both initiatorBitRates/EDR and Secure LE */
  if ((type->id == NDEF_TYPE_ID_BLUETOOTH_BREDR) ||
      (type->id == NDEF_TYPE_ID_BLUETOOTH_SECURE_LE)) {
    length  = sizeof(uint16_t); /* 2 bytes for length */
  }

  /* For initiatorBitRates/EDR only, but no test needed because length is 0 in that case */
  length += ndefData->bufDeviceAddress.length;

  /* Go through all EIRs */
  for (uint32_t i = 0; i < (uint32_t)SIZEOF_ARRAY(ndefData->eir); i++) {
#ifdef NDEF_BLUETOOTH_ENCODE_EMPTY_DATA_EIR
    /* Send all/valid EIRs (even EIRs with data length == 0) */
#else
    /* Send EIRs with data length != 0U only */
    if (ndefBluetoothGetEirDataLength(ndefDtata->eir[i]) != 0U)
#endif
    {
      length += ndefBluetoothEirLength(ndefData->eir[i]);
    }
  }

  return length;
}


/*****************************************************************************/
ReturnCode ndefBluetoothReset(ndefTypeBluetooth *bluetooth)
{
  ndefConstBuffer bufEmpty = { NULL, 0 };

  if (bluetooth == NULL) {
    return ERR_PARAM;
  }

  /* Initialize ndefBuffer */
  bluetooth->bufDeviceAddress = bufEmpty;

  /* Initialize all EIRs */
  for (uint32_t i = 0; i < (uint32_t)SIZEOF_ARRAY(bluetooth->eir); i++) {
    bluetooth->eir[i] = NULL;
  }

  return ERR_NONE;
}


/*****************************************************************************/
static const uint8_t *ndefBluetoothToPayloadItem(const ndefType *type, ndefConstBuffer *bufItem, bool begin)
{
  static uint32_t item = 0;
  static uint32_t eirId = 0;

  const ndefTypeBluetooth *ndefData;

  if ((type == NULL) || (bufItem == NULL) ||
      ((type->id != NDEF_TYPE_ID_BLUETOOTH_BREDR)        &&
       (type->id != NDEF_TYPE_ID_BLUETOOTH_LE)           &&
       (type->id != NDEF_TYPE_ID_BLUETOOTH_SECURE_BREDR) &&
       (type->id != NDEF_TYPE_ID_BLUETOOTH_SECURE_LE))) {
    return NULL;
  }

  ndefData = &type->data.bluetooth;

  bufItem->buffer = NULL;
  bufItem->length = 0;

  /* Initialization */
  if (begin == true) {
    item = 0;
    eirId = 0;
  }

  /* initiatorBitRates/EDR or Secure Low Energy */
  if ((type->id == NDEF_TYPE_ID_BLUETOOTH_BREDR) ||
      (type->id == NDEF_TYPE_ID_BLUETOOTH_SECURE_LE)) {
    if (item == 0U) {
      /* for initiatorBitRates-EDR and Secure LE, Device address & length are managed outside EIR */
      /* First item for NDEF_TYPE_ID_BLUETOOTH_BREDR: Payload length */
      static uint16_t len;
      len = (uint16_t)ndefBluetoothPayloadGetLength(type);
      bufItem->buffer = (const uint8_t *)&len;
      bufItem->length = sizeof(uint16_t);

      item++;
      return bufItem->buffer;
    }
  }
  if (type->id == NDEF_TYPE_ID_BLUETOOTH_BREDR) {
    if (item == 1U) {
      /* for initiatorBitRates-EDR Device address & length are managed outside EIR */
      /* Second item for NDEF_TYPE_ID_BLUETOOTH_BREDR: Device Address */
      bufItem->buffer = ndefData->bufDeviceAddress.buffer;
      bufItem->length = ndefData->bufDeviceAddress.length;

      item++;
      return bufItem->buffer;
    }
  }

  /* Go through all EIRs */
  while (eirId < (uint32_t)SIZEOF_ARRAY(ndefData->eir)) {
#ifdef NDEF_BLUETOOTH_ENCODE_EMPTY_DATA_EIR
    /* Send all/valid EIRs (even EIRs with data length == 0) */
    if (ndefBluetoothEirLength(ndefData->eir[eirId]) != 0U)
#else
    /* Send EIRs with data length != 0U only */
    if (ndefBluetoothEirDataLength(ndefData->eir[eirId]) != 0U)
#endif
    {
      bufItem->buffer = (const uint8_t *)ndefData->eir[eirId];
      bufItem->length = ndefBluetoothEirLength(ndefData->eir[eirId]);

      eirId++;
      return bufItem->buffer;
    }
    eirId++;
  }

  return bufItem->buffer;
}


/*****************************************************************************/
static ReturnCode ndefBluetoothInit(ndefType *type, const ndefTypeBluetooth *bluetooth)
{
  ndefTypeBluetooth *ndefData;

  if ((type == NULL) || (bluetooth == NULL)) {
    return ERR_PARAM;
  }

  /* type->id set by the caller */
  type->getPayloadLength = ndefBluetoothPayloadGetLength;
  type->getPayloadItem   = ndefBluetoothToPayloadItem;
  type->typeToRecord     = ndefBluetoothToRecord;
  ndefData               = &type->data.bluetooth;

  /* Copy in a bulk */
  (void)ST_MEMCPY(ndefData, bluetooth, sizeof(ndefTypeBluetooth));

  return ERR_NONE;
}


/*****************************************************************************/
ReturnCode ndefBluetoothBrEdrInit(ndefType *type, const ndefTypeBluetooth *bluetooth)
{
  if (type == NULL) {
    return ERR_PARAM;
  }

  /* Initialize a Basic Rate/Enhanced Data Rate type */
  type->id = NDEF_TYPE_ID_BLUETOOTH_BREDR;

  return ndefBluetoothInit(type, bluetooth);
}


/*****************************************************************************/
ReturnCode ndefBluetoothLeInit(ndefType *type, const ndefTypeBluetooth *bluetooth)
{
  if (type == NULL) {
    return ERR_PARAM;
  }

  /* Initialize a Bluetooth Low Energy type */
  type->id = NDEF_TYPE_ID_BLUETOOTH_LE;

  return ndefBluetoothInit(type, bluetooth);
}


/*****************************************************************************/
ReturnCode ndefBluetoothSecureBrEdrInit(ndefType *type, const ndefTypeBluetooth *bluetooth)
{
  if (type == NULL) {
    return ERR_PARAM;
  }

  /* Initialize a Secure initiatorBitRates/EDR type */
  type->id = NDEF_TYPE_ID_BLUETOOTH_SECURE_BREDR;

  return ndefBluetoothInit(type, bluetooth);
}


/*****************************************************************************/
ReturnCode ndefBluetoothSecureLeInit(ndefType *type, const ndefTypeBluetooth *bluetooth)
{
  if (type == NULL) {
    return ERR_PARAM;
  }

  /* Initialize a Secure Low Energy type */
  type->id = NDEF_TYPE_ID_BLUETOOTH_SECURE_LE;

  return ndefBluetoothInit(type, bluetooth);
}


/*****************************************************************************/
ReturnCode ndefGetBluetooth(const ndefType *type, ndefTypeBluetooth *bluetooth)
{
  const ndefTypeBluetooth *ndefData;

  if ((type == NULL) || (bluetooth == NULL) ||
      ((type->id != NDEF_TYPE_ID_BLUETOOTH_BREDR)        &&
       (type->id != NDEF_TYPE_ID_BLUETOOTH_LE)           &&
       (type->id != NDEF_TYPE_ID_BLUETOOTH_SECURE_BREDR) &&
       (type->id != NDEF_TYPE_ID_BLUETOOTH_SECURE_LE))) {
    return ERR_PARAM;
  }

  ndefData = &type->data.bluetooth;

  /* Copy in a bulk */
  (void)ST_MEMCPY(bluetooth, ndefData, sizeof(ndefTypeBluetooth));

  return ERR_NONE;
}


/*****************************************************************************/
#ifdef NDEF_BLUETOOTH_CHECK_REFERENCE_LENGTH
static uint32_t ndefBluetoothEirRefLength(uint8_t eirType)
{
  uint32_t length;
  switch (eirType) {
    case NDEF_BT_EIR_FLAGS                            : length = sizeof(uint8_t)                                      ; break;
    case NDEF_BT_EIR_TX_POWER_LEVEL                   : length = sizeof(uint8_t)                                      ; break;
    case NDEF_BT_EIR_DEVICE_CLASS                     : length = NDEF_BT_EIR_DEVICE_CLASS_SIZE                        ; break;
    case NDEF_BT_EIR_SIMPLE_PAIRING_HASH              : length = NDEF_BT_EIR_SIMPLE_PAIRING_HASH_SIZE                 ; break;
    case NDEF_BT_EIR_SIMPLE_PAIRING_RANDOMIZER        : length = NDEF_BT_EIR_SIMPLE_PAIRING_RANDOMIZER_SIZE           ; break;
    case NDEF_BT_EIR_SECURITY_MANAGER_TK_VALUE        : length = NDEF_BT_EIR_SECURITY_MANAGER_TK_SIZE                 ; break;
    case NDEF_BT_EIR_SECURITY_MANAGER_FLAGS           : length = sizeof(uint8_t)                                      ; break;
    case NDEF_BT_EIR_SLAVE_CONNECTION_INTERVAL_RANGE  : length = NDEF_BT_EIR_SLAVE_CONNECTION_INTERVAL_RANGE_SIZE     ; break;
    case NDEF_BT_EIR_LE_DEVICE_ADDRESS                : length = NDEF_BT_EIR_BLE_DEVICE_ADDRESS_SIZE + sizeof(uint8_t); break;
    case NDEF_BT_EIR_LE_ROLE                          : length = sizeof(uint8_t)                                      ; break;
    case NDEF_BT_EIR_LE_SECURE_CONN_CONFIRMATION_VALUE: length = NDEF_BT_EIR_SECURE_CO_CONFIRMATION_VALUE_SIZE        ; break;
    case NDEF_BT_EIR_LE_SECURE_CONN_RANDOM_VALUE      : length = NDEF_BT_EIR_SECURE_CO_RANDOM_VALUE_SIZE              ; break;
    default:
      /* No length constraint on the following EIRs:   */
      /* NDEF_BT_EIR_SERVICE_CLASS_UUID_PARTIAL_16     */
      /* NDEF_BT_EIR_SERVICE_CLASS_UUID_COMPLETE_16    */
      /* NDEF_BT_EIR_SERVICE_CLASS_UUID_PARTIAL_32     */
      /* NDEF_BT_EIR_SERVICE_CLASS_UUID_COMPLETE_32    */
      /* NDEF_BT_EIR_SERVICE_CLASS_UUID_PARTIAL_128    */
      /* NDEF_BT_EIR_SERVICE_CLASS_UUID_COMPLETE_128   */
      /* NDEF_BT_EIR_SHORT_LOCAL_NAME                  */
      /* NDEF_BT_EIR_COMPLETE_LOCAL_NAME               */
      /* NDEF_BT_EIR_SERVICE_SOLICITATION_16           */
      /* NDEF_BT_EIR_SERVICE_SOLICITATION_128          */
      /* NDEF_BT_EIR_SERVICE_DATA                      */
      /* NDEF_BT_EIR_APPEARANCE                        */
      /* NDEF_BT_EIR_MANUFACTURER_DATA                 */
      length = 0;
      break;
  }

  return length;
}
#endif


/*****************************************************************************/
static ReturnCode ndefPayloadToBluetooth(const ndefConstBuffer *bufPayload, ndefTypeId typeId, ndefType *type)
{
  ndefTypeBluetooth *ndefData;

  if ((bufPayload == NULL) || (bufPayload->buffer == NULL) ||
      (type       == NULL)) {
    return ERR_PARAM;
  }

  if (bufPayload->length < NDEF_BLUETOOTH_PAYLOAD_LENGTH_MIN) { /* Bluetooth Payload Min */
    return ERR_PROTO;
  }

  /* MISRA complains that this conditional expression is always false, so comment it out */
  /* if ( (typeId != NDEF_TYPE_ID_BLUETOOTH_BREDR)  &&
       (typeId != NDEF_TYPE_ID_BLUETOOTH_BLE)    &&
       (typeId != NDEF_TYPE_ID_BLUETOOTH_SECURE_LE) )
  {
      return ERR_PARAM;
  } */

  type->id               = typeId;
  type->getPayloadLength = ndefBluetoothPayloadGetLength;
  type->getPayloadItem   = ndefBluetoothToPayloadItem;
  type->typeToRecord     = ndefBluetoothToRecord;
  ndefData               = &type->data.bluetooth;

  /* Reset every field */
  if (ndefBluetoothReset(ndefData) != ERR_NONE) {
    return ERR_PARAM;
  }

  uint32_t offset = 0;

  /* Extract data from the payload */
  if ((typeId == NDEF_TYPE_ID_BLUETOOTH_BREDR) ||
      (typeId == NDEF_TYPE_ID_BLUETOOTH_SECURE_LE)) {
    uint16_t length = bufPayload->buffer[offset];
    NO_WARNING(length);
    offset += sizeof(uint16_t);

    /* Could check length and bufPayload->length match */
  }
  if (typeId == NDEF_TYPE_ID_BLUETOOTH_BREDR) {
    ndefData->bufDeviceAddress.buffer = &bufPayload->buffer[offset];
    ndefData->bufDeviceAddress.length = NDEF_BT_EIR_DEVICE_ADDRESS_SIZE;
    offset += NDEF_BT_EIR_DEVICE_ADDRESS_SIZE;
  }

  while (offset < bufPayload->length) {
    const uint8_t *eir = &bufPayload->buffer[offset];
    uint8_t eir_length = ndefBluetoothEirLength(eir);

    if (eir_length == 0U) {
      break;  /* Leave when find an empty EIR */
    }

    offset += eir_length;

#ifdef NDEF_BLUETOOTH_CHECK_REFERENCE_LENGTH
    uint8_t  eir_data_length = ndefBluetoothEirDataLength(eir);
    uint8_t  eir_type        = ndefBluetoothEirType(eir);
    uint32_t refLength       = ndefBluetoothEirRefLength(eir_type);
    /* Check length match */
    if ((refLength != 0U) &&
        (refLength != eir_data_length)) {
      return ERR_PROTO;
    }
#endif
    ReturnCode err = ndefBluetoothSetEir(ndefData, eir);
    if (err != ERR_NONE) {
      return err;
    }
  }

  /* The client is in charge to check that the mandatory fields are there */

  return ERR_NONE;
}


/*****************************************************************************/
ReturnCode ndefRecordToBluetooth(const ndefRecord *record, ndefType *type)
{
  const ndefType *ndefData;
  ndefTypeId typeId;

  if ((record == NULL) || (type == NULL)) {
    return ERR_PARAM;
  }

  /* "application/vnd.bluetooth.ep.oob" */
  if (ndefRecordTypeMatch(record, NDEF_TNF_MEDIA_TYPE, &bufMediaTypeBluetoothBrEdr)) {
    typeId = NDEF_TYPE_ID_BLUETOOTH_BREDR;
  }
  /* "application/vnd.bluetooth.le.oob" */
  else if (ndefRecordTypeMatch(record, NDEF_TNF_MEDIA_TYPE, &bufMediaTypeBluetoothLe)) {
    typeId = NDEF_TYPE_ID_BLUETOOTH_LE;
  }
  /* "application/vnd.bluetooth.secure.ep.oob" */
  else if (ndefRecordTypeMatch(record, NDEF_TNF_MEDIA_TYPE, &bufMediaTypeBluetoothSecureBrEdr)) {
    typeId = NDEF_TYPE_ID_BLUETOOTH_SECURE_BREDR;
  }
  /* "application/vnd.bluetooth.secure.le.oob" */
  else if (ndefRecordTypeMatch(record, NDEF_TNF_MEDIA_TYPE, &bufMediaTypeBluetoothSecureLe)) {
    typeId = NDEF_TYPE_ID_BLUETOOTH_SECURE_LE;
  } else {
    return ERR_PROTO;
  }

  ndefData = ndefRecordGetNdefType(record);
  if ((ndefData != NULL) && ((ndefData->id == NDEF_TYPE_ID_BLUETOOTH_BREDR)        ||
                             (ndefData->id == NDEF_TYPE_ID_BLUETOOTH_LE)           ||
                             (ndefData->id == NDEF_TYPE_ID_BLUETOOTH_SECURE_BREDR) ||
                             (ndefData->id == NDEF_TYPE_ID_BLUETOOTH_SECURE_LE))
     ) {
    (void)ST_MEMCPY(type, ndefData, sizeof(ndefType));
    return ERR_NONE;
  }

  return ndefPayloadToBluetooth(&record->bufPayload, typeId, type);
}


/*****************************************************************************/
ReturnCode ndefBluetoothToRecord(const ndefType *type, ndefRecord *record)
{
  if ((type == NULL) || (record == NULL)) {
    return ERR_PARAM;
  }

  (void)ndefRecordReset(record);

  if (type->id == NDEF_TYPE_ID_BLUETOOTH_BREDR) {
    /* "application/vnd.bluetooth.ep.oob" */
    (void)ndefRecordSetType(record, NDEF_TNF_MEDIA_TYPE, &bufMediaTypeBluetoothBrEdr);
  } else if (type->id == NDEF_TYPE_ID_BLUETOOTH_LE) {
    /* "application/vnd.bluetooth.le.oob" */
    (void)ndefRecordSetType(record, NDEF_TNF_MEDIA_TYPE, &bufMediaTypeBluetoothLe);
  } else if (type->id == NDEF_TYPE_ID_BLUETOOTH_SECURE_BREDR) {
    /* "application/vnd.bluetooth.secure.ep.oob" */
    (void)ndefRecordSetType(record, NDEF_TNF_MEDIA_TYPE, &bufMediaTypeBluetoothSecureBrEdr);
  } else if (type->id == NDEF_TYPE_ID_BLUETOOTH_SECURE_LE) {
    /* "application/vnd.bluetooth.secure.le.oob" */
    (void)ndefRecordSetType(record, NDEF_TNF_MEDIA_TYPE, &bufMediaTypeBluetoothSecureLe);
  } else {
    return ERR_PROTO;
  }

  if (ndefRecordSetNdefType(record, type) != ERR_NONE) {
    return ERR_PARAM;
  }

  return ERR_NONE;
}

#endif
