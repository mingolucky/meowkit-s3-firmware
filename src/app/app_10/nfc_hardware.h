#pragma once
#include "../../bsp/config.h"
#include <rfal_rfst25r3916.h>

namespace meow_nfc {
static_assert(HAL_NFC_IRQ_PIN == -1, "This board adapter uses IRQ register polling");
static_assert(HAL_NFC_XTAL_HZ == 27120000UL, "RFAL timing requires the 27.12 MHz crystal");
static_assert(HAL_NFC_SUPPLY_MV == 3300, "ST25R3916 board profile is for 3.3 V");
static_assert(HAL_I2C_ADDR_ST25R3916 == 0x50, "ST25R3916 I2C address is fixed");

// Warm-boot recovery follows M5Stack UnitST25R3916::begin(). Keep RFAL's
// chip-specific reset/oscillator/regulator setup instead of M5's AAT profile.
inline ReturnCode prepareSt25r3916(RfalRfST25R3916Class& rf) {
    ReturnCode rc = rf.st25r3916ExecuteCommand(ST25R3916_CMD_STOP);
    if (rc == ERR_NONE) rc = rf.st25r3916ClrRegisterBits(ST25R3916_REG_OP_CONTROL,
        ST25R3916_REG_OP_CONTROL_tx_en | ST25R3916_REG_OP_CONTROL_rx_en | ST25R3916_REG_OP_CONTROL_wu);
    if (rc == ERR_NONE) delay(2);
    return rc;
}
inline ReturnCode configureSt25r3916Board(RfalRfST25R3916Class& rf) {
    // AAT_A/B and MCU_CLK are unconnected in the supplied schematic.
    ReturnCode rc = rf.st25r3916ChangeRegisterBits(ST25R3916_REG_IO_CONF2,
        ST25R3916_REG_IO_CONF2_aat_en | ST25R3916_REG_IO_CONF2_sup3V | ST25R3916_REG_IO_CONF2_io_drv_lvl,
        ST25R3916_REG_IO_CONF2_sup3V_3V | ST25R3916_REG_IO_CONF2_io_drv_lvl);
    if (rc == ERR_NONE) rc = rf.st25r3916ChangeRegisterBits(ST25R3916_REG_IO_CONF1, 0x07, 0x07);
    return rc;
}
} // namespace meow_nfc
