/**
 * @file I2C_Class.cpp
 * @brief ESP-IDF native I2C master driver for MeowKit ESP32-S3
 */

#include "I2C_Class.hpp"
#include "../config.h"
#include <driver/i2c.h>
#include <esp_log.h>

static const char* TAG = "I2C";

// ── Global I2C bus instances ──
I2C_Class In_I2C;
I2C_Class Ex_I2C;

// ── I2C_Class implementation ──

bool I2C_Class::begin(i2c_port_t port_num, int sda, int scl)
{
    _port_num = port_num;
    _pin_sda  = sda;
    _pin_scl  = scl;
    return begin();
}

bool I2C_Class::begin(void)
{
    if (_port_num < 0 || _pin_sda < 0 || _pin_scl < 0) {
        return false;
    }

    if (!_mutex) _mutex = xSemaphoreCreateMutex();

    i2c_config_t conf = {};
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = (gpio_num_t)_pin_sda;
    conf.scl_io_num = (gpio_num_t)_pin_scl;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2CSPEED;
#if SOC_I2C_SUPPORT_SLAVE
    conf.clk_flags = 0;
#endif

    esp_err_t err = i2c_param_config(_port_num, &conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "param config failed: %s", esp_err_to_name(err));
        return false;
    }

    err = i2c_driver_install(_port_num, I2C_MODE_MASTER, 0, 0, 0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "driver install failed: %s", esp_err_to_name(err));
        return false;
    }

    return true;
}

bool I2C_Class::release(void) const
{
    if (_port_num < 0) return false;
    return i2c_driver_delete(_port_num) == ESP_OK;
}

bool I2C_Class::transfer(std::uint8_t address, const std::uint8_t* tx,
                         std::size_t txLength, std::uint8_t* rx,
                         std::size_t rxLength, std::uint32_t timeoutMs) const
{
    if (!isEnabled() || !_mutex || address > 0x7f || (!txLength && !rxLength) ||
        (txLength && !tx) || (rxLength && !rx)) return false;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (!cmd) return false;
    esp_err_t err = ESP_OK;
    auto step = [&](esp_err_t value) { if (err == ESP_OK) err = value; };
    if (txLength) {
        step(i2c_master_start(cmd));
        step(i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE, true));
        step(i2c_master_write(cmd, tx, txLength, true));
    }
    if (rxLength) {
        step(i2c_master_start(cmd));
        step(i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_READ, true));
        if (rxLength > 1) step(i2c_master_read(cmd, rx, rxLength - 1, I2C_MASTER_ACK));
        step(i2c_master_read_byte(cmd, rx + rxLength - 1, I2C_MASTER_NACK));
    }
    step(i2c_master_stop(cmd));
    const TickType_t ticks = pdMS_TO_TICKS(timeoutMs ? timeoutMs : 1) + 1;
    if (err == ESP_OK) {
        if (xSemaphoreTake(_mutex, ticks) == pdTRUE) {
            err = i2c_master_cmd_begin(_port_num, cmd, ticks);
            xSemaphoreGive(_mutex);
        } else err = ESP_ERR_TIMEOUT;
    }
    i2c_cmd_link_delete(cmd);
    return err == ESP_OK;
}

// ── Register-level operations ──

bool I2C_Class::writeRegister(std::uint8_t address, std::uint8_t reg,
                              const std::uint8_t* data, std::size_t length,
                              std::uint32_t freq) const
{
    (void)freq;
    if (!data || length == 0) return false;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write(cmd, data, length, true);
    i2c_master_stop(cmd);

    if (_mutex) xSemaphoreTake(_mutex, portMAX_DELAY);
    esp_err_t err = i2c_master_cmd_begin(_port_num, cmd, pdMS_TO_TICKS(100));
    if (_mutex) xSemaphoreGive(_mutex);
    i2c_cmd_link_delete(cmd);
    return err == ESP_OK;
}

bool I2C_Class::readRegister(std::uint8_t address, std::uint8_t reg,
                             std::uint8_t* result, std::size_t length,
                             std::uint32_t freq) const
{
    (void)freq;
    if (!result || length == 0) return false;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    // Write phase: send register address
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    // Read phase: repeated start
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_READ, true);
    if (length > 1) {
        i2c_master_read(cmd, result, length - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, result + length - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);

    if (_mutex) xSemaphoreTake(_mutex, portMAX_DELAY);
    esp_err_t err = i2c_master_cmd_begin(_port_num, cmd, pdMS_TO_TICKS(100));
    if (_mutex) xSemaphoreGive(_mutex);
    i2c_cmd_link_delete(cmd);
    return err == ESP_OK;
}

bool I2C_Class::writeRegister8(std::uint8_t address, std::uint8_t reg,
                               std::uint8_t data, std::uint32_t freq) const
{
    return writeRegister(address, reg, &data, 1, freq);
}

std::uint8_t I2C_Class::readRegister8(std::uint8_t address, std::uint8_t reg,
                                      std::uint32_t freq) const
{
    std::uint8_t result = 0;
    readRegister(address, reg, &result, 1, freq);
    return result;
}

bool I2C_Class::bitOn(std::uint8_t address, std::uint8_t reg,
                      std::uint8_t mask, std::uint32_t freq) const
{
    std::uint8_t value = 0;
    if (!readRegister(address, reg, &value, 1, freq)) {
        return false;  // Read failed, don't write garbage
    }
    value |= mask;
    return writeRegister8(address, reg, value, freq);
}

bool I2C_Class::bitOff(std::uint8_t address, std::uint8_t reg,
                       std::uint8_t mask, std::uint32_t freq) const
{
    std::uint8_t value = 0;
    if (!readRegister(address, reg, &value, 1, freq)) {
        return false;  // Read failed, don't write garbage
    }
    value &= ~mask;
    return writeRegister8(address, reg, value, freq);
}

// ── I2C scan ──

bool I2C_Class::scanID(std::uint8_t addr, std::uint32_t freq) const
{
    (void)freq;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);

    if (_mutex) xSemaphoreTake(_mutex, portMAX_DELAY);
    esp_err_t err = i2c_master_cmd_begin(_port_num, cmd, pdMS_TO_TICKS(20));
    if (_mutex) xSemaphoreGive(_mutex);
    i2c_cmd_link_delete(cmd);
    return err == ESP_OK;
}

void I2C_Class::scanID(bool* result, std::uint32_t freq) const
{
    // 0x00-0x07: reserved, skip to prevent system hang
    for (int i = 0; i < 8; ++i) {
        result[i] = false;
    }
    // 0x08-0x77: valid 7-bit addresses
    for (int i = 8; i < 0x78; ++i) {
        result[i] = scanID((std::uint8_t)i, freq);
    }
    // 0x78-0x7F: reserved
    for (int i = 0x78; i < 128; ++i) {
        result[i] = false;
    }
}

// ── I2C_Device helpers ──

bool I2C_Device::writeRegister8Array(const std::uint8_t* reg_data_array,
                                     std::size_t length) const
{
    for (std::size_t i = 0; i + 1 < length; i += 2) {
        if (!_i2c->writeRegister8(_addr, reg_data_array[i], reg_data_array[i + 1], _freq)) {
            return false;
        }
    }
    return true;
}
