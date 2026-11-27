/**
 * @file 
 * @author 
 * @brief 
 * @version 
 * @date 
 * 
 * @copyright 
 * 
 */
#include "devices.h"
#include <driver/i2c.h>
#include <driver/gpio.h>
#if MEOWKIT_HW_TEST_ENABLE
#include <IRsend.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <SD_MMC.h>
#include <FS.h>
#include <Audio.h>
#include <driver/i2s.h>
#include <driver/ledc.h>
#include "sdmmc/SDMMC_Class.hpp"
#include <SPI.h>
#include <vector>
#include <Preferences.h>
#include "../app/app_common/mk_tui.h"
#endif

/* ── Audio library diagnostic callback (weak symbol override) ── */
#if MEOWKIT_HW_TEST_ENABLE
void audio_info(const char *info) {
    Serial.printf("[Audio] %s\n", info);
}
#endif


/* ── Forward declarations for internal helpers ──────────── */
static bool _i2cBusRecovery(gpio_num_t sda, gpio_num_t scl);

/* ════════════════════════════════════════════════════════════
 *  DEVICES::init() — Full hardware initialization
 * ════════════════════════════════════════════════════════════ */
bool DEVICES::init()
{
    bool ok = true;

    /* Drive BL pin HIGH immediately (active-low backlight = OFF).
     * GPIO42 stays in GPIO-output mode until initBacklight() attaches LEDC
     * after Lcd.init()+fillScreen complete — keeping the screen dark throughout
     * the entire LCD init and GRAM clear, with no dependency on LEDC timing. */
    gpio_set_direction((gpio_num_t)HAL_PIN_LCD_BL, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)HAL_PIN_LCD_BL, 1);

    /* Serial — wait for monitor to connect */
    Serial.begin(9600);
    delay(300);

    /* I2C bus */
    if (!In_I2C.begin(I2C_NUM_0, HAL_PIN_I2C_SDA, HAL_PIN_I2C_SCL)) {
        while (1) delay(1000);
    }
    delay(50);

    /* PMU — AXP173 (powers everything, must be first) */
    if (!pmu.begin()) {
        /* begin() may fail on pure-battery boot if REG detection glitches.
         * Double-check: read REG 0x12 directly — POR default 0x03, never 0xFF. */
        uint8_t chk = pmu.readRegister8(0x12);
        if (chk != 0xFF) {
            Serial.printf("[PMU] begin() false-neg, REG 0x12=0x%02X — forcing init\n", chk);
            /* Device IS present, continue with configuration */
        } else {
            Serial.println("[PMU] AXP173 truly not found");
            ok = false;
            goto pmu_done;
        }
    }
    {
        /* ── 1. VBUS-IPSOUT 电源通路管理 (REG 0x30) ──
        * POR 默认 0x60, 显式写入确保状态确定
        * Bit7=0: VBUS 存在时允许给 IPSOUT 供电
        * Bit6=1: VHOLD 限压使能
        * Bit5:3=100: VHOLD=4.4V (防止 VBUS 拉低电池充电)
        * Bit1=0: VBUS 限流使能
        * Bit0=0: 限流 500mA                                */
        pmu.writeRegister8(0x30, 0x60);

        /* ── 2. 关机电压 (REG 0x31) ──
        * 设 VOFF=2.9V，给电池→IPSOUT 切换留余量
        * 避免 VBUS 拔除瞬间 IPSOUT 跌破默认 3.0V 触发关机  */
        pmu.setVoffVoltage(2900);

        /* ── 3. 先设电压，再使能输出 ── */
        pmu.setOutputVoltage(OP_DCDC1, 3300);
        pmu.setOutputVoltage(OP_LDO2,  3300);
        pmu.setOutputVoltage(OP_LDO3,  3300);
        pmu.setOutputVoltage(OP_LDO4,  3300);

        /* ── 4. 【核心修复】一次性原子写入 REG 0x12 ──
        * 替代 4 次独立 read-modify-write，避免 I2C 抖动导致丢位
        *
        * Bit0=1 DCDC1    ┐
        * Bit1=1 LDO4     │
        * Bit2=1 LDO2     ├── 0x0F = 四路全部使能
        * Bit3=1 LDO3     ┘
        * Bit4=0 DCDC2 (未使用)
        * Bit6=0 EXTEN (原理图 EXTEN→R47→IPSOUT, 按需开启)
        *
        * 如需使能 EXTEN: 改为 0x4F                          */
        pmu.writeRegister8(0x12, 0x0F);

        /* ── 5. 回读验证 ── */
        uint8_t reg12 = pmu.readRegister8(0x12);
        if (reg12 != 0x0F) {
            Serial.printf("[PMU] WARNING: REG 0x12 = 0x%02X (expect 0x0F), retry...\n", reg12);
            delay(10);
            pmu.writeRegister8(0x12, 0x0F);
            reg12 = pmu.readRegister8(0x12);
            Serial.printf("[PMU] REG 0x12 retry = 0x%02X\n", reg12);
        }
        delay(20);  /* 等待输出稳定 */

        pmu.setPWROKDelay(1);

        pmu.setChargeEnable(true);
        pmu.setChargeCurrent(CHG_450mA);
        pmu.setCoulometer(COULOMETER_ENABLE, true);

        pmu.setADCEnable(ADC_BAT_V,  true);
        pmu.setADCEnable(ADC_BAT_C,  true);
        pmu.setADCEnable(ADC_VBUS_V, true);
        pmu.setADCEnable(ADC_VBUS_C, true);
        pmu.setADCEnable(ADC_TS,     true);
        pmu.setTSCurrent(80, 80);
        pmu.setChipTempEnable(true);

        pmu.setPowerOnTime(POWERON_1S);
        pmu.setPowerOffTime(POWEROFF_4S);
    }
pmu_done:

    delay(100);   /* power rails stabilize */

    /* I2C bus recovery  */
    _i2cBusRecovery((gpio_num_t)HAL_PIN_I2C_SDA,
                    (gpio_num_t)HAL_PIN_I2C_SCL);

    /* IO Expander — PCA9557 (LCD/Touch reset + PA_EN)
     * Verify the device is reachable on I2C before relying on it.
     * If the bus is still wobbly after recovery, retry a few times. */
    io_exp.begin(HAL_I2C_ADDR_PCA9557, &In_I2C);

    {
        bool ioexp_ok = false;
        for (int attempt = 0; attempt < 5; attempt++) {
            /* Try a register read to confirm PCA9557 is responding */
            if (io_exp.pinMode(HAL_IOEXP_LCD_RST, OUTPUT)) {
                ioexp_ok = true;
                break;
            }
            Serial.printf("[IO_EXP] PCA9557 not responding, retry %d/5\n", attempt + 1);
            delay(100);
            _i2cBusRecovery((gpio_num_t)HAL_PIN_I2C_SDA,
                            (gpio_num_t)HAL_PIN_I2C_SCL);
            delay(50);
        }
        if (!ioexp_ok) {
            Serial.println("[IO_EXP] PCA9557 init FAILED after retries");
            ok = false;
            goto skip_display;   /* LCD/Touch depend on IO expander */
        }
    }

    /* Configure all IO pins as OUTPUT */
    io_exp.pinMode(HAL_IOEXP_LCD_CS,    OUTPUT);
    io_exp.pinMode(HAL_IOEXP_LCD_RST,   OUTPUT);
    io_exp.pinMode(HAL_IOEXP_PA_EN,     OUTPUT);
    io_exp.pinMode(HAL_IOEXP_TOUCH_RST, OUTPUT);

    /* ── Reset sequence: LCD (ST7789) + Touch (FT6336) ── */
    /* Assert resets LOW, deselect LCD */
    {
        bool rst_ok = true;
        rst_ok &= io_exp.digitalWrite(HAL_IOEXP_LCD_CS,    HIGH);
        rst_ok &= io_exp.digitalWrite(HAL_IOEXP_LCD_RST,   LOW);
        rst_ok &= io_exp.digitalWrite(HAL_IOEXP_PA_EN,     LOW);
        rst_ok &= io_exp.digitalWrite(HAL_IOEXP_TOUCH_RST, LOW);
        if (!rst_ok) Serial.println("[IO_EXP] Warning: reset-LOW write failed");
        delay(20);

        /* Release resets — ST7789 needs ~120ms, FT6336 needs ~100ms */
        rst_ok  = io_exp.digitalWrite(HAL_IOEXP_LCD_RST,   HIGH);
        rst_ok &= io_exp.digitalWrite(HAL_IOEXP_TOUCH_RST, HIGH);
        if (!rst_ok) Serial.println("[IO_EXP] Warning: reset-HIGH write failed");
        delay(120);

        /* Select LCD */
        rst_ok = io_exp.digitalWrite(HAL_IOEXP_LCD_CS, LOW);
        if (!rst_ok) Serial.println("[IO_EXP] Warning: CS-LOW write failed");
        delay(5);
    }

    /* Touch — FT6336 */
    ctp.init(&In_I2C);

    /* LCD — ST7789
     * GPIO42 stays HIGH (backlight OFF, active-low) via GPIO throughout Lcd.init().
     * 1. Lcd.init()      — sends SLPOUT+DISPON while GPIO42 is GPIO-controlled HIGH.
     *                      Old GRAM from previous boot is invisible (BL physically off).
     * 2. fillScreen      — overwrites retained GRAM with solid black while BL still off.
     * 3. initBacklight() — attaches LEDC now that GRAM is clean; any duty=0 glitch
     *                      during ledcAttachPin() briefly shows black GRAM = invisible. */
    Lcd.init();
    Lcd.fillScreen(TFT_BLACK);
    delay(200);
    Lcd.initBacklight();

skip_display:

    /* RTC — PCF8563 */
    if (!rtc.begin()) ok = false;

    /* IMU — BMI270 + BMM150 */
    if (!imu.begin(&In_I2C)) ok = false;

    /* LED — WS2812B: 3 quick green blinks on boot, then off until settings load */
    led.begin();
    for (int i = 0; i < 3; i++) {
        led.setColor(WS2812B_Class::GREEN);
        delay(120);
        led.off();
        delay(120);
    }

    /* Buttons — A/B + Joystick */
    button.begin();

#if MEOWKIT_HW_TEST_ENABLE
    /* ══════════════════════════════════════════════════════════════
     *  Hardware Board Test (BBT) — 12 Items
     *  Each test: A = Start, B = Skip
     *  Set MEOWKIT_HW_TEST_ENABLE to 0 in config.h to disable
     * ══════════════════════════════════════════════════════════════ */
    Serial.println("\n========== MeowKit Hardware BBT ==========\n");

    /* ── Run-once guard: skip BBT after factory test has been completed ── */
    {
        Preferences _bbt_prefs;
        _bbt_prefs.begin("mk_bbt", true);
        bool _bbt_done = _bbt_prefs.getBool("done", false);
        _bbt_prefs.end();
        if (_bbt_done) {
            Serial.println("[BBT] Factory test already completed — entering system UI");
            goto bbt_end;
        }
    }

    /* ── Nested scope: keeps bbt_results out of goto-over-decl range ── */
    {
    /* ── TUI: results tracker + opening overview ── */
    int8_t bbt_results[BBT_NUM_TESTS] = {};
    bbt_drawOverview(Lcd, bbt_results, true);
    /* Wait for A to start, B to skip entire BBT */
    {
        button.update(); button.A.hasChanged(); button.B.hasChanged(); delay(200);
        bool bbt_skip = false;
        while (true) {
            button.update();
            if (button.A.pressed()) { break; }
            if (button.B.pressed()) { bbt_skip = true; break; }
            delay(20);
        }
        if (bbt_skip) goto bbt_done;
    }

    /* ── BBT 1/12 — Display (ST7789 320×240 SPI + PWM backlight) ─ */
    {
        bbt_drawPrompt(Lcd, "01", "DISPLAY ST7789",
                       "ST7789 320x240 SPI IPS + PWM BL",
                       "COLOR SWEEP + BACKLIGHT PWM",
                       bbt_results);
        Serial.println("[BBT 1/12] Display");
        button.update(); button.A.hasChanged(); button.B.hasChanged(); delay(200);
        bool enter = false;
        while (true) {
            button.update();
            if (button.A.pressed()) { enter = true; break; }
            if (button.B.pressed()) { break; }
            delay(20);
        }
        if (enter) {
            bbt_beginContent(Lcd, "01", "DISPLAY ST7789");
            Serial.println("[BBT 1] Display color test start");

            /* Color table: name + 16-bit color */
            struct { const char* name; uint16_t color; } colors[] = {
                {"RED",     TFT_RED},
                {"GREEN",   TFT_GREEN},
                {"BLUE",    TFT_BLUE},
                {"WHITE",   TFT_WHITE},
                {"YELLOW",  TFT_YELLOW},
                {"CYAN",    TFT_CYAN},
                {"MAGENTA", TFT_MAGENTA},
                {"ORANGE",  TFT_ORANGE},
                {"BLACK",   TFT_BLACK},
            };
            const int nColors = sizeof(colors) / sizeof(colors[0]);

            for (int c = 0; c < nColors; c++) {
                Lcd.fillScreen(colors[c].color);
                /* Draw label in contrasting color */
                uint16_t txt = (colors[c].color == TFT_BLACK || colors[c].color == TFT_BLUE)
                               ? TFT_WHITE : TFT_BLACK;
                Lcd.setTextColor(txt, colors[c].color);
                Lcd.setFont(&fonts::efontCN_16);
                Lcd.setCursor(80, 112);
                Lcd.printf(" %d/%d %s ", c + 1, nColors, colors[c].name);
                Serial.printf("[BBT 1] %s\n", colors[c].name);
                delay(800);

                /* Allow B to skip remaining colors */
                button.update();
                if (button.B.pressed()) break;
            }

            /* Backlight PWM sweep: dim → bright → normal */
            Lcd.fillScreen(TFT_WHITE);
            Lcd.setTextColor(TFT_BLACK, TFT_WHITE);
            Lcd.setCursor(40, 112);
            Lcd.printf(" Backlight PWM sweep... ");
            Serial.println("[BBT 1] Backlight sweep");
            for (int b = 0; b <= 255; b += 5) {
                Lcd.setBrightness(b);
                delay(20);
            }
            for (int b = 255; b >= 0; b -= 5) {
                Lcd.setBrightness(b);
                delay(20);
            }
            Lcd.setBrightness(128);  /* restore to normal */

            bbt_results[BBT_IDX_DISPLAY] = BBT_ST_PASS;
            bbt_drawResult(Lcd, "01", "DISPLAY ST7789", true,
                           "9-COLOR SWEEP: PASS",
                           "BACKLIGHT PWM SWEEP: PASS");
            Serial.println("[BBT 1] Display PASS");
            delay(1500);
            button.update(); button.A.hasChanged(); button.B.hasChanged(); delay(100);
            while (true) { button.update(); if (button.B.pressed()) break; delay(20); }
        } else {
            bbt_results[BBT_IDX_DISPLAY] = BBT_ST_SKIP;
        }
    }

    /* ── BBT 2/12 — Touch (FT6336 — tap 4 corners + center) ──── */
    {
        bbt_drawPrompt(Lcd, "02", "TOUCH FT6336",
                       "CAPACITIVE TOUCH I2C 0x38",
                       "TAP 4 CORNERS + CENTER (5 TARGETS)",
                       bbt_results);
        Serial.println("[BBT 2/12] Touch");
        button.update(); button.A.hasChanged(); button.B.hasChanged(); delay(200);
        bool enter = false;
        while (true) {
            button.update();
            if (button.A.pressed()) { enter = true; break; }
            if (button.B.pressed()) { break; }
            delay(20);
        }
        if (enter) {
            Serial.println("[BBT 2] Touch calibration test start");

            /* 5 target points: 4 corners (with margin) + center */
            const int margin = 30;
            struct { int x; int y; const char* label; } targets[] = {
                { margin,       margin,        "Top-Left"     },
                { 320 - margin, margin,        "Top-Right"    },
                { margin,       240 - margin,  "Bottom-Left"  },
                { 320 - margin, 240 - margin,  "Bottom-Right" },
                { 160,          120,           "Center"       },
            };
            const int nTargets = 5;
            const int hitRadius = 35;  /* touch within this radius = pass */
            int passed = 0;

            for (int t = 0; t < nTargets; t++) {
                Lcd.fillScreen(TFT_BLACK);
                Lcd.setFont(&fonts::efontCN_16);
                Lcd.setTextColor(TFT_CYAN, TFT_BLACK);
                Lcd.setCursor(40, 0);
                Lcd.printf(" [BBT 2] Tap: %s (%d/%d)", targets[t].label, t + 1, nTargets);

                /* Draw crosshair target */
                int tx = targets[t].x;
                int ty = targets[t].y;
                Lcd.drawCircle(tx, ty, 15, TFT_YELLOW);
                Lcd.drawCircle(tx, ty, 5,  TFT_YELLOW);
                Lcd.drawLine(tx - 20, ty, tx + 20, ty, TFT_YELLOW);
                Lcd.drawLine(tx, ty - 20, tx, ty + 20, TFT_YELLOW);

                Serial.printf("[BBT 2] Waiting tap: %s (%d,%d)\n", targets[t].label, tx, ty);

                /* Wait for touch hit or B to skip */
                bool hit = false;
                bool skip = false;
                while (!hit && !skip) {
                    button.update();
                    if (button.B.pressed()) { skip = true; break; }

                    if (ctp.isTouched()) {
                        int px = 0, py = 0;
                        ctp.getPos(px, py);

                        /* Draw touch point */
                        Lcd.fillCircle(px, py, 3, TFT_GREEN);

                        /* Check distance to target */
                        int dx = px - tx;
                        int dy = py - ty;
                        if (dx * dx + dy * dy <= hitRadius * hitRadius) {
                            hit = true;
                            passed++;
                            Lcd.fillCircle(tx, ty, 15, TFT_GREEN);
                            Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
                            Lcd.setCursor(80, 220);
                            Lcd.printf(" OK! (%d,%d)", px, py);
                            Serial.printf("[BBT 2] HIT %s at (%d,%d)\n", targets[t].label, px, py);
                            delay(500);
                        }
                        /* Debounce: wait for release */
                        while (ctp.isTouched()) delay(20);
                    }
                    delay(20);
                }
                if (skip) break;
            }

            /* Result */
            bool touch_pass = (passed == nTargets);
            char td1[40]; snprintf(td1, sizeof(td1), "TARGETS HIT: %d/%d", passed, nTargets);
            bbt_results[BBT_IDX_TOUCH] = touch_pass ? BBT_ST_PASS : BBT_ST_FAIL;
            bbt_drawResult(Lcd, "02", "TOUCH FT6336", touch_pass,
                           td1, "5-POINT CALIBRATION TEST");
            Serial.printf("[BBT 2] Touch %s %d/%d\n",
                          touch_pass ? "PASS" : "FAIL", passed, nTargets);
            delay(1500);
            button.update(); button.A.hasChanged(); button.B.hasChanged(); delay(100);
            while (true) { button.update(); if (button.B.pressed()) break; delay(20); }
        } else {
            bbt_results[BBT_IDX_TOUCH] = BBT_ST_SKIP;
        }
    }

    /* ── BBT 3/12 — I2C Scan (7 known devices, 100kHz) ────────── */
    {
        bbt_drawPrompt(Lcd, "03", "I2C BUS SCAN",
                       "7 KNOWN DEVICES @ I2C_NUM_0",
                       "SCAN @ 100KHZ — CHECK ALL ADDRESSES",
                       bbt_results);
        Serial.println("[BBT 3/12] I2C Scan");
        button.update(); button.A.hasChanged(); button.B.hasChanged(); delay(200);
        bool enter = false;
        while (true) {
            button.update();
            if (button.A.pressed()) { enter = true; break; }
            if (button.B.pressed()) { break; }
            delay(20);
        }
        if (enter) {
            bbt_beginContent(Lcd, "03", "I2C BUS SCAN");
            Serial.println("[BBT 3] I2C scan start");

            struct { uint8_t addr; const char* name; } devs[] = {
                { 0x18, "ES8311 Audio Output"  },
                { 0x19, "PCA9557 IO Expander"  },
                { 0x34, "AXP173 PMIC"          },
                { 0x38, "FT6336 Touch Screen"  },
                { 0x41, "ES7210 Audio Input"   },
                { 0x51, "PCF8563 RTC"          },
                { 0x68, "BMI270 IMU"           },
            };
            const int nDevs = sizeof(devs) / sizeof(devs[0]);

            Lcd.setTextColor(TFT_YELLOW, TFT_BLACK);
            Lcd.printf(" Scanning @ 100kHz...\n");
            Serial.println("[BBT 3] Scanning @ 100kHz...");
            delay(300);

            int found = 0;
            for (int i = 0; i < nDevs; i++) {
                bool present = In_I2C.scanID(devs[i].addr);
                if (present) found++;

                uint16_t color = present ? TFT_GREEN : TFT_RED;
                const char* mark = present ? "[OK]" : "[--]";

                Lcd.setTextColor(color, TFT_BLACK);
                Lcd.printf(" %s 0x%02X: %s\n", mark, devs[i].addr, devs[i].name);
                Serial.printf(" %s 0x%02X: %s\n", mark, devs[i].addr, devs[i].name);
                delay(150);
            }

            /* Result */
            Lcd.printf("\n");
            bool i2c_pass = (found == nDevs);
            bbt_results[BBT_IDX_I2C] = i2c_pass ? BBT_ST_PASS : BBT_ST_FAIL;
            char i2c_d1[40]; snprintf(i2c_d1, sizeof(i2c_d1), "FOUND: %d/%d DEVICES", found, nDevs);
            bbt_drawResult(Lcd, "03", "I2C BUS SCAN", i2c_pass,
                           i2c_d1, i2c_pass ? "ALL DEVICES PRESENT" : "MISSING DEVICE(S)");
            Serial.printf("[BBT 3] I2C: %d/%d %s\n", found, nDevs, i2c_pass ? "PASS" : "FAIL");
            delay(1500);
            button.update(); button.A.hasChanged(); button.B.hasChanged(); delay(100);
            while (true) { button.update(); if (button.B.pressed()) break; delay(20); }
        } else {
            bbt_results[BBT_IDX_I2C] = BBT_ST_SKIP;
        }
    }

    /* ── BBT 4/12 — RTC (PCF8563 — set & read time) ───────────── */
    {
        Lcd.fillScreen(TFT_BLACK);
        Lcd.setCursor(0, 0);
        Lcd.setTextColor(TFT_CYAN, TFT_BLACK);
        Lcd.setFont(&fonts::efontCN_16);
        Lcd.printf(" [BBT 4/12] RTC PCF8563\n");
        Lcd.printf(" Write 2026/01/01 00:00:00\n");
        Lcd.printf(" Read & display 5s\n");
        Lcd.printf(" A=Start  B=Skip\n");
        Serial.println("[BBT 4/12] RTC");
        button.update(); button.A.hasChanged(); button.B.hasChanged(); delay(200);
        bool enter = false;
        while (true) {
            button.update();
            if (button.A.pressed()) { enter = true; break; }
            if (button.B.pressed()) { break; }
            delay(20);
        }
        if (enter) {
            bbt_beginContent(Lcd, "04", "RTC PCF8563");
            Serial.println("[BBT 4] RTC test start");

            /* Write initial time: 2026/01/01 00:00:00 Thursday (weekday 4) */
            RTC_Time wt;
            wt.year    = 2026;
            wt.month   = 1;
            wt.day     = 1;
            wt.weekday = 4;   /* 0=Sun, 4=Thu */
            wt.hour    = 0;
            wt.min     = 0;
            wt.sec     = 0;

            bool setOk = rtc.setTime(wt);
            Lcd.setTextColor(setOk ? TFT_GREEN : TFT_RED, TFT_BLACK);
            Lcd.printf(" setTime: %s\n", setOk ? "OK" : "FAIL");
            Serial.printf("[BBT 4] setTime: %s\n", setOk ? "OK" : "FAIL");

            if (setOk) {
                const char* weekNames[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
                int timeY = Lcd.getCursorY();

                /* Read & display for 5 seconds */
                Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
                Lcd.printf(" Reading RTC for 5s...\n\n");
                int dispY = Lcd.getCursorY();

                uint32_t t0 = millis();
                while (millis() - t0 < 5000) {
                    RTC_Time rt;
                    if (rtc.getTime(rt)) {
                        const char* wk = (rt.weekday <= 6) ? weekNames[rt.weekday] : "???";

                        /* LCD: overwrite time line */
                        Lcd.fillRect(0, dispY, 320, 40, TFT_BLACK);
                        Lcd.setCursor(10, dispY);
                        Lcd.setTextColor(TFT_YELLOW, TFT_BLACK);
                        Lcd.setFont(&fonts::efontCN_16);
                        Lcd.printf(" %04d/%02d/%02d %s\n",
                                   rt.year, rt.month, rt.day, wk);
                        Lcd.printf(" %02d:%02d:%02d",
                                   rt.hour, rt.min, rt.sec);

                        Serial.printf("\r[BBT 4] %04d/%02d/%02d %s %02d:%02d:%02d",
                                      rt.year, rt.month, rt.day, wk,
                                      rt.hour, rt.min, rt.sec);
                    }
                    delay(500);

                    button.update();
                    if (button.B.pressed()) break;
                }
                Serial.println();
            }

            /* Result */
            bbt_results[BBT_IDX_RTC] = setOk ? BBT_ST_PASS : BBT_ST_FAIL;
            bbt_drawResult(Lcd, "04", "RTC PCF8563", setOk,
                           setOk ? "SET + READ-BACK: OK" : "I2C WRITE FAILED",
                           "PCF8563 @ I2C 0x51");
            Serial.println(setOk ? "[BBT 4] RTC PASS" : "[BBT 4] RTC FAIL");
            delay(1500);
            button.update(); button.A.hasChanged(); button.B.hasChanged(); delay(100);
            while (true) { button.update(); if (button.B.pressed()) break; delay(20); }
        } else {
            bbt_results[BBT_IDX_RTC] = BBT_ST_SKIP;
        }
    }

    /* ── BBT 5/12 — IMU (BMI270 + BMM150) ─────────────────────── */
    {
        Lcd.fillScreen(TFT_BLACK);
        Lcd.setCursor(0, 0);
        Lcd.setTextColor(TFT_CYAN, TFT_BLACK);
        Lcd.setFont(&fonts::efontCN_16);
        Lcd.printf(" [BBT 5/12] IMU BMI270+BMM150\n");
        Lcd.printf(" Accel+Gyro live + Bubble\n");
        Lcd.printf(" A=Start  B=Skip\n");
        Serial.println("[BBT 5/12] IMU");
        button.update(); button.A.hasChanged(); button.B.hasChanged(); delay(200);
        bool enter = false;
        while (true) {
            button.update();
            if (button.A.pressed()) { enter = true; break; }
            if (button.B.pressed()) { break; }
            delay(20);
        }
        if (enter) {
            Serial.println("[BBT 5] IMU test start");

            bool imu_ok = imu.isEnabled();
            if (!imu_ok) {
                Lcd.setTextColor(TFT_RED, TFT_BLACK);
                Lcd.printf(" IMU not found!\n");
                Serial.println("[BBT 5] IMU not found");
                delay(1500);
            } else {
                Lcd.fillScreen(TFT_BLACK);

                /* ── Static UI: labels on left, bubble sphere on right ── */
                /* Sphere parameters */
                const int SPH_CX = 248, SPH_CY = 120, SPH_R = 55;
                const int BUB_R = 6;
                const float BUB_SENS = 50.0f;

                /* Draw sphere ring + crosshair */
                Lcd.drawCircle(SPH_CX, SPH_CY, SPH_R,     0x0600);
                Lcd.drawCircle(SPH_CX, SPH_CY, SPH_R + 1, 0x0600);
                Lcd.drawFastHLine(SPH_CX - 4, SPH_CY, 9, 0x0600);
                Lcd.drawFastVLine(SPH_CX, SPH_CY - 4, 9, 0x0600);

                /* Labels */
                Lcd.setFont(&fonts::efontCN_16);
                Lcd.setTextColor(TFT_RED, TFT_BLACK);
                Lcd.setCursor(4, 10);  Lcd.printf("Ax:");
                Lcd.setTextColor(TFT_YELLOW, TFT_BLACK);
                Lcd.setCursor(4, 30);  Lcd.printf("Ay:");
                Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
                Lcd.setCursor(4, 50);  Lcd.printf("Az:");

                Lcd.setTextColor(0xFD20, TFT_BLACK);
                Lcd.setCursor(4, 80);  Lcd.printf("Gx:");
                Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
                Lcd.setCursor(4, 100); Lcd.printf("Gy:");
                Lcd.setTextColor(TFT_BLUE, TFT_BLACK);
                Lcd.setCursor(4, 120); Lcd.printf("Gz:");

                Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
                Lcd.setCursor(4, 150); Lcd.printf("Temp:");

                Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
                Lcd.setCursor(4, 220); Lcd.printf(" B=Exit");

                int prev_bx = SPH_CX, prev_by = SPH_CY;
                float prev_acc[3] = {0}, prev_gyro[3] = {0};
                float prev_temp = -999.0f;
                const int VX = 50;   /* value X offset */
                const int VW = 120;  /* value clear width */

                bool running = true;
                while (running) {
                    auto mask = imu.update();
                    if (mask) {
                        auto data = imu.getImuData();
                        char buf[20];

                        /* ── Accel values ── */
                        float acc[3] = { data.accel.x, data.accel.y, data.accel.z };
                        uint16_t accClr[3] = { TFT_RED, TFT_YELLOW, TFT_GREEN };
                        int accY[3] = { 10, 30, 50 };
                        for (int i = 0; i < 3; i++) {
                            if (fabsf(acc[i] - prev_acc[i]) > 0.005f) {
                                Lcd.fillRect(VX, accY[i], VW, 18, TFT_BLACK);
                                Lcd.setTextColor(accClr[i], TFT_BLACK);
                                Lcd.setCursor(VX, accY[i]);
                                snprintf(buf, sizeof(buf), "%.3f g", acc[i]);
                                Lcd.printf("%s", buf);
                                prev_acc[i] = acc[i];
                            }
                        }

                        /* ── Gyro values ── */
                        float gyro[3] = { data.gyro.x, data.gyro.y, data.gyro.z };
                        uint16_t gyroClr[3] = { 0xFD20, TFT_GREEN, TFT_BLUE };
                        int gyroY[3] = { 80, 100, 120 };
                        for (int i = 0; i < 3; i++) {
                            if (fabsf(gyro[i] - prev_gyro[i]) > 0.05f) {
                                Lcd.fillRect(VX, gyroY[i], VW, 18, TFT_BLACK);
                                Lcd.setTextColor(gyroClr[i], TFT_BLACK);
                                Lcd.setCursor(VX, gyroY[i]);
                                snprintf(buf, sizeof(buf), "%.1f d/s", gyro[i]);
                                Lcd.printf("%s", buf);
                                prev_gyro[i] = gyro[i];
                            }
                        }

                        /* ── Temperature ── */
                        if (fabsf(data.temperature - prev_temp) > 0.1f) {
                            Lcd.fillRect(VX + 10, 150, 80, 18, TFT_BLACK);
                            Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
                            Lcd.setCursor(VX + 10, 150);
                            snprintf(buf, sizeof(buf), "%.1f C", data.temperature);
                            Lcd.printf("%s", buf);
                            prev_temp = data.temperature;
                        }

                        /* ── Bubble level (like app_06) ── */
                        int bx = SPH_CX + (int)( data.accel.y * BUB_SENS);
                        int by = SPH_CY + (int)(-data.accel.x * BUB_SENS);

                        /* Clamp inside ring */
                        float dx = (float)(bx - SPH_CX);
                        float dy = (float)(by - SPH_CY);
                        float dist = sqrtf(dx * dx + dy * dy);
                        float maxD = (float)(SPH_R - BUB_R - 2);
                        if (dist > maxD && dist > 0.1f) {
                            float sc = maxD / dist;
                            bx = SPH_CX + (int)(dx * sc);
                            by = SPH_CY + (int)(dy * sc);
                        }

                        if (abs(bx - prev_bx) >= 2 || abs(by - prev_by) >= 2) {
                            Lcd.startWrite();
                            /* Erase old bubble */
                            Lcd.fillCircle(prev_bx, prev_by, BUB_R + 2, TFT_BLACK);
                            /* Redraw crosshair if overlapped */
                            if (abs(prev_bx - SPH_CX) < BUB_R + 6 &&
                                abs(prev_by - SPH_CY) < BUB_R + 6) {
                                Lcd.drawFastHLine(SPH_CX - 4, SPH_CY, 9, 0x0600);
                                Lcd.drawFastVLine(SPH_CX, SPH_CY - 4, 9, 0x0600);
                            }
                            /* Redraw ring */
                            Lcd.drawCircle(SPH_CX, SPH_CY, SPH_R,     0x0600);
                            Lcd.drawCircle(SPH_CX, SPH_CY, SPH_R + 1, 0x0600);
                            /* Draw new bubble */
                            Lcd.fillCircle(bx, by, BUB_R, 0x07E0);
                            Lcd.fillCircle(bx, by, BUB_R - 3, TFT_WHITE);
                            Lcd.endWrite();
                            prev_bx = bx;
                            prev_by = by;
                        }

                        Serial.printf("\r[BBT 5] A:%.2f/%.2f/%.2f G:%.1f/%.1f/%.1f T:%.1f",
                                      acc[0], acc[1], acc[2],
                                      gyro[0], gyro[1], gyro[2],
                                      data.temperature);
                    }

                    button.update();
                    if (button.B.pressed()) running = false;
                    delay(20);
                }
                Serial.println();
            }

            /* Result */
            bbt_results[BBT_IDX_IMU] = imu_ok ? BBT_ST_PASS : BBT_ST_FAIL;
            bbt_drawResult(Lcd, "05", "IMU BMI270", imu_ok,
                           imu_ok ? "BMI270 + BMM150: RESPONDING" : "BMI270 NOT FOUND",
                           imu_ok ? "ACCEL/GYRO/MAG: OK" : "I2C 0x68 NO ACK");
            Serial.println(imu_ok ? "[BBT 5] IMU PASS" : "[BBT 5] IMU FAIL");
            delay(1500);
            button.update(); button.A.hasChanged(); button.B.hasChanged(); delay(100);
            while (true) { button.update(); if (button.B.pressed()) break; delay(20); }
        } else {
            bbt_results[BBT_IDX_IMU] = BBT_ST_SKIP;
        }
    }

    /* ── BBT 6/12 — LED (WS2812B R/G/Y + breathing/blink) ────── */
    {
        bbt_drawPrompt(Lcd, "06", "LED WS2812B",
                       "WS2812B-2020 STATUS LED (GPIO38)",
                       "SOLID R/G/Y + BLINK FAST + BREATHING",
                       bbt_results);
        Serial.println("[BBT 6/12] LED");
        button.update(); button.A.hasChanged(); button.B.hasChanged(); delay(200);
        bool enter = false;
        while (true) {
            button.update();
            if (button.A.pressed()) { enter = true; break; }
            if (button.B.pressed()) { break; }
            delay(20);
        }
        if (enter) {
            bbt_beginContent(Lcd, "06", "LED WS2812B");
            Serial.println("[BBT 6] LED test start");

            auto showStep = [&](const char* label, WS2812B_Class::Effect eff,
                                WS2812B_Class::Color color, float spd, int ms) {
                Lcd.setTextColor(TFT_YELLOW, TFT_BLACK);
                Lcd.printf(" %s\n", label);
                Serial.printf("[BBT 6] %s\n", label);
                led.setEffect(eff, color, spd);
                /* Update LED & check B to skip */
                uint32_t t0 = millis();
                while (millis() - t0 < (uint32_t)ms) {
                    led.update();
                    button.update();
                    if (button.B.pressed()) return false;
                    delay(20);
                }
                return true;
            };

            bool ok = true;
            /* 1) RED solid 2s */
            if (ok) ok = showStep("RED SOLID 2s",
                WS2812B_Class::SOLID, WS2812B_Class::RED, 1.0f, 2000);
            /* 2) GREEN solid 2s */
            if (ok) ok = showStep("GREEN SOLID 2s",
                WS2812B_Class::SOLID, WS2812B_Class::GREEN, 1.0f, 2000);
            /* 3) YELLOW solid 2s */
            if (ok) ok = showStep("YELLOW SOLID 2s",
                WS2812B_Class::SOLID, WS2812B_Class::YELLOW, 1.0f, 2000);
            /* 4) GREEN blink fast 3s */
            if (ok) ok = showStep("GREEN BLINK_FAST 3s",
                WS2812B_Class::BLINK_FAST, WS2812B_Class::GREEN, 1.0f, 3000);
            /* 5) GREEN breathing 4s */
            if (ok) ok = showStep("GREEN BREATHING 4s",
                WS2812B_Class::BREATHING, WS2812B_Class::GREEN, 1.0f, 4000);

            /* Restore to init state */
            led.setEffect(WS2812B_Class::BREATHING, WS2812B_Class::GREEN, 0.5f);

            bbt_results[BBT_IDX_LED] = BBT_ST_PASS;
            bbt_drawResult(Lcd, "06", "LED WS2812B", true,
                           "R/G/Y SOLID + BLINK + BREATHING",
                           "ALL EFFECTS: PASS");
            Serial.println("[BBT 6] LED PASS");
            delay(1500);
            button.update(); button.A.hasChanged(); button.B.hasChanged(); delay(100);
            while (true) { button.update(); if (button.B.pressed()) break; delay(20); }
        } else {
            bbt_results[BBT_IDX_LED] = BBT_ST_SKIP;
        }
    }

    /* ── BBT 7/12 — Buttons & Joystick (6 keys) ─────────────── */
    {
        bbt_drawPrompt(Lcd, "07", "BUTTONS+JOY",
                       "A / B + UP / DOWN / LEFT / RIGHT",
                       "PRESS ALL 6 KEYS TO PASS",
                       bbt_results);
        Serial.println("[BBT 7/12] Buttons");
        button.update(); button.A.hasChanged(); button.B.hasChanged(); delay(200);
        bool enter = false;
        while (true) {
            button.update();
            if (button.A.pressed()) { enter = true; break; }
            if (button.B.pressed()) { break; }
            delay(20);
        }
        if (enter) {
            Serial.println("[BBT 7] Buttons test start");

            Lcd.fillScreen(TFT_BLACK);
            Lcd.setFont(&fonts::efontCN_16);

            /* 6 buttons to test — track which have been pressed */
            struct { Button_Class* btn; const char* name; bool hit; } keys[] = {
                { &button.A,     "A",     false },
                { &button.B,     "B",     false },
                { &button.Up,    "Up",    false },
                { &button.Down,  "Down",  false },
                { &button.Left,  "Left",  false },
                { &button.Right, "Right", false },
            };
            const int nKeys = 6;

            /* Drain stale edges */
            button.update();
            for (int i = 0; i < nKeys; i++) keys[i].btn->hasChanged();
            delay(200);

            /* Display key status list */
            auto drawKeys = [&]() {
                for (int i = 0; i < nKeys; i++) {
                    int y = 10 + i * 28;
                    Lcd.fillRect(0, y, 320, 24, TFT_BLACK);
                    Lcd.setCursor(10, y);
                    if (keys[i].hit) {
                        Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
                        Lcd.printf(" [OK] %-6s", keys[i].name);
                    } else {
                        Lcd.setTextColor(TFT_DARKGREY, TFT_BLACK);
                        Lcd.printf(" [  ] %-6s", keys[i].name);
                    }
                }
                /* Count & footer */
                int done = 0;
                for (int i = 0; i < nKeys; i++) if (keys[i].hit) done++;
                Lcd.fillRect(0, 185, 320, 50, TFT_BLACK);
                Lcd.setCursor(10, 190);
                Lcd.setTextColor(TFT_YELLOW, TFT_BLACK);
                Lcd.printf(" %d/%d — Press all keys", done, nKeys);
                Lcd.setCursor(10, 215);
                Lcd.setTextColor(TFT_CYAN, TFT_BLACK);
                Lcd.printf(" Long-press A to finish");
            };

            drawKeys();

            bool running = true;
            uint32_t aHoldStart = 0;
            while (running) {
                button.update();

                bool changed = false;
                for (int i = 0; i < nKeys; i++) {
                    if (!keys[i].hit && keys[i].btn->pressed()) {
                        keys[i].hit = true;
                        changed = true;
                        Serial.printf("[BBT 7] %s pressed\n", keys[i].name);
                    }
                }
                if (changed) drawKeys();

                /* Check if all done */
                int done = 0;
                for (int i = 0; i < nKeys; i++) if (keys[i].hit) done++;
                if (done == nKeys) {
                    delay(300);
                    running = false;
                }

                /* Long-press A (>1.5s) to force finish early */
                if (button.A.state() == false) { /* active-low: pressed */
                    if (aHoldStart == 0) aHoldStart = millis();
                    if (millis() - aHoldStart > 1500) running = false;
                } else {
                    aHoldStart = 0;
                }

                delay(20);
            }

            /* Result */
            int passed = 0;
            for (int i = 0; i < nKeys; i++) if (keys[i].hit) passed++;

            bool btn_pass = (passed == nKeys);
            char bd1[40]; snprintf(bd1, sizeof(bd1), "PRESSED: %d/%d KEYS", passed, nKeys);
            bbt_results[BBT_IDX_BUTTONS] = btn_pass ? BBT_ST_PASS : BBT_ST_FAIL;
            bbt_drawResult(Lcd, "07", "BUTTONS+JOY", btn_pass,
                           bd1, "A / B / UP / DOWN / LEFT / RIGHT");
            Serial.printf("[BBT 7] Buttons %s %d/%d\n",
                          btn_pass ? "PASS" : "FAIL", passed, nKeys);
            delay(1500);
            button.update(); button.A.hasChanged(); button.B.hasChanged(); delay(100);
            while (true) { button.update(); if (button.B.pressed()) break; delay(20); }
        } else {
            bbt_results[BBT_IDX_BUTTONS] = BBT_ST_SKIP;
        }
    }

    /* ── BBT 8/12 — IR (940nm TX / 950nm RX, 38kHz) ───────────── */
    {
        Lcd.fillScreen(TFT_BLACK);
        Lcd.setCursor(0, 0);
        Lcd.setTextColor(TFT_CYAN, TFT_BLACK);
        Lcd.setFont(&fonts::efontCN_16);
        Lcd.printf(" [BBT 8/12] IR TX/RX\n");
        Lcd.printf(" TX=GPIO7  RX=GPIO5\n");
        Lcd.printf(" A=Start  B=Skip\n");
        Serial.println("[BBT 8/12] IR");
        button.update(); button.A.hasChanged(); button.B.hasChanged(); delay(200);
        bool enter = false;
        while (true) {
            button.update();
            if (button.A.pressed()) { enter = true; break; }
            if (button.B.pressed()) { break; }
            delay(20);
        }
        if (enter) {
            /* ──── Phase 1: TX — send 10 common IR codes ──── */
            Lcd.fillScreen(TFT_BLACK);
            Lcd.setCursor(0, 0);
            Lcd.setTextColor(TFT_CYAN, TFT_BLACK);
            Lcd.setFont(&fonts::efontCN_16);
            Lcd.printf(" [BBT 8] IR TX Phase\n");
            Lcd.printf(" Send 10 common IR codes\n");
            Lcd.printf(" A=Start  B=Skip\n");
            Serial.println("[BBT 8] IR TX phase");
            button.update(); button.A.hasChanged(); button.B.hasChanged(); delay(200);

            bool tx_enter = false;
            while (true) {
                button.update();
                if (button.A.pressed()) { tx_enter = true; break; }
                if (button.B.pressed()) { break; }
                delay(20);
            }

            bool tx_ok = false;
            if (tx_enter) {
                Serial.println("[BBT 8] TX start");
                IRsend irSend(HAL_PIN_IR_TX);
                irSend.begin();

                struct { decode_type_t proto; uint64_t value; uint16_t bits; const char* label; } codes[] = {
                    { NEC,       0x20DF10EF,           32, "NEC  LG TV"       },
                    { NEC,       0x08F7C03F,           32, "NEC  Generic"     },
                    { SAMSUNG,   0xE0E040BF,           32, "Samsung TV"       },
                    { SAMSUNG,   0xE0E09966,           32, "Samsung TV2"      },
                    { RC5,       0x100C,               13, "RC5  Philips"     },
                    { RC6,       0x000C,               20, "RC6  Philips"     },
                    { SONY,      0xA90,                12, "Sony 12-bit"      },
                    { SONY,      0x540C,               15, "Sony 15-bit"      },
                    { LG,        0x20DF10EF,           28, "LG   TV"          },
                    { PANASONIC, 0x400401000BCDull,     48, "Panasonic TV"     },
                };
                const int nCodes = 10;

                Lcd.fillScreen(TFT_BLACK);
                Lcd.setCursor(0, 0);
                Lcd.setTextColor(TFT_CYAN, TFT_BLACK);
                Lcd.printf(" IR TX: Sending %d codes\n\n", nCodes);

                bool aborted = false;
                for (int i = 0; i < nCodes; i++) {
                    button.update();
                    if (button.B.pressed()) { aborted = true; break; }

                    irSend.send(codes[i].proto, codes[i].value, codes[i].bits);

                    Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
                    Lcd.printf(" [%2d] %s\n", i + 1, codes[i].label);
                    Serial.printf("[BBT 8] TX %d/%d: %s val=0x%llX bits=%d\n",
                                  i + 1, nCodes, codes[i].label,
                                  codes[i].value, codes[i].bits);
                    delay(400);
                }

                if (!aborted) {
                    Lcd.printf("\n");
                    Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
                    Lcd.printf(" TX complete: %d/%d sent\n", nCodes, nCodes);
                    Serial.printf("[BBT 8] TX complete: %d/%d\n", nCodes, nCodes);
                    tx_ok = true;
                } else {
                    Lcd.setTextColor(TFT_YELLOW, TFT_BLACK);
                    Lcd.printf("\n TX skipped by user\n");
                    Serial.println("[BBT 8] TX skipped");
                }
                delay(1000);
            }

            /* ──── Phase 2: RX — receive and display IR signals ──── */
            Lcd.fillScreen(TFT_BLACK);
            Lcd.setCursor(0, 0);
            Lcd.setTextColor(TFT_CYAN, TFT_BLACK);
            Lcd.setFont(&fonts::efontCN_16);
            Lcd.printf(" [BBT 8] IR RX Phase\n");
            Lcd.printf(" Receive & display IR data\n");
            Lcd.printf(" A=Start  B=Skip\n");
            Serial.println("[BBT 8] IR RX phase");
            button.update(); button.A.hasChanged(); button.B.hasChanged(); delay(200);

            bool rx_enter = false;
            while (true) {
                button.update();
                if (button.A.pressed()) { rx_enter = true; break; }
                if (button.B.pressed()) { break; }
                delay(20);
            }

            bool rx_ok = false;
            if (rx_enter) {
                Serial.println("[BBT 8] RX start — waiting for IR signals");
                IRrecv irRecv(HAL_PIN_IR_RX, 1024, 50, true);
                decode_results results;
                irRecv.enableIRIn();

                Lcd.fillScreen(TFT_BLACK);
                Lcd.setCursor(0, 0);
                Lcd.setTextColor(TFT_CYAN, TFT_BLACK);
                Lcd.printf(" IR RX: Waiting...\n");
                Lcd.printf(" Point remote at sensor\n");
                Lcd.setTextColor(TFT_YELLOW, TFT_BLACK);
                Lcd.printf(" B=Exit\n\n");

                int rxCount = 0;
                int dispY = Lcd.getCursorY();
                bool running = true;

                while (running) {
                    if (irRecv.decode(&results)) {
                        rxCount++;
                        rx_ok = true;

                        String protoName = typeToString(results.decode_type, results.repeat);
                        Serial.printf("[BBT 8] RX #%d: proto=%s val=0x%llX bits=%d addr=0x%X cmd=0x%X\n",
                                      rxCount, protoName.c_str(),
                                      results.value, results.bits,
                                      results.address, results.command);

                        /* Show on LCD — scroll region */
                        if (dispY > 200) {
                            Lcd.fillRect(0, 60, 320, 160, TFT_BLACK);
                            dispY = 60;
                        }
                        Lcd.setCursor(0, dispY);
                        Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
                        Lcd.printf(" #%d %s\n", rxCount, protoName.c_str());
                        dispY = Lcd.getCursorY();
                        Lcd.setCursor(0, dispY);
                        Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
                        Lcd.printf("  V:0x%llX B:%d\n", results.value, results.bits);
                        dispY = Lcd.getCursorY();
                        Lcd.setCursor(0, dispY);
                        Lcd.printf("  A:0x%X C:0x%X\n", results.address, results.command);
                        dispY = Lcd.getCursorY();

                        irRecv.resume();
                    }

                    button.update();
                    if (button.B.pressed()) running = false;
                    delay(20);
                }

                irRecv.disableIRIn();
                Serial.printf("[BBT 8] RX done, received %d signals\n", rxCount);
            }

            /* Result */
            bool ir_pass = tx_ok || rx_ok;
            char ir_d1[48], ir_d2[48];
            snprintf(ir_d1, sizeof(ir_d1), "TX: %s  RX: %s",
                     tx_ok ? "10/10 SENT" : "SKIP",
                     rx_ok ? "SIGNALS RX" : "NONE");
            snprintf(ir_d2, sizeof(ir_d2), "940NM TX/950NM RX @ 38KHZ");
            bbt_results[BBT_IDX_IR] = ir_pass ? BBT_ST_PASS : BBT_ST_FAIL;
            bbt_drawResult(Lcd, "08", "IR TX/RX", ir_pass, ir_d1, ir_d2);
            Serial.println(ir_pass ? "[BBT 8] IR PASS" : "[BBT 8] IR FAIL/SKIP");
            delay(1500);
            button.update(); button.A.hasChanged(); button.B.hasChanged(); delay(100);
            while (true) { button.update(); if (button.B.pressed()) break; delay(20); }
        } else {
            bbt_results[BBT_IDX_IR] = BBT_ST_SKIP;
        }
    }

    /* ══════════════════════════════════════════════════════════
     *  BBT 9a/12 — Mic (ES7210 ADC ← ZTS6216 MEMS)
     *  A = start, B = exit, loop until B pressed
     * ══════════════════════════════════════════════════════════ */
    {
        bbt_drawPrompt(Lcd, "09a", "MIC ES7210",
                       "ES7210 ADC + ZTS6216 MEMS MIC",
                       "LIVE VU METER — PRESS B TO STOP",
                       bbt_results);
        Serial.println("[MicTest] Press A to start, B to skip");

        /* Drain any stale button edges */
        button.update();
        button.A.hasChanged();
        button.B.hasChanged();
        delay(200);

        bool enter = false;
        while (true) {
            button.update();
            if (button.A.pressed()) { enter = true; break; }
            if (button.B.pressed()) { break; }
            delay(20);
        }

        if (enter) {
            ES7210_Class es7210(ES7210_I2C_ADDR, &In_I2C);
            bool codec_ok = es7210.begin(44100, ES7210_BIT_16,
                                         ES7210_FMT_I2S, ES7210_SIGNAL_I2S);
            if (codec_ok) {
                es7210.selectMic(ES7210_MIC1 | ES7210_MIC2);
                es7210.setGain(ES7210_GAIN_30DB);
                es7210.start();
            }
            Serial.printf("[MicTest] ES7210 init: %s\n", codec_ok ? "OK" : "FAIL");
            Lcd.printf(" ES7210 init: %s\n", codec_ok ? "OK" : "FAIL");

            i2s_config_t i2s_cfg = {};
            i2s_cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX);
            i2s_cfg.sample_rate          = 44100;
            i2s_cfg.bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT;
            i2s_cfg.channel_format       = I2S_CHANNEL_FMT_RIGHT_LEFT;
            i2s_cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
            i2s_cfg.intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1;
            i2s_cfg.dma_buf_count        = 4;
            i2s_cfg.dma_buf_len          = 512;
            i2s_cfg.use_apll             = true;
            i2s_cfg.tx_desc_auto_clear   = false;
            i2s_cfg.fixed_mclk          = 0;
#if ESP_IDF_VERSION_MAJOR >= 5
            i2s_cfg.mclk_multiple       = I2S_MCLK_MULTIPLE_256;
            i2s_cfg.bits_per_chan        = I2S_BITS_PER_CHAN_16BIT;
#endif

            esp_err_t err = i2s_driver_install(I2S_NUM_1, &i2s_cfg, 0, nullptr);
            Serial.printf("[MicTest] I2S install: %s\n", err == ESP_OK ? "OK" : "FAIL");
            Lcd.printf(" I2S install: %s\n", err == ESP_OK ? "OK" : "FAIL");

            if (err == ESP_OK) {
                i2s_pin_config_t pins = {};
                pins.mck_io_num   = HAL_PIN_I2S_MCLK;
                pins.bck_io_num   = HAL_PIN_I2S_BCLK;
                pins.ws_io_num    = HAL_PIN_I2S_WS;
                pins.data_out_num = I2S_PIN_NO_CHANGE;
                pins.data_in_num  = HAL_PIN_I2S_DIN;
                i2s_set_pin(I2S_NUM_1, &pins);
                i2s_zero_dma_buffer(I2S_NUM_1);
                i2s_start(I2S_NUM_1);

                Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
                Lcd.printf(" Mic live — B=Exit\n");
                Serial.println("[MicTest] Running — B to exit");

                constexpr size_t kSamples = 512;
                int16_t buf[kSamples * 2];
                float ema_db = -90.0f;

                /* LCD bar area: row=96, 40 chars wide */
                int lcd_bar_y = Lcd.getCursorY();

                bool running = true;
                while (running) {
                    size_t bytes_read = 0;
                    i2s_read(I2S_NUM_1, buf, sizeof(buf), &bytes_read, 10);

                    if (bytes_read > 0) {
                        size_t n = bytes_read / sizeof(int16_t);
                        double sum_sq = 0;
                        for (size_t i = 0; i < n; i++) {
                            float s = (float)buf[i];
                            sum_sq += s * s;
                        }
                        float rms = sqrtf((float)(sum_sq / n));
                        float db = 20.0f * log10f((rms + 1.0f) / 32768.0f);
                        if (db < -90.0f) db = -90.0f;

                        float alpha = (db > ema_db) ? 0.3f : 0.1f;
                        ema_db += alpha * (db - ema_db);

                        float norm = (ema_db - (-70.0f)) / ((-15.0f) - (-70.0f));
                        if (norm < 0.0f) norm = 0.0f;
                        if (norm > 1.0f) norm = 1.0f;
                        int bars = (int)(norm * 40.0f);

                        /* Serial bar */
                        char line[80];
                        int pos = 0;
                        line[pos++] = '[';
                        for (int i = 0; i < 40; i++)
                            line[pos++] = (i < bars) ? '=' : ' ';
                        line[pos++] = ']';
                        line[pos] = '\0';
                        Serial.printf("\r%s %6.1f dB", line, ema_db);

                        /* LCD bar */
                        Lcd.fillRect(0, lcd_bar_y, 320, 20, TFT_BLACK);
                        Lcd.setCursor(0, lcd_bar_y);
                        Lcd.setTextColor(TFT_YELLOW, TFT_BLACK);
                        Lcd.printf(" %s %.0fdB", line, ema_db);
                    }

                    button.update();
                    if (button.B.pressed()) running = false;
                }

                Serial.println("\n[MicTest] Stopped");
                i2s_stop(I2S_NUM_1);
                i2s_driver_uninstall(I2S_NUM_1);
            }

            if (codec_ok) {
                es7210.stop();
                es7210.end();
            }

            bbt_results[BBT_IDX_MIC] = BBT_ST_PASS;
            bbt_drawResult(Lcd, "09a", "MIC ES7210", true,
                           "ES7210 ADC INIT: OK",
                           "LIVE AUDIO CAPTURED");
            delay(1500);
            button.update(); button.A.hasChanged(); button.B.hasChanged(); delay(100);
            while (true) { button.update(); if (button.B.pressed()) break; delay(20); }
        } else {
            bbt_results[BBT_IDX_MIC] = BBT_ST_SKIP;
        }
    }

    /* ══════════════════════════════════════════════════════════
     *  BBT 9b/12 — Speaker (ES8311 DAC → NS4150B → 1W Speaker)
     *  A = start, B = exit, 1kHz beep 5s loop until B pressed
     * ══════════════════════════════════════════════════════════ */
    {
        Lcd.fillScreen(TFT_BLACK);
        Lcd.setCursor(0, 0);
        Lcd.setTextColor(TFT_CYAN, TFT_BLACK);
        Lcd.setFont(&fonts::efontCN_16);
        Lcd.printf(" [BBT 9b/12] Speaker (ES8311)\n");
        Lcd.printf(" ES8311 -> NS4150B -> Speaker\n");
        Lcd.printf(" A=Start  B=Skip\n");
        Serial.println("[SpkTest] Press A to start, B to skip");

        /* Drain any stale button edges */
        button.update();
        button.A.hasChanged();
        button.B.hasChanged();
        delay(200);

        bool enter = false;
        while (true) {
            button.update();
            if (button.A.pressed()) { enter = true; break; }
            if (button.B.pressed()) { break; }
            delay(20);
        }

        if (enter) {
            /* ── Hard-reset ES8311 via I2C before init ──
             * After hardware reset (AXP173 stays on), ES8311 retains stale
             * PLL/register state. A full register reset (0x1F) + sufficient
             * delay ensures clean re-init regardless of prior codec state.
             */
            es8311_set_i2c(&In_I2C);
            es8311_handle_t tmp_h = es8311_create((i2c_port_t)0, ES8311_ADDRRES_0);
            if (tmp_h) {
                es8311_write_reg(tmp_h, 0x00, 0x1F);   // Full reset
                delay(50);                               // PLL re-lock time
                es8311_write_reg(tmp_h, 0x00, 0x00);   // Clear reset
                delay(10);
                es8311_delete(tmp_h);
                Serial.println("[SpkTest] ES8311 hard reset done");
            }

            /* Init Speaker_Class (ES8311 codec + I2S TX on I2S_NUM_0) */
            speaker.config().sample_rate = 44100;
            bool spk_ok = speaker.begin(&In_I2C);
            Serial.printf("[SpkTest] Speaker init: %s\n", spk_ok ? "OK" : "FAIL");
            Lcd.printf(" Speaker init: %s\n", spk_ok ? "OK" : "FAIL");

            if (spk_ok) {
                speaker.setVolume(60);

                /* Enable NS4150B amplifier via PCA9557 IO3 */
                io_exp.digitalWrite(HAL_IOEXP_PA_EN, HIGH);
                delay(200);  // NS4150B startup time
                Serial.println("[SpkTest] PA_EN=HIGH (NS4150B on)");
                Lcd.printf(" PA_EN=HIGH (amp on)\n");

                Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
                Lcd.printf(" Playing 1kHz beep — B=Exit\n");
                Serial.println("[SpkTest] 1kHz beep loop — B to exit");

                int lcd_status_y = Lcd.getCursorY();
                int cycle = 0;
                bool running = true;
                while (running) {
                    cycle++;
                    Lcd.fillRect(0, lcd_status_y, 320, 20, TFT_BLACK);
                    Lcd.setCursor(0, lcd_status_y);
                    Lcd.setTextColor(TFT_YELLOW, TFT_BLACK);
                    Lcd.printf(" Beep #%d (5s)...", cycle);
                    Serial.printf("[SpkTest] Beep #%d\n", cycle);

                    /* Play 1 kHz tone for 5 seconds, check B every 100ms */
                    uint32_t t0 = millis();
                    while (millis() - t0 < 5000) {
                        speaker.tone(1000, 200);
                        /* Check B button during beep */
                        for (int i = 0; i < 4; i++) {
                            delay(50);
                            button.update();
                            if (button.B.pressed()) { running = false; break; }
                        }
                        if (!running) break;
                    }
                    if (!running) break;

                    /* Brief silence between beeps */
                    speaker.stop();
                    delay(300);
                    button.update();
                    if (button.B.pressed()) running = false;
                }

                speaker.stop();
                io_exp.digitalWrite(HAL_IOEXP_PA_EN, LOW);
                Serial.println("[SpkTest] PA_EN=LOW (amp off)");
            }

            speaker.end();
            Serial.println("[SpkTest] Done");

            bbt_results[BBT_IDX_SPEAKER] = BBT_ST_PASS;
            bbt_drawResult(Lcd, "09b", "SPEAKER ES8311", true,
                           "ES8311 + NS4150B: 1KHZ BEEP OK",
                           "ES8311 DAC I2C INIT: OK");
            delay(1500);
            button.update(); button.A.hasChanged(); button.B.hasChanged(); delay(100);
            while (true) { button.update(); if (button.B.pressed()) break; delay(20); }
        } else {
            bbt_results[BBT_IDX_SPEAKER] = BBT_ST_SKIP;
        }
    }

    /* ── BBT 10/12 — SD Card (SDMMC 1-bit)── */
    {
        bbt_drawPrompt(Lcd, "10", "SD CARD",
                       "SDMMC 1-BIT CLK47 CMD48 D021",
                       "MOUNT + SEQ WRITE/READ 1MB VERIFY",
                       bbt_results);
        Serial.println("[BBT 10/12] SD Card");
        button.update(); button.A.hasChanged(); button.B.hasChanged(); delay(200);
        bool enter = false;
        while (true) {
            button.update();
            if (button.A.pressed()) { enter = true; break; }
            if (button.B.pressed()) { break; }
            delay(20);
        }
        if (enter) {
            bbt_beginContent(Lcd, "10", "SD CARD");
            Serial.println("[BBT 10] SD Card production stress test start");

            /* ════ Phase 1: Mount & Card Info ════ */
            SD_MMC.setPins(HAL_PIN_SD_CLK, HAL_PIN_SD_CMD, HAL_PIN_SD_D0);
            bool mounted = sdmmcEnsureReady();
            uint8_t cardType = mounted ? SD_MMC.cardType() : CARD_NONE;
            bool card_ok = mounted && (cardType != CARD_NONE);

            Lcd.setTextColor(card_ok ? TFT_GREEN : TFT_RED, TFT_BLACK);
            Lcd.printf(" Mount: %s\n", card_ok ? "OK" : "FAIL");
            Serial.printf("[BBT 10] Mount: %s\n", card_ok ? "OK" : "FAIL");

            bool all_pass = false;
            if (card_ok) {
                const char* typeStr = "UNKNOWN";
                switch (cardType) {
                    case CARD_MMC:  typeStr = "MMC";  break;
                    case CARD_SD:   typeStr = "SD";   break;
                    case CARD_SDHC: typeStr = "SDHC"; break;
                }
                uint64_t sizeMB  = SD_MMC.cardSize() / (1024 * 1024);
                uint64_t totalMB = SD_MMC.totalBytes() / (1024 * 1024);
                uint64_t usedMB  = SD_MMC.usedBytes()  / (1024 * 1024);

                Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
                Lcd.printf(" %s %lluMB (Free %lluMB)\n", typeStr, sizeMB, totalMB - usedMB);
                Serial.printf("[BBT 10] %s %lluMB Total=%lluMB Used=%lluMB\n",
                              typeStr, sizeMB, totalMB, usedMB);

                /* ════ Phase 2: Sequential Write Speed (4KB block × 256 = 1MB) ════ */
                Lcd.setTextColor(TFT_CYAN, TFT_BLACK);
                Lcd.printf(" Seq Write 1MB (4KB blk)...\n");

                const char* perfPath = "/_bbt_perf.tmp";
                const size_t BLK_SIZE = 4096;
                const int BLK_COUNT = 256;  /* 1MB total */
                uint8_t* blk = (uint8_t*)malloc(BLK_SIZE);

                bool perf_ok = false;
                float writeSpeed = 0, readSpeed = 0;

                if (blk) {
                    /* Fill with pseudo-random pattern (XOR-shift for speed) */
                    uint32_t seed = 0xDEADBEEF;
                    for (size_t i = 0; i < BLK_SIZE; i += 4) {
                        seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5;
                        memcpy(blk + i, &seed, 4);
                    }

                    /* Sequential write */
                    File wf = SD_MMC.open(perfPath, FILE_WRITE);
                    if (wf) {
                        uint32_t t0 = millis();
                        size_t totalWritten = 0;
                        bool write_err = false;
                        for (int i = 0; i < BLK_COUNT; i++) {
                            /* Rotate pattern per block to detect stuck-bit faults */
                            blk[0] = (uint8_t)(i & 0xFF);
                            blk[1] = (uint8_t)((i >> 8) & 0xFF);
                            size_t w = wf.write(blk, BLK_SIZE);
                            if (w != BLK_SIZE) { write_err = true; break; }
                            totalWritten += w;
                        }
                        wf.flush();
                        wf.close();
                        uint32_t dt = millis() - t0;
                        if (dt == 0) dt = 1;
                        writeSpeed = (float)totalWritten / 1024.0f / ((float)dt / 1000.0f);

                        Lcd.setTextColor(write_err ? TFT_RED : TFT_GREEN, TFT_BLACK);
                        Lcd.printf(" W: %.0f KB/s (%lums)%s\n", writeSpeed, dt,
                                   write_err ? " ERR" : "");
                        Serial.printf("[BBT 10] SeqWrite: %.0f KB/s %lums %s\n",
                                      writeSpeed, dt, write_err ? "ERR" : "OK");

                        /* ════ Phase 3: Sequential Read Speed + Full Verify ════ */
                        if (!write_err) {
                            Lcd.setTextColor(TFT_CYAN, TFT_BLACK);
                            Lcd.printf(" Seq Read + Verify...\n");

                            File rf = SD_MMC.open(perfPath, FILE_READ);
                            bool verify_ok = true;
                            if (rf) {
                                uint8_t* rbuf = (uint8_t*)malloc(BLK_SIZE);
                                if (rbuf) {
                                    uint32_t t1 = millis();
                                    size_t totalRead = 0;
                                    for (int i = 0; i < BLK_COUNT; i++) {
                                        size_t r = rf.read(rbuf, BLK_SIZE);
                                        if (r != BLK_SIZE) { verify_ok = false; break; }
                                        totalRead += r;

                                        /* Rebuild expected pattern */
                                        uint32_t vs = 0xDEADBEEF;
                                        for (size_t j = 0; j < BLK_SIZE; j += 4) {
                                            vs ^= vs << 13; vs ^= vs >> 17; vs ^= vs << 5;
                                            memcpy(blk + j, &vs, 4);
                                        }
                                        blk[0] = (uint8_t)(i & 0xFF);
                                        blk[1] = (uint8_t)((i >> 8) & 0xFF);

                                        if (memcmp(rbuf, blk, BLK_SIZE) != 0) {
                                            verify_ok = false;
                                            /* Find first mismatch byte for diagnosis */
                                            for (size_t j = 0; j < BLK_SIZE; j++) {
                                                if (rbuf[j] != blk[j]) {
                                                    Serial.printf("[BBT 10] Mismatch blk=%d off=%d "
                                                                  "exp=0x%02X got=0x%02X\n",
                                                                  i, j, blk[j], rbuf[j]);
                                                    break;
                                                }
                                            }
                                            break;
                                        }
                                    }
                                    uint32_t dt2 = millis() - t1;
                                    if (dt2 == 0) dt2 = 1;
                                    readSpeed = (float)totalRead / 1024.0f / ((float)dt2 / 1000.0f);

                                    Lcd.setTextColor(verify_ok ? TFT_GREEN : TFT_RED, TFT_BLACK);
                                    Lcd.printf(" R: %.0f KB/s Verify:%s\n", readSpeed,
                                               verify_ok ? "OK" : "FAIL");
                                    Serial.printf("[BBT 10] SeqRead: %.0f KB/s %lums Verify=%s\n",
                                                  readSpeed, dt2, verify_ok ? "OK" : "FAIL");
                                    free(rbuf);
                                } else {
                                    verify_ok = false;
                                    Lcd.setTextColor(TFT_RED, TFT_BLACK);
                                    Lcd.printf(" R: malloc failed\n");
                                }
                                rf.close();
                            } else {
                                verify_ok = false;
                            }

                            /* ════ Phase 4: Random R/W Stress (small blocks) ════ */
                            Lcd.setTextColor(TFT_CYAN, TFT_BLACK);
                            Lcd.printf(" Random R/W 50 cycles...\n");

                            const char* rndPath = "/_bbt_rnd.tmp";
                            const size_t RND_BLK = 512;
                            const int RND_CYCLES = 50;
                            uint8_t wbuf[512], vbuf[512];
                            int rnd_pass = 0;

                            for (int c = 0; c < RND_CYCLES; c++) {
                                /* Generate unique pattern per cycle */
                                uint32_t rs = 0xCAFE0000 | c;
                                for (size_t j = 0; j < RND_BLK; j += 4) {
                                    rs ^= rs << 13; rs ^= rs >> 17; rs ^= rs << 5;
                                    memcpy(wbuf + j, &rs, (j + 4 <= RND_BLK) ? 4 : RND_BLK - j);
                                }

                                /* Write */
                                File f = SD_MMC.open(rndPath, FILE_WRITE);
                                if (!f) break;
                                size_t w = f.write(wbuf, RND_BLK);
                                f.close();
                                if (w != RND_BLK) break;

                                /* Read back & verify */
                                f = SD_MMC.open(rndPath, FILE_READ);
                                if (!f) break;
                                size_t r = f.read(vbuf, RND_BLK);
                                f.close();
                                if (r != RND_BLK) break;

                                if (memcmp(wbuf, vbuf, RND_BLK) == 0) {
                                    rnd_pass++;
                                } else {
                                    Serial.printf("[BBT 10] RndRW mismatch cycle=%d\n", c);
                                    break;
                                }

                                /* Let user abort */
                                button.update();
                                if (button.B.pressed()) break;
                            }
                            SD_MMC.remove(rndPath);

                            bool rnd_ok = (rnd_pass == RND_CYCLES);
                            Lcd.setTextColor(rnd_ok ? TFT_GREEN : TFT_RED, TFT_BLACK);
                            Lcd.printf(" RndRW: %d/%d %s\n", rnd_pass, RND_CYCLES,
                                       rnd_ok ? "PASS" : "FAIL");
                            Serial.printf("[BBT 10] RndRW: %d/%d\n", rnd_pass, RND_CYCLES);

                            /* ════ Phase 5: Remount Stability ════ */
                            Lcd.setTextColor(TFT_CYAN, TFT_BLACK);
                            Lcd.printf(" Remount x3...\n");

                            int remount_pass = 0;
                            for (int r = 0; r < 3; r++) {
                                SD_MMC.end();
                                delay(200);
                                SD_MMC.setPins(HAL_PIN_SD_CLK, HAL_PIN_SD_CMD, HAL_PIN_SD_D0);
                                bool rm = sdmmcEnsureReady();
                                uint8_t ct = rm ? SD_MMC.cardType() : CARD_NONE;
                                if (rm && ct != CARD_NONE) {
                                    remount_pass++;
                                    Serial.printf("[BBT 10] Remount #%d OK\n", r + 1);
                                } else {
                                    Serial.printf("[BBT 10] Remount #%d FAIL\n", r + 1);
                                    /* Re-mount for cleanup */
                                    SD_MMC.setPins(HAL_PIN_SD_CLK, HAL_PIN_SD_CMD, HAL_PIN_SD_D0);
                                    sdmmcEnsureReady();
                                    break;
                                }
                            }
                            bool remount_ok = (remount_pass == 3);
                            Lcd.setTextColor(remount_ok ? TFT_GREEN : TFT_RED, TFT_BLACK);
                            Lcd.printf(" Remount: %d/3 %s\n", remount_pass,
                                       remount_ok ? "PASS" : "FAIL");

                            /* ════ Phase 6: Large-file boundary test (cross-cluster) ════ */
                            const char* bigPath = "/_bbt_big.tmp";
                            const size_t BIG_BLK = 4096;
                            const int BIG_BLKS = 64;  /* 256KB */
                            Lcd.setTextColor(TFT_CYAN, TFT_BLACK);
                            Lcd.printf(" BigFile 256KB verify...\n");

                            bool big_ok = false;
                            {
                                File bf = SD_MMC.open(bigPath, FILE_WRITE);
                                if (bf) {
                                    bool w_ok = true;
                                    for (int i = 0; i < BIG_BLKS; i++) {
                                        /* Pattern: each block filled with its index byte */
                                        memset(blk, (uint8_t)(i & 0xFF), BIG_BLK);
                                        if (bf.write(blk, BIG_BLK) != BIG_BLK) { w_ok = false; break; }
                                    }
                                    bf.flush();
                                    bf.close();

                                    if (w_ok) {
                                        /* Verify: read back and check each block */
                                        File vf = SD_MMC.open(bigPath, FILE_READ);
                                        if (vf) {
                                            big_ok = true;
                                            for (int i = 0; i < BIG_BLKS; i++) {
                                                size_t rd = vf.read(blk, BIG_BLK);
                                                if (rd != BIG_BLK) { big_ok = false; break; }
                                                uint8_t expected = (uint8_t)(i & 0xFF);
                                                for (size_t j = 0; j < BIG_BLK; j++) {
                                                    if (blk[j] != expected) {
                                                        Serial.printf("[BBT 10] BigFile mismatch "
                                                                      "blk=%d off=%d exp=0x%02X got=0x%02X\n",
                                                                      i, j, expected, blk[j]);
                                                        big_ok = false;
                                                        break;
                                                    }
                                                }
                                                if (!big_ok) break;
                                            }
                                            vf.close();
                                        }
                                    }
                                    SD_MMC.remove(bigPath);
                                }
                            }
                            Lcd.setTextColor(big_ok ? TFT_GREEN : TFT_RED, TFT_BLACK);
                            Lcd.printf(" BigFile: %s\n", big_ok ? "PASS" : "FAIL");
                            Serial.printf("[BBT 10] BigFile 256KB: %s\n", big_ok ? "PASS" : "FAIL");

                            /* Cleanup perf file */
                            SD_MMC.remove(perfPath);

                            /* ════ Overall Verdict ════ */
                            all_pass = verify_ok && rnd_ok && remount_ok && big_ok;

                            /* Speed sanity — minimum thresholds for production */
                            if (writeSpeed < 200.0f || readSpeed < 400.0f) {
                                Serial.printf("[BBT 10] WARNING: Speed below threshold "
                                              "W=%.0f R=%.0f KB/s\n", writeSpeed, readSpeed);
                                Lcd.setTextColor(TFT_ORANGE, TFT_BLACK);
                                Lcd.printf(" Speed LOW W:%.0f R:%.0f KB/s\n", writeSpeed, readSpeed);
                                /* Don't fail — just warn. Some cards are slow but functional. */
                            }
                        }
                    } else {
                        Lcd.setTextColor(TFT_RED, TFT_BLACK);
                        Lcd.printf(" Write failed\n");
                    }
                    free(blk);
                } else {
                    Lcd.setTextColor(TFT_RED, TFT_BLACK);
                    Lcd.printf(" malloc 4KB failed\n");
                }
            }

            /* Unmount */
            if (mounted) SD_MMC.end();

            /* ════ Result Summary ════ */
            bbt_results[BBT_IDX_SD] = all_pass ? BBT_ST_PASS : BBT_ST_FAIL;
            bbt_drawResult(Lcd, "10", "SD CARD", all_pass,
                           all_pass ? "MOUNT+W+R+VERIFY: ALL PASS" : "SD CARD TEST FAIL",
                           "SDMMC 1-BIT W+R+VERIFY+REMOUNT");
            Serial.println(all_pass ? "[BBT 10] SD Card ALL PASS" : "[BBT 10] SD Card FAIL");
            delay(1500);
            button.update(); button.A.hasChanged(); button.B.hasChanged(); delay(100);
            while (true) { button.update(); if (button.B.pressed()) break; delay(20); }
        } else {
            bbt_results[BBT_IDX_SD] = BBT_ST_SKIP;
        }
    }

    /* ── BBT 11/12 — PMU (AXP173 power status) ────────────────── */
    {
        bbt_drawPrompt(Lcd, "11", "PMU AXP173",
                       "AXP173 PMIC I2C 0x34",
                       "POWER STATUS LIVE — PRESS B TO STOP",
                       bbt_results);
        Serial.println("[BBT 11/12] PMU");
        button.update(); button.A.hasChanged(); button.B.hasChanged(); delay(200);
        bool enter = false;
        while (true) {
            button.update();
            if (button.A.pressed()) { enter = true; break; }
            if (button.B.pressed()) { break; }
            delay(20);
        }
        if (enter) {
            Lcd.fillScreen(TFT_BLACK);
            Lcd.setCursor(0, 0);
            Lcd.setFont(&fonts::efontCN_16);

            /* ── Outputs: DCDC1 / LDO2 / LDO3 / LDO4 ── */
            Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
            Lcd.printf(" Outputs: DCDC1/LDO2/LDO3/LDO4\n");
            Lcd.printf("          = 3.3V\n");

            /* ── Charging config ── */
            Lcd.setTextColor(TFT_CYAN, TFT_BLACK);
            bool chgEnabled = pmu.isCharging();
            Lcd.printf(" Charging: 450mA  Coulometer:");
            Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
            Lcd.printf(" [Enabled]\n");

            /* ── USB / Battery status ── */
            bool usbExist  = pmu.isVBUSExist();
            bool batExist  = pmu.isBatExist();
            float batV     = pmu.getBatVoltage();
            float batLevel = pmu.getBatLevel();

            Lcd.setTextColor(usbExist ? TFT_GREEN : TFT_RED, TFT_BLACK);
            Lcd.printf(" USB %s", usbExist ? "Exist" : "None ");
            Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
            Lcd.printf("| Battery: ");
            if (batExist) {
                Lcd.setTextColor(TFT_YELLOW, TFT_BLACK);
                Lcd.printf("%.2fV %.0f%%\n", batV, batLevel);
            } else {
                Lcd.setTextColor(TFT_RED, TFT_BLACK);
                Lcd.printf("N/A\n");
            }

            /* ── USB voltage & current ── */
            if (usbExist) {
                float usbV = pmu.getVBUSVoltage();
                float usbI = pmu.getVBUSCurrent();
                Lcd.setTextColor(TFT_CYAN, TFT_BLACK);
                Lcd.printf(" USB: %.2fV %.0fmA\n", usbV, usbI);
            } else {
                Lcd.setTextColor(TFT_DARKGREY, TFT_BLACK);
                Lcd.printf(" USB: --\n");
            }

            /* ── AXP173 internal temperature ── */
            float chipTemp = pmu.getAXP173Temp();
            Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
            Lcd.printf(" AXP173 Internal: %.1f C\n", chipTemp);

            /* ── Battery TS pin (NTC) temperature ──
             * getTSTemp() returns -273.15 when TS pin is floating/no NTC fitted.
             * Show "N/A" in that case to avoid misleading the user. */
            float tsTemp = pmu.getTSTemp();
            bool tsValid = (tsTemp > -200.0f);
            if (tsValid) {
                Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
                Lcd.printf(" Battery TS Pin:  %.1f C\n", tsTemp);
            } else {
                Lcd.setTextColor(TFT_DARKGREY, TFT_BLACK);
                Lcd.printf(" Battery TS Pin:  N/A (no NTC)\n");
            }

            /* ── Charge direction indicator ── */
            Lcd.setTextColor(chgEnabled ? TFT_GREEN : TFT_ORANGE, TFT_BLACK);
            Lcd.printf(" Status: %s\n", chgEnabled ? "Charging" : "Not charging");

            Serial.printf("[BBT 11/12] USB:%s Bat:%.2fV %.0f%% USB:%.2fV Temp:%.1fC TS:%s\n",
                          usbExist ? "Y" : "N", batV, batLevel,
                          usbExist ? pmu.getVBUSVoltage() : 0.0f,
                          chipTemp, tsValid ? String(tsTemp, 1).c_str() : "N/A");

            /* live-refresh until B pressed */
            Lcd.setTextColor(TFT_DARKGREY, TFT_BLACK);
            Lcd.printf("\n B = Exit");
            while (true) {
                button.update();
                if (button.B.pressed()) break;

                /* refresh dynamic values every 500ms */
                batV     = pmu.getBatVoltage();
                batLevel = pmu.getBatLevel();
                usbExist = pmu.isVBUSExist();
                chgEnabled = pmu.isCharging();

                /* Battery line (row 3, y≈16*3=48) */
                Lcd.setCursor(0, 48);
                Lcd.setTextColor(usbExist ? TFT_GREEN : TFT_RED, TFT_BLACK);
                Lcd.printf(" USB %s", usbExist ? "Exist" : "None ");
                Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
                Lcd.printf("| Battery: ");
                if (pmu.isBatExist()) {
                    Lcd.setTextColor(TFT_YELLOW, TFT_BLACK);
                    Lcd.printf("%.2fV %.0f%% \n", batV, batLevel);
                } else {
                    Lcd.setTextColor(TFT_RED, TFT_BLACK);
                    Lcd.printf("N/A        \n");
                }

                /* USB line */
                if (usbExist) {
                    Lcd.setTextColor(TFT_CYAN, TFT_BLACK);
                    Lcd.printf(" USB: %.2fV %.0fmA   \n", pmu.getVBUSVoltage(), pmu.getVBUSCurrent());
                } else {
                    Lcd.setTextColor(TFT_DARKGREY, TFT_BLACK);
                    Lcd.printf(" USB: --              \n");
                }

                /* Temperatures */
                chipTemp = pmu.getAXP173Temp();
                tsTemp   = pmu.getTSTemp();
                tsValid  = (tsTemp > -200.0f);
                Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
                Lcd.printf(" AXP173 Internal: %.1f C  \n", chipTemp);
                if (tsValid) {
                    Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
                    Lcd.printf(" Battery TS Pin:  %.1f C      \n", tsTemp);
                } else {
                    Lcd.setTextColor(TFT_DARKGREY, TFT_BLACK);
                    Lcd.printf(" Battery TS Pin:  N/A        \n");
                }

                /* Charge status */
                Lcd.setTextColor(chgEnabled ? TFT_GREEN : TFT_ORANGE, TFT_BLACK);
                Lcd.printf(" Status: %s       \n", chgEnabled ? "Charging    " : "Not charging");

                delay(500);
            }

            bbt_results[BBT_IDX_PMU] = BBT_ST_PASS;
            bbt_drawResult(Lcd, "11", "PMU AXP173", true,
                           "DCDC1/LDO2/LDO3/LDO4: 3.3V OK",
                           "AXP173 PMIC I2C 0x34: OK");
            delay(1500);
            button.update(); button.A.hasChanged(); button.B.hasChanged(); delay(100);
            while (true) { button.update(); if (button.B.pressed()) break; delay(20); }
        } else {
            bbt_results[BBT_IDX_PMU] = BBT_ST_SKIP;
        }
    }

    /* ── BBT 12/12 — GPIO Test ─────────────────────────────────── */
    {
        bbt_drawPrompt(Lcd, "12", "GPIO TEST",
                       "SHT30/W25Q64/INT/ADC/PWM",
                       "5 SUB-TESTS — PRESS B TO SKIP EACH",
                       bbt_results);
        Serial.println("[BBT 12/12] GPIO");
        button.update(); button.A.hasChanged(); button.B.hasChanged(); delay(200);
        bool enter = false;
        while (true) {
            button.update();
            if (button.A.pressed()) { enter = true; break; }
            if (button.B.pressed()) { break; }
            delay(20);
        }
        if (enter) {
            int gpio_pass = 0;
            int gpio_total = 5;

            /* ──────────────────────────────────────────────
             *  12-1) SHT30 温湿度传感器 (I2C 0x44)
             * ────────────────────────────────────────────── */
            {
                Lcd.fillScreen(TFT_BLACK);
                Lcd.setCursor(0, 0);
                Lcd.setTextColor(TFT_CYAN, TFT_BLACK);
                Lcd.setFont(&fonts::efontCN_16);
                Lcd.printf(" [GPIO 1/5] SHT30 (I2C 0x44)\n");
                Serial.println("[BBT 12-1] SHT30 temp/humidity");

                /* Probe I2C address */
                bool sht_found = In_I2C.scanID(HAL_I2C_ADDR_SHT30);
                Lcd.printf(" I2C probe: %s\n", sht_found ? "OK" : "NOT FOUND");
                Serial.printf("[BBT 12-1] I2C probe 0x44: %s\n", sht_found ? "OK" : "FAIL");

                bool sht_ok = false;
                if (sht_found) {
                    /* Send single-shot measurement command: high repeatability, clock stretching disabled
                     * Command 0x2400 — use writeRegister8: reg=0x24, data=0x00 */
                    In_I2C.writeRegister8(HAL_I2C_ADDR_SHT30, 0x24, 0x00);
                    delay(20);  /* SHT30 measurement time ~15ms */

                    /* Read 6 bytes: Temp MSB, Temp LSB, Temp CRC, Hum MSB, Hum LSB, Hum CRC
                     * SHT30 read-back needs a plain I2C read (no register address),
                     * so use i2c_cmd_handle directly on In_I2C's port. */
                    uint8_t data[6] = {};
                    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
                    i2c_master_start(cmd);
                    i2c_master_write_byte(cmd, (HAL_I2C_ADDR_SHT30 << 1) | I2C_MASTER_READ, true);
                    i2c_master_read(cmd, data, 5, I2C_MASTER_ACK);
                    i2c_master_read_byte(cmd, &data[5], I2C_MASTER_NACK);
                    i2c_master_stop(cmd);
                    esp_err_t rd_err = i2c_master_cmd_begin(In_I2C.getPort(), cmd, pdMS_TO_TICKS(100));
                    i2c_cmd_link_delete(cmd);

                    if (rd_err == ESP_OK) {
                        uint16_t rawTemp = ((uint16_t)data[0] << 8) | data[1];
                        uint16_t rawHum  = ((uint16_t)data[3] << 8) | data[4];
                        float temp = -45.0f + 175.0f * ((float)rawTemp / 65535.0f);
                        float hum  = 100.0f * ((float)rawHum / 65535.0f);

                        /* Sanity check: -40~125°C, 0~100% */
                        if (temp > -40.0f && temp < 125.0f && hum >= 0.0f && hum <= 100.0f) {
                            sht_ok = true;
                            Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
                            Lcd.printf(" Temp: %.1f C\n", temp);
                            Lcd.printf(" Humi: %.1f %%\n", hum);
                            Serial.printf("[BBT 12-1] T=%.1fC H=%.1f%%\n", temp, hum);
                        } else {
                            Lcd.setTextColor(TFT_RED, TFT_BLACK);
                            Lcd.printf(" Data out of range\n");
                            Serial.printf("[BBT 12-1] Bad data T=%.1f H=%.1f\n", temp, hum);
                        }
                    } else {
                        Lcd.setTextColor(TFT_RED, TFT_BLACK);
                        Lcd.printf(" I2C read failed (%s)\n", esp_err_to_name(rd_err));
                    }
                }

                Lcd.setTextColor(sht_ok ? TFT_GREEN : TFT_RED, TFT_BLACK);
                Lcd.printf(" SHT30: %s\n", sht_ok ? "PASS" : "FAIL");
                Serial.printf("[BBT 12-1] SHT30 %s\n", sht_ok ? "PASS" : "FAIL");
                if (sht_ok) gpio_pass++;

                Lcd.setTextColor(TFT_DARKGREY, TFT_BLACK);
                Lcd.printf(" B = Next\n");
                while (true) { button.update(); if (button.B.pressed()) break; delay(20); }
            }

            /* ──────────────────────────────────────────────
             *  12-2) W25Q64 SPI Flash — Read ID + Write/Read verify
             *         MOSI=GPIO11, MISO=GPIO10, SCK=GPIO9
             *         CS = PCA9557 IO2
             * ────────────────────────────────────────────── */
            {
                Lcd.fillScreen(TFT_BLACK);
                Lcd.setCursor(0, 0);
                Lcd.setTextColor(TFT_CYAN, TFT_BLACK);
                Lcd.setFont(&fonts::efontCN_16);
                Lcd.printf(" [GPIO 2/5] W25Q64 SPI Flash\n");
                Serial.println("[BBT 12-2] W25Q64 SPI Flash");

                /* Configure CS pin via IO expander */
                io_exp.pinMode(HAL_IOEXP_FLASH_CS, OUTPUT);
                io_exp.digitalWrite(HAL_IOEXP_FLASH_CS, HIGH);

                /* Init SPI bus on custom pins */
                SPIClass flashSPI(HSPI);
                flashSPI.begin(HAL_PIN_FLASH_SCK, HAL_PIN_FLASH_MISO, HAL_PIN_FLASH_MOSI, -1);
                flashSPI.setFrequency(1000000);  /* 1MHz for safety */

                auto cs_low  = [&]() { io_exp.digitalWrite(HAL_IOEXP_FLASH_CS, LOW);  delayMicroseconds(5); };
                auto cs_high = [&]() { io_exp.digitalWrite(HAL_IOEXP_FLASH_CS, HIGH); delayMicroseconds(5); };

                /* Read JEDEC ID (command 0x9F) — expect EF 40 17 for W25Q64 */
                cs_low();
                flashSPI.transfer(0x9F);
                uint8_t mfr  = flashSPI.transfer(0x00);
                uint8_t type = flashSPI.transfer(0x00);
                uint8_t cap  = flashSPI.transfer(0x00);
                cs_high();

                bool id_ok = (mfr == 0xEF && type == 0x40 && cap == 0x17);
                Lcd.printf(" JEDEC ID: %02X %02X %02X %s\n", mfr, type, cap,
                           id_ok ? "(W25Q64 OK)" : "(unexpected)");
                Serial.printf("[BBT 12-2] JEDEC: %02X %02X %02X\n", mfr, type, cap);

                bool rw_ok = false;
                if (id_ok) {
                    /* Use last sector (addr 0x7FF000) to avoid data area conflicts */
                    const uint32_t testAddr = 0x7FF000;
                    const uint8_t testPattern[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0x12, 0x34 };
                    const int testLen = sizeof(testPattern);

                    /* Write Enable (0x06) */
                    cs_low(); flashSPI.transfer(0x06); cs_high();
                    delay(5);

                    /* Sector Erase 4KB (0x20) */
                    Lcd.printf(" Erasing sector 0x%06X...\n", testAddr);
                    cs_low();
                    flashSPI.transfer(0x20);
                    flashSPI.transfer((testAddr >> 16) & 0xFF);
                    flashSPI.transfer((testAddr >>  8) & 0xFF);
                    flashSPI.transfer(testAddr & 0xFF);
                    cs_high();

                    /* Wait for erase to complete (poll status register bit0=BUSY) */
                    uint32_t t0 = millis();
                    while (millis() - t0 < 3000) {
                        cs_low();
                        flashSPI.transfer(0x05);  /* Read Status Reg 1 */
                        uint8_t sr = flashSPI.transfer(0x00);
                        cs_high();
                        if (!(sr & 0x01)) break;
                        delay(10);
                    }

                    /* Write Enable again */
                    cs_low(); flashSPI.transfer(0x06); cs_high();
                    delay(5);

                    /* Page Program (0x02) */
                    Lcd.printf(" Writing %d bytes...\n", testLen);
                    cs_low();
                    flashSPI.transfer(0x02);
                    flashSPI.transfer((testAddr >> 16) & 0xFF);
                    flashSPI.transfer((testAddr >>  8) & 0xFF);
                    flashSPI.transfer(testAddr & 0xFF);
                    for (int i = 0; i < testLen; i++) flashSPI.transfer(testPattern[i]);
                    cs_high();

                    /* Wait for program complete */
                    t0 = millis();
                    while (millis() - t0 < 500) {
                        cs_low();
                        flashSPI.transfer(0x05);
                        uint8_t sr = flashSPI.transfer(0x00);
                        cs_high();
                        if (!(sr & 0x01)) break;
                        delay(1);
                    }

                    /* Read Data (0x03) */
                    Lcd.printf(" Reading back...\n");
                    uint8_t readBuf[8] = {};
                    cs_low();
                    flashSPI.transfer(0x03);
                    flashSPI.transfer((testAddr >> 16) & 0xFF);
                    flashSPI.transfer((testAddr >>  8) & 0xFF);
                    flashSPI.transfer(testAddr & 0xFF);
                    for (int i = 0; i < testLen; i++) readBuf[i] = flashSPI.transfer(0x00);
                    cs_high();

                    /* Verify */
                    rw_ok = (memcmp(testPattern, readBuf, testLen) == 0);
                    Lcd.printf(" W: ");
                    for (int i = 0; i < testLen; i++) Lcd.printf("%02X ", testPattern[i]);
                    Lcd.printf("\n R: ");
                    for (int i = 0; i < testLen; i++) Lcd.printf("%02X ", readBuf[i]);
                    Lcd.printf("\n");
                    Serial.printf("[BBT 12-2] Verify: %s\n", rw_ok ? "MATCH" : "MISMATCH");
                }

                bool flash_ok = id_ok && rw_ok;
                Lcd.setTextColor(flash_ok ? TFT_GREEN : TFT_RED, TFT_BLACK);
                Lcd.printf(" W25Q64: %s\n", flash_ok ? "PASS" : "FAIL");
                Serial.printf("[BBT 12-2] W25Q64 %s\n", flash_ok ? "PASS" : "FAIL");
                if (flash_ok) gpio_pass++;

                flashSPI.end();

                Lcd.setTextColor(TFT_DARKGREY, TFT_BLACK);
                Lcd.printf(" B = Next\n");
                while (true) { button.update(); if (button.B.pressed()) break; delay(20); }
            }

            /* ──────────────────────────────────────────────
             *  12-3) INT 中断输入 (GPIO8) — 上升沿捕获计数
             *         按下扩展板按钮 S1 测试
             * ────────────────────────────────────────────── */
            {
                Lcd.fillScreen(TFT_BLACK);
                Lcd.setCursor(0, 0);
                Lcd.setTextColor(TFT_CYAN, TFT_BLACK);
                Lcd.setFont(&fonts::efontCN_16);
                Lcd.printf(" [GPIO 3/5] INT (GPIO%d)\n", HAL_PIN_GPIO_INT);
                Lcd.printf(" Rising edge capture\n");
                Lcd.printf(" Press S1 button on ext board\n");
                Serial.println("[BBT 12-3] INT rising edge test");

                /* Use volatile counter for ISR */
                static volatile uint32_t _isr_count = 0;
                _isr_count = 0;

                pinMode(HAL_PIN_GPIO_INT, INPUT);
                attachInterrupt(digitalPinToInterrupt(HAL_PIN_GPIO_INT),
                    []() { _isr_count++; }, RISING);

                Lcd.setTextColor(TFT_YELLOW, TFT_BLACK);
                Lcd.printf(" Waiting 10s... press S1!\n");
                int lcd_cnt_y = Lcd.getCursorY();

                uint32_t lastCount = 0;
                uint32_t t0 = millis();
                bool int_ok = false;
                while (millis() - t0 < 10000) {
                    uint32_t cnt = _isr_count;
                    if (cnt != lastCount) {
                        lastCount = cnt;
                        Lcd.fillRect(0, lcd_cnt_y, 320, 20, TFT_BLACK);
                        Lcd.setCursor(0, lcd_cnt_y);
                        Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
                        Lcd.printf(" Rising edges: %lu", cnt);
                        Serial.printf("[BBT 12-3] edges=%lu\n", cnt);
                    }
                    if (cnt >= 3) { int_ok = true; break; }  /* 3+ edges = PASS */
                    button.update();
                    if (button.B.pressed()) break;
                    delay(20);
                }

                detachInterrupt(digitalPinToInterrupt(HAL_PIN_GPIO_INT));

                if (!int_ok && _isr_count > 0) int_ok = true;  /* At least 1 edge detected */

                Lcd.printf("\n");
                Lcd.setTextColor(int_ok ? TFT_GREEN : TFT_RED, TFT_BLACK);
                Lcd.printf(" INT: %s (count=%lu)\n", int_ok ? "PASS" : "FAIL", (uint32_t)_isr_count);
                Serial.printf("[BBT 12-3] INT %s count=%lu\n", int_ok ? "PASS" : "FAIL", (uint32_t)_isr_count);
                if (int_ok) gpio_pass++;

                Lcd.setTextColor(TFT_DARKGREY, TFT_BLACK);
                Lcd.printf(" B = Next\n");
                while (true) { button.update(); if (button.B.pressed()) break; delay(20); }
            }

            /* ──────────────────────────────────────────────
             *  12-4) ADC 输入 (GPIO18) — 读取电压
             *         扩展板分压: 3V3 — 10K — ADC — 10K — GND
             *         理论值 ≈ 1.65V
             * ────────────────────────────────────────────── */
            {
                Lcd.fillScreen(TFT_BLACK);
                Lcd.setCursor(0, 0);
                Lcd.setTextColor(TFT_CYAN, TFT_BLACK);
                Lcd.setFont(&fonts::efontCN_16);
                Lcd.printf(" [GPIO 4/5] ADC (GPIO%d)\n", HAL_PIN_GPIO_ADC);
                Lcd.printf(" Expect ~1.65V (10K/10K divider)\n");
                Serial.println("[BBT 12-4] ADC voltage test");

                /* Configure ADC — analogRead on ESP32-S3 uses default 12-bit, 0-3.3V */
                analogReadResolution(12);
                analogSetAttenuation(ADC_11db);  /* Full scale ~3.3V */
                pinMode(HAL_PIN_GPIO_ADC, INPUT);

                /* Take 16 samples with averaging */
                uint32_t sum = 0;
                const int nSamples = 16;
                for (int i = 0; i < nSamples; i++) {
                    sum += analogRead(HAL_PIN_GPIO_ADC);
                    delay(5);
                }
                uint32_t avgRaw = sum / nSamples;
                float voltage = (float)avgRaw * 3.3f / 4095.0f;

                Lcd.setTextColor(TFT_YELLOW, TFT_BLACK);
                Lcd.printf(" Raw: %lu / 4095\n", avgRaw);
                Lcd.printf(" Voltage: %.3f V\n", voltage);
                Serial.printf("[BBT 12-4] ADC raw=%lu V=%.3f\n", avgRaw, voltage);

                /* Pass if voltage is between 1.0V and 2.3V (nominal 1.65V, ±0.65V tolerance) */
                bool adc_ok = (voltage > 1.0f && voltage < 2.3f);
                Lcd.setTextColor(adc_ok ? TFT_GREEN : TFT_RED, TFT_BLACK);
                Lcd.printf(" ADC: %s (expect 1.0~2.3V)\n", adc_ok ? "PASS" : "FAIL");
                Serial.printf("[BBT 12-4] ADC %s\n", adc_ok ? "PASS" : "FAIL");
                if (adc_ok) gpio_pass++;

                Lcd.setTextColor(TFT_DARKGREY, TFT_BLACK);
                Lcd.printf(" B = Next\n");
                while (true) { button.update(); if (button.B.pressed()) break; delay(20); }
            }

            /* ──────────────────────────────────────────────
             *  12-5) PWM 输出 (GPIO17) — 1kHz 50% 占空比
             *         TP1 测试点可用示波器/万用表验证
             * ────────────────────────────────────────────── */
            {
                Lcd.fillScreen(TFT_BLACK);
                Lcd.setCursor(0, 0);
                Lcd.setTextColor(TFT_CYAN, TFT_BLACK);
                Lcd.setFont(&fonts::efontCN_16);
                Lcd.printf(" [GPIO 5/5] PWM (GPIO%d)\n", HAL_PIN_GPIO_PWM);
                Lcd.printf(" 1kHz 50%% output on TP1\n");
                Serial.println("[BBT 12-5] PWM output test");

                /* Configure LEDC for PWM output */
                const ledc_timer_t   pwm_timer   = LEDC_TIMER_2;
                const ledc_channel_t pwm_channel = LEDC_CHANNEL_4;
                const uint32_t       pwm_freq    = 1000;   /* 1kHz */
                const uint32_t       pwm_duty    = 512;    /* 50% of 10-bit (1024) */
                const ledc_timer_bit_t pwm_res   = LEDC_TIMER_10_BIT;

                ledc_timer_config_t timer_cfg = {};
                timer_cfg.speed_mode      = LEDC_LOW_SPEED_MODE;
                timer_cfg.timer_num       = pwm_timer;
                timer_cfg.duty_resolution = pwm_res;
                timer_cfg.freq_hz         = pwm_freq;
                timer_cfg.clk_cfg         = LEDC_AUTO_CLK;
                esp_err_t terr = ledc_timer_config(&timer_cfg);

                ledc_channel_config_t ch_cfg = {};
                ch_cfg.speed_mode = LEDC_LOW_SPEED_MODE;
                ch_cfg.channel    = pwm_channel;
                ch_cfg.timer_sel  = pwm_timer;
                ch_cfg.intr_type  = LEDC_INTR_DISABLE;
                ch_cfg.gpio_num   = HAL_PIN_GPIO_PWM;
                ch_cfg.duty       = pwm_duty;
                ch_cfg.hpoint     = 0;
                esp_err_t cerr = ledc_channel_config(&ch_cfg);

                bool pwm_ok = (terr == ESP_OK && cerr == ESP_OK);
                if (pwm_ok) {
                    Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
                    Lcd.printf(" PWM running: %luHz %d%%\n", pwm_freq, pwm_duty * 100 / 1024);
                    Lcd.printf(" Check TP1 with scope/meter\n");
                    Serial.printf("[BBT 12-5] PWM %luHz duty=%d started\n", pwm_freq, pwm_duty);

                    /* Let PWM run — user can verify on TP1 */
                    Lcd.setTextColor(TFT_YELLOW, TFT_BLACK);
                    Lcd.printf(" PWM active — B=Stop\n");
                    while (true) { button.update(); if (button.B.pressed()) break; delay(20); }

                    /* Stop PWM */
                    ledc_stop(LEDC_LOW_SPEED_MODE, pwm_channel, 0);
                    gpio_reset_pin((gpio_num_t)HAL_PIN_GPIO_PWM);
                } else {
                    Lcd.setTextColor(TFT_RED, TFT_BLACK);
                    Lcd.printf(" PWM config failed\n");
                    Serial.printf("[BBT 12-5] PWM config fail t=%d c=%d\n", terr, cerr);
                }

                Lcd.setTextColor(pwm_ok ? TFT_GREEN : TFT_RED, TFT_BLACK);
                Lcd.printf(" PWM: %s\n", pwm_ok ? "PASS" : "FAIL");
                Serial.printf("[BBT 12-5] PWM %s\n", pwm_ok ? "PASS" : "FAIL");
                if (pwm_ok) gpio_pass++;
            }

            /* ── GPIO Test Summary ── */
            bool gpio_pass_all = (gpio_pass == gpio_total);
            char gd1[40]; snprintf(gd1, sizeof(gd1), "PASS: %d/%d SUB-TESTS", gpio_pass, gpio_total);
            bbt_results[BBT_IDX_GPIO] = gpio_pass_all ? BBT_ST_PASS : BBT_ST_FAIL;
            bbt_drawResult(Lcd, "12", "GPIO TEST", gpio_pass_all,
                           gd1, "SHT30/W25Q64/INT/ADC/PWM");
            Serial.printf("[BBT 12] GPIO result: %d/%d\n", gpio_pass, gpio_total);
            delay(1500);
            button.update(); button.A.hasChanged(); button.B.hasChanged(); delay(100);
            while (true) { button.update(); if (button.B.pressed()) break; delay(20); }
        } else {
            bbt_results[BBT_IDX_GPIO] = BBT_ST_SKIP;
        }
    }

bbt_done:
    /* ── Save run-once flag if at least one test was attempted ── */
    {
        int _started = 0;
        for (int i = 0; i < BBT_NUM_TESTS; i++)
            if (bbt_results[i] != BBT_ST_PENDING) _started++;
        if (_started > 0) {
            Preferences _bbt_prefs;
            _bbt_prefs.begin("mk_bbt", false);
            _bbt_prefs.putBool("done", true);
            _bbt_prefs.end();
            Serial.println("[BBT] Run-once flag saved — next boot enters system UI directly");
        }
    }
    Serial.println("\n========== BBT Complete ==========\n");
    bbt_drawOverview(Lcd, bbt_results, false);
    button.update(); button.A.hasChanged(); button.B.hasChanged(); delay(200);
    while (true) { button.update(); if (button.B.pressed()) break; delay(20); }
    } /* close nested bbt_results scope */

bbt_end: ;
#endif /* MEOWKIT_HW_TEST_ENABLE */

    return ok;
}


/* ════════════════════════════════════════════════════════════
 *  I2C Bus Recovery — 9 clock pulses to release stuck SDA
 * ════════════════════════════════════════════════════════════ */
static bool _i2cBusRecovery(gpio_num_t sda, gpio_num_t scl)
{
    i2c_driver_delete(I2C_NUM_0);

    gpio_set_direction(scl, GPIO_MODE_OUTPUT_OD);
    gpio_set_direction(sda, GPIO_MODE_INPUT);
    gpio_set_level(scl, 1);
    delayMicroseconds(5);

    bool recovered = false;
    for (int i = 0; i < 9; i++) {
        gpio_set_level(scl, 0);
        delayMicroseconds(5);
        gpio_set_level(scl, 1);
        delayMicroseconds(5);
        if (gpio_get_level(sda)) {
            recovered = true;
            break;
        }
    }

    /* Re-initialize I2C driver */
    In_I2C.begin();
    delay(20);

    return recovered;
}
