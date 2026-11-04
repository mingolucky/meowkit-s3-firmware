/**
  ******************************************************************************
  * @file           : rfal_default_configuration.h
  * @brief          : This file contains definitions for RFAL Default Configuration
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __RFAL_DEFAULT_CONFIG_H__
#define __RFAL_DEFAULT_CONFIG_H__



/* Exported constants --------------------------------------------------------*/
/** @defgroup Platform_Exported_Constants
 *  @{
 */

/**
  * @}
  */

/**
  * @}
  */

/*
******************************************************************************
* RFAL FEATURES CONFIGURATION
******************************************************************************
*/
#define RFAL_FEATURE_LISTEN_MODE               true       /*!< Enable/Disable RFAL support for Listen Mode                               */
#define RFAL_FEATURE_WAKEUP_MODE               false       /*!< Enable/Disable RFAL support for the Wake-Up mode                          */
#define RFAL_FEATURE_LOWPOWER_MODE             false      /*!< Enable/Disable RFAL support for the Low Power mode                        */
#define RFAL_FEATURE_NFCA                      true       /*!< Enable/Disable RFAL support for NFC-A (ISO14443A)                         */
#define RFAL_FEATURE_NFCB                      true       /*!< Enable/Disable RFAL support for NFC-B (ISO14443B)                         */
#define RFAL_FEATURE_NFCF                      true       /*!< Enable/Disable RFAL support for NFC-F (FeliCa)                            */
#define RFAL_FEATURE_NFCV                      true       /*!< Enable/Disable RFAL support for NFC-V (ISO15693)                          */
#define RFAL_FEATURE_T1T                       true       /*!< Enable/Disable RFAL support for T1T (Topaz)                               */
#define RFAL_FEATURE_T2T                       true       /*!< Enable/Disable RFAL support for T2T                                       */
#define RFAL_FEATURE_T4T                       true       /*!< Enable/Disable RFAL support for T4T                                       */
#define RFAL_FEATURE_ST25TB                    true       /*!< Enable/Disable RFAL support for ST25TB                                    */
#define RFAL_FEATURE_ST25xV                    true       /*!< Enable/Disable RFAL support for  ST25TV/ST25DV                            */
#define RFAL_FEATURE_DYNAMIC_ANALOG_CONFIG     false      /*!< Enable/Disable Analog Configs to be dynamically updated (RAM)             */
#define RFAL_FEATURE_DPO                       false      /*!< Enable/Disable RFAL Dynamic Power Output support                          */
#define RFAL_FEATURE_ISO_DEP                   true       /*!< Enable/Disable RFAL support for ISO-DEP (ISO14443-4)                      */
#define RFAL_FEATURE_ISO_DEP_POLL              true       /*!< Enable/Disable RFAL support for Poller mode (PCD) ISO-DEP (ISO14443-4)    */
#define RFAL_FEATURE_ISO_DEP_LISTEN            true       /*!< Enable/Disable RFAL support for Listen mode (PICC) ISO-DEP (ISO14443-4)   */
#define RFAL_FEATURE_NFC_DEP                   true       /*!< Enable/Disable RFAL support for NFC-DEP (NFCIP1/P2P)                      */

#define RFAL_FEATURE_ISO_DEP_IBLOCK_MAX_LEN    256U       /*!< ISO-DEP I-Block max length. Please use values as defined by rfalIsoDepFSx */
#define RFAL_FEATURE_NFC_DEP_BLOCK_MAX_LEN     254U       /*!< NFC-DEP Block/Payload length. Allowed values: 64, 128, 192, 254           */
#define RFAL_FEATURE_NFC_RF_BUF_LEN            258U       /*!< RF buffer length used by RFAL NFC layer                                   */

#define RFAL_FEATURE_ISO_DEP_APDU_MAX_LEN      512U       /*!< ISO-DEP APDU max length. Please use multiples of I-Block max length       */
#define RFAL_FEATURE_NFC_DEP_PDU_MAX_LEN       512U       /*!< NFC-DEP PDU max length.                                                   */

#define RFAL_SUPPORT_MODE_POLL_NFCA                true          /*!< RFAL Poll NFCA mode support switch    */
#define RFAL_SUPPORT_MODE_POLL_NFCB                true          /*!< RFAL Poll NFCB mode support switch    */
#define RFAL_SUPPORT_MODE_POLL_NFCF                true         /*!< RFAL Poll NFCF mode support switch    */
#define RFAL_SUPPORT_MODE_POLL_NFCV                true          /*!< RFAL Poll NFCV mode support switch    */
#define RFAL_SUPPORT_MODE_POLL_ACTIVE_P2P          true         /*!< RFAL Poll AP2P mode support switch    */
#define RFAL_SUPPORT_MODE_LISTEN_NFCA              false         /*!< RFAL Listen NFCA mode support switch  */
#define RFAL_SUPPORT_MODE_LISTEN_NFCB              false         /*!< RFAL Listen NFCB mode support switch  */
#define RFAL_SUPPORT_MODE_LISTEN_NFCF              false         /*!< RFAL Listen NFCF mode support switch  */
#define RFAL_SUPPORT_MODE_LISTEN_ACTIVE_P2P        false         /*!< RFAL Listen AP2P mode support switch  */

/*******************************************************************************/
/*! RFAL supported Card Emulation (CE)        */
#define RFAL_SUPPORT_CE                            ( RFAL_SUPPORT_MODE_LISTEN_NFCA || RFAL_SUPPORT_MODE_LISTEN_NFCB || RFAL_SUPPORT_MODE_LISTEN_NFCF )

/*! RFAL supported Reader/Writer (RW)         */
#define RFAL_SUPPORT_RW                            ( RFAL_SUPPORT_MODE_POLL_NFCA || RFAL_SUPPORT_MODE_POLL_NFCB || RFAL_SUPPORT_MODE_POLL_NFCF || RFAL_SUPPORT_MODE_POLL_NFCV )

/*! RFAL support for Active P2P (AP2P)        */
#define RFAL_SUPPORT_AP2P                          ( RFAL_SUPPORT_MODE_POLL_ACTIVE_P2P || RFAL_SUPPORT_MODE_LISTEN_ACTIVE_P2P )


/*******************************************************************************/
#define RFAL_SUPPORT_BR_RW_106                      true         /*!< RFAL RW  106 Bit Rate support switch   */
#define RFAL_SUPPORT_BR_RW_212                      true         /*!< RFAL RW  212 Bit Rate support switch   */
#define RFAL_SUPPORT_BR_RW_424                      true         /*!< RFAL RW  424 Bit Rate support switch   */
#define RFAL_SUPPORT_BR_RW_848                      true         /*!< RFAL RW  848 Bit Rate support switch   */
#define RFAL_SUPPORT_BR_RW_1695                     false        /*!< RFAL RW 1695 Bit Rate support switch   */
#define RFAL_SUPPORT_BR_RW_3390                     false        /*!< RFAL RW 3390 Bit Rate support switch   */
#define RFAL_SUPPORT_BR_RW_6780                     false        /*!< RFAL RW 6780 Bit Rate support switch   */
#define RFAL_SUPPORT_BR_RW_13560                    false        /*!< RFAL RW 6780 Bit Rate support switch   */


/*******************************************************************************/
#define RFAL_SUPPORT_BR_AP2P_106                    true         /*!< RFAL AP2P  106 Bit Rate support switch */
#define RFAL_SUPPORT_BR_AP2P_212                    true         /*!< RFAL AP2P  212 Bit Rate support switch */
#define RFAL_SUPPORT_BR_AP2P_424                    true         /*!< RFAL AP2P  424 Bit Rate support switch */
#define RFAL_SUPPORT_BR_AP2P_848                    false        /*!< RFAL AP2P  848 Bit Rate support switch */


/*******************************************************************************/
#define RFAL_SUPPORT_BR_CE_A_106                    true         /*!< RFAL CE A 106 Bit Rate support switch  */
#define RFAL_SUPPORT_BR_CE_A_212                    false        /*!< RFAL CE A 212 Bit Rate support switch  */
#define RFAL_SUPPORT_BR_CE_A_424                    false        /*!< RFAL CE A 424 Bit Rate support switch  */
#define RFAL_SUPPORT_BR_CE_A_848                    false        /*!< RFAL CE A 848 Bit Rate support switch  */


/*******************************************************************************/
#define RFAL_SUPPORT_BR_CE_B_106                    false        /*!< RFAL CE B 106 Bit Rate support switch  */
#define RFAL_SUPPORT_BR_CE_B_212                    false        /*!< RFAL CE B 212 Bit Rate support switch  */
#define RFAL_SUPPORT_BR_CE_B_424                    false        /*!< RFAL CE B 424 Bit Rate support switch  */
#define RFAL_SUPPORT_BR_CE_B_848                    false        /*!< RFAL CE B 848 Bit Rate support switch  */


/*******************************************************************************/
#define RFAL_SUPPORT_BR_CE_F_212                    true         /*!< RFAL CE F 212 Bit Rate support switch  */
#define RFAL_SUPPORT_BR_CE_F_424                    true         /*!< RFAL CE F 424 Bit Rate support switch  */

/*
******************************************************************************
* RFAL CUSTOM SETTINGS
******************************************************************************
  Custom analog configs are used to cope with Automatic Antenna Tuning (AAT)
  that are optimized differently for each board.
*/
#define RFAL_ANALOG_CONFIG_CUSTOM                         /*!< Use Custom Analog Configs when defined                                    */

/* Exported variables --------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */


#endif /* __RFAL_DEFAULT_CONFIG_H__*/

