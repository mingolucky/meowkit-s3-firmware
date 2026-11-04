// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef _I2C_CLASS_H__
#define _I2C_CLASS_H__

#if __has_include(<driver/i2c.h>)
#include <driver/i2c.h>
#endif

#include <cstdint>
#include <cstddef>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

/**
 * @brief ESP-IDF native I2C master driver.
 * 
 * NOTE: The freq parameter in register-level methods is currently ignored.
 * ESP-IDF legacy I2C API uses the frequency set at begin() time.
 * This parameter is retained for API compatibility.
 */
class I2C_Class
{
public:
    /// Setup and begin I2C peripheral.
    bool begin(i2c_port_t port_num, int pin_sda, int pin_scl);

    /// Begin with previously stored parameters.
    bool begin(void);

    /// Release I2C driver.
    bool release(void) const;

    /// Atomic raw transaction, optionally with a repeated START before reading.
    /// Uses the same mutex as register access on this shared bus.
    bool transfer(std::uint8_t address, const std::uint8_t* tx, std::size_t txLength,
                  std::uint8_t* rx, std::size_t rxLength,
                  std::uint32_t timeoutMs = 100) const;

    // ── Register-level operations (complete START…STOP transaction) ──

    /// Write multiple bytes to register.
    bool writeRegister(std::uint8_t address, std::uint8_t reg,
                       const std::uint8_t* data, std::size_t length,
                       std::uint32_t freq = 0) const;

    /// Read multiple bytes from register.
    bool readRegister(std::uint8_t address, std::uint8_t reg,
                      std::uint8_t* result, std::size_t length,
                      std::uint32_t freq = 0) const;

    /// Write single byte to register.
    bool writeRegister8(std::uint8_t address, std::uint8_t reg,
                        std::uint8_t data, std::uint32_t freq = 0) const;

    /// Read single byte from register. Returns 0 on error.
    std::uint8_t readRegister8(std::uint8_t address, std::uint8_t reg,
                               std::uint32_t freq = 0) const;

    /// Read-modify-write: set bits.
    bool bitOn(std::uint8_t address, std::uint8_t reg,
               std::uint8_t mask, std::uint32_t freq = 0) const;

    /// Read-modify-write: clear bits.
    bool bitOff(std::uint8_t address, std::uint8_t reg,
                std::uint8_t mask, std::uint32_t freq = 0) const;

    /// Scan single 7-bit address.
    bool scanID(std::uint8_t addr, std::uint32_t freq = 100000) const;

    /// Scan addresses 8–119; result must be bool[128].
    void scanID(bool* result, std::uint32_t freq = 100000) const;

    // ── Accessors ──
    i2c_port_t getPort(void) const { return _port_num; }
    int8_t     getSDA(void)  const { return _pin_sda;  }
    int8_t     getSCL(void)  const { return _pin_scl;  }
    bool       isEnabled(void) const { return _port_num >= 0; }

private:
    i2c_port_t        _port_num = (i2c_port_t)-1;
    int8_t            _pin_sda  = -1;
    int8_t            _pin_scl  = -1;
    SemaphoreHandle_t _mutex    = nullptr;
};



/// Internal I2C bus (shared by on-board peripherals)
extern I2C_Class In_I2C;
/// External I2C bus (expansion header, if any)
extern I2C_Class Ex_I2C;

/**
 * @brief Base class for I2C peripherals.
 * 
 * Inherit from this to get simplified register access.
 */
class I2C_Device
{
public:
    I2C_Device(std::uint8_t i2c_addr, std::uint32_t freq, I2C_Class* i2c = &In_I2C)
        : _i2c(i2c), _freq(freq), _addr(i2c_addr), _init(false) {}

    void setPort(I2C_Class* i2c)           { _i2c = i2c; }
    void setClock(std::uint32_t freq)      { _freq = freq; }
    void setAddress(std::uint8_t i2c_addr) { _addr = i2c_addr; }
    std::uint8_t getAddress(void) const    { return _addr; }

    bool writeRegister8(std::uint8_t reg, std::uint8_t data) const {
        return _i2c->writeRegister8(_addr, reg, data, _freq);
    }

    std::uint8_t readRegister8(std::uint8_t reg) const {
        return _i2c->readRegister8(_addr, reg, _freq);
    }

    bool writeRegister8Array(const std::uint8_t* reg_data_array, std::size_t length) const;

    bool writeRegister(std::uint8_t reg, const std::uint8_t* data, std::size_t length) const {
        return _i2c->writeRegister(_addr, reg, data, length, _freq);
    }

    bool readRegister(std::uint8_t reg, std::uint8_t* result, std::size_t length) const {
        return _i2c->readRegister(_addr, reg, result, length, _freq);
    }

    bool bitOn(std::uint8_t reg, std::uint8_t mask) const {
        return _i2c->bitOn(_addr, reg, mask, _freq);
    }

    bool bitOff(std::uint8_t reg, std::uint8_t mask) const {
        return _i2c->bitOff(_addr, reg, mask, _freq);
    }

    bool isEnabled(void) const { return _init; }

protected:
    I2C_Class*    _i2c;
    std::uint32_t _freq;
    std::uint8_t  _addr;
    bool          _init;
};

#endif
