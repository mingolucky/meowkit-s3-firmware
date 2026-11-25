/**
 * @file CTP_FT6336.cpp
 * @brief FT6336 Touch Screen Driver
 * @version 
 * @date 
 * 
 * @copyright
 */
#pragma once
#include "Arduino.h"
#include "../i2c/I2C_Class.hpp"

/* Configs */
#define CTP_DEV_ADDR            0x38   // FT6336 I2C address
#define CTP_INT_PIN             -1     // INT pin not used (悬空)
#define CTP_I2C_FREQ            400000 // I2C clock frequency

#define CTP_HORIZON              // Setting for horizontal orientation

#ifdef CTP_HORIZON
    #define CTP_REVERS_XPOS     1
    #define CTP_REVERS_YPOS     0
    #define CTP_REVERS_XY       1
#else
#ifdef CTP_PORTRAIT
    #define CTP_REVERS_XPOS     0
    #define CTP_REVERS_YPOS     1
    #define CTP_REVERS_XY       0
#endif
#endif

#define USING_MAP_TO_CALIBRATE  0   // Option for calibration

class CTP_Class {
    private:
        int _x_pos;
        int _y_pos;
        bool _enable;

        /* I2C related */
        I2C_Class* _i2c;
        uint8_t _dev_addr;

        inline void _I2C_init(I2C_Class* i2c, uint8_t dev_addr)
        {
            _i2c = i2c;
            _dev_addr = dev_addr;
        }

        inline bool _I2C_checkDevAvl()
        {
            // Use scanID to check if device is available
            return !_i2c->scanID(_dev_addr, CTP_I2C_FREQ);
        }

        inline void _I2C_write1Byte(uint8_t addr, uint8_t data)
        {
            _i2c->writeRegister8(_dev_addr, addr, data, CTP_I2C_FREQ);
        }

        inline void _I2C_readBuff(uint8_t addr, int size, uint8_t buff[])
        {
            _i2c->readRegister(_dev_addr, addr, buff, size, CTP_I2C_FREQ);
        }

        inline uint8_t _I2C_read8Bit(uint8_t addr)
        {
            return _i2c->readRegister8(_dev_addr, addr, CTP_I2C_FREQ);
        }

        inline uint16_t _I2C_read12Bit(uint8_t addr)
        {
            uint8_t buff[2];
            _I2C_readBuff(addr, 2, buff);
            return (buff[0] << 4) + buff[1];
        }

        /* CTP init */
        inline void _init()
        {
            _reset_coor();
            _enable = true;

            // Disable auto-sleep on FT6336
            _I2C_write1Byte(0xFE, 0x07);
           printf("[CTP] Auto-sleep disabled\n");
        }

        inline void _reset_coor()
        {
            _x_pos = -1;
            _y_pos = -1;
        }

        inline void _decode_coor(const uint8_t* buff)
        {
            #if CTP_REVERS_XY
                _y_pos = ((buff[0]&0x0F) << 8) | buff[1];
                _x_pos = ((buff[2]&0x0F) << 8) | buff[3];
                #if CTP_REVERS_XPOS
                    _x_pos = -( _x_pos ) + 320;
                #endif
                #if CTP_REVERS_YPOS
                    _y_pos = -( _y_pos ) + 240;
                #endif
            #else
                _x_pos = ((buff[0]&0x0F) << 8) | buff[1];
                _y_pos = ((buff[2]&0x0F) << 8) | buff[3];
            #endif
        }

        inline void _update_coor()
        {
            uint8_t buff[4];
            if (_i2c->readRegister(_dev_addr, 0x03, buff, sizeof(buff), CTP_I2C_FREQ))
                _decode_coor(buff);
            else
                _reset_coor();
        }

    public:
        /**
         * @brief Initialize the CTP with I2C_Class object
         * 
         * @param i2c I2C class interface (e.g., In_I2C)
         */
        inline void init(I2C_Class* i2c)
        {
            _I2C_init(i2c, CTP_DEV_ADDR);

            // Wait until device is available
            int retry = 0;
            while (_I2C_checkDevAvl()) {
                printf("[CTP] Please touch screen to activate\n");
                delay(500);
                if (++retry > 10) {
                    printf("[CTP] Warning: Device not responding, continuing anyway\n");
                    break;
                }
            }
           printf("[CTP] Initialization successful\n");
            _init();
        }

        inline bool isTouched()
        {
            if (!_enable) return false;
            return _I2C_read8Bit(0x02) ? true : false;
        }

        // Read the contact count and first contact from the same I2C sample.
        inline bool readPoint(int& x, int& y)
        {
            uint8_t sample[5];
            if (!_enable || !_i2c->readRegister(_dev_addr, 0x02, sample, sizeof(sample), CTP_I2C_FREQ))
                return false;
            const uint8_t contacts = sample[0] & 0x0f;
            if (contacts == 0 || contacts > 2 || (sample[1] >> 6) == 1) return false;
            _decode_coor(sample + 1);
            x = _x_pos;
            y = _y_pos;
            return true;
        }

        inline void getPos(int& x_pos, int& y_pos)
        {
            _update_coor();
            x_pos = _x_pos;
            y_pos = _y_pos;

            #if USING_MAP_TO_CALIBRATE
                x_pos = map(_x_pos, 6, 258, 0, 280);
                y_pos = map(_y_pos, 15, 230, 0, 240);

                #ifdef CTP_HORIZON
                    x_pos = constrain(x_pos, 0, 280);
                    y_pos = constrain(y_pos, 0, 240);
                #else
                #ifdef CTP_PORTRAIT
                    x_pos = constrain(x_pos, 0, 240);
                    y_pos = constrain(y_pos, 0, 280);
                #endif
                #endif           
            #endif
        }

        inline void printCoordinate()
        {
            if (isTouched()) _update_coor();
            else _reset_coor();

            Serial.printf("X:%03d Y:%03d\n", _x_pos, _y_pos);
        }

        inline void enable() { _enable = true; }
        inline void disable() { _enable = false; }
};
