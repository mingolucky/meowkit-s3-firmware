/**
 * @file  config.h
 * @brief MeowKit BSP — Pin Map & Board Constants
 *
 * MCU : ESP32-S3-WROOM-1-N16R8  (Dual-core LX7 240 MHz, 16 MB Flash, 8 MB PSRAM)
 * Board: MeowKit v1.0
 *
 * Naming convention:  HAL_PIN_<PERIPHERAL>_<SIGNAL>
 * All GPIO numbers refer to ESP32-S3 absolute GPIO index.
 */
#pragma once

/* ═══════════════════════════════════════════════════════════════
 *  Project Meta
 * ═══════════════════════════════════════════════════════════════ */
#define BSP_VERSION                 "v1.0"
#define PROJECT_NAME                "MeowKit"
#define FIRMWARE_VERSION            "v1.0.1"

/* ═══════════════════════════════════════════════════════════════
 *  Hardware Board Test (BBT) — set 1 to enable, 0 to disable
 * ═══════════════════════════════════════════════════════════════ */
#define MEOWKIT_HW_TEST_ENABLE      0

/* ═══════════════════════════════════════════════════════════════
 *  Display — ST7789 IPS 320×240, SPI
 * ═══════════════════════════════════════════════════════════════ */
#define HAL_PIN_LCD_MOSI            40
#define HAL_PIN_LCD_MISO            (-1)
#define HAL_PIN_LCD_SCLK            41
#define HAL_PIN_LCD_DC              39
#define HAL_PIN_LCD_CS              (-1)      /* via PCA9557 IO1 */
#define HAL_PIN_LCD_RST             (-1)      /* via PCA9557 IO0 */
#define HAL_PIN_LCD_BUSY            (-1)
#define HAL_PIN_LCD_BL              42        /* PWM backlight   */

/* ═══════════════════════════════════════════════════════════════
 *  Buttons — membrane tactile A / B
 * ═══════════════════════════════════════════════════════════════ */
#define HAL_PIN_BTN_A               6
#define HAL_PIN_BTN_B               4

/* ═══════════════════════════════════════════════════════════════
 *  Joystick — 5-way directional (Up/Down/Left/Right/Press)
 * ═══════════════════════════════════════════════════════════════ */
#define HAL_PIN_JOY_UP              12
#define HAL_PIN_JOY_DOWN            18
#define HAL_PIN_JOY_LEFT            17
#define HAL_PIN_JOY_RIGHT           8

/* ═══════════════════════════════════════════════════════════════
 *  SD Card — SDMMC 1-bit mode (Lexar 633x 32 GB)
 * ═══════════════════════════════════════════════════════════════ */
#define HAL_PIN_SD_CLK              47
#define HAL_PIN_SD_CMD              48
#define HAL_PIN_SD_D0               21

/* ═══════════════════════════════════════════════════════════════
 *  I2C — Shared bus (AXP173/PCA9557/FT6336/ES8311/ES7210/PCF8563/BMI270)
 * ═══════════════════════════════════════════════════════════════ */
#define HAL_PIN_I2C_SCL             2
#define HAL_PIN_I2C_SDA             1
#define HAL_I2C_FREQ                100000    /* 100 kHz */

/* ═══════════════════════════════════════════════════════════════
 *  I2S — Audio bus (ES8311 DAC + ES7210 ADC, shared BCLK/WS)
 * ═══════════════════════════════════════════════════════════════ */
#define HAL_PIN_I2S_MCLK            15        /* Master clock      */
#define HAL_PIN_I2S_BCLK            14        /* Bit clock         */
#define HAL_PIN_I2S_WS              13        /* Word select / LRCK */
#define HAL_PIN_I2S_DOUT            16        /* Data out → ES8311 */
#define HAL_PIN_I2S_DIN             3         /* Data in  ← ES7210 */

/* ═══════════════════════════════════════════════════════════════
 *  IR — 940 nm TX / 950 nm RX  (38 kHz carrier)
 * ═══════════════════════════════════════════════════════════════ */
#define HAL_PIN_IR_TX               7
#define HAL_PIN_IR_RX               5

/* ═══════════════════════════════════════════════════════════════
 *  LED — WS2812B-2020 (single pixel, data pin)
 * ═══════════════════════════════════════════════════════════════ */
#define HAL_PIN_LED_WS2812          38

/* ═══════════════════════════════════════════════════════════════
 *  PMU IRQ — AXP173 interrupt output (active low)
 * ═══════════════════════════════════════════════════════════════ */
#define HAL_PIN_PMU_IRQ             43

/* ═══════════════════════════════════════════════════════════════
 *  USB — ESP32-S3 native (fixed pins, not remappable)
 * ═══════════════════════════════════════════════════════════════ */
#define HAL_PIN_USB_DP              20
#define HAL_PIN_USB_DN              19

/* ═══════════════════════════════════════════════════════════════
 *  I2C Device Addresses
 * ═══════════════════════════════════════════════════════════════ */
#define HAL_I2C_ADDR_ES8311         0x18
#define HAL_I2C_ADDR_PCA9557        0x19
#define HAL_I2C_ADDR_AXP173         0x34
#define HAL_I2C_ADDR_FT6336         0x38
#define HAL_I2C_ADDR_ES7210         0x41
#define HAL_I2C_ADDR_PCF8563        0x51
#define HAL_I2C_ADDR_BMI270         0x68
#define HAL_I2C_ADDR_SHT30          0x44

/* ═══════════════════════════════════════════════════════════════
 *  PCA9557 IO Expander Pin Assignments (directly on expander)
 * ═══════════════════════════════════════════════════════════════ */
#define HAL_IOEXP_LCD_RST           0         /* PCA9557 IO0 */
#define HAL_IOEXP_LCD_CS            1         /* PCA9557 IO1 */
#define HAL_IOEXP_FLASH_CS          2         /* PCA9557 IO2 — W25Q64 SPI Flash CS */
#define HAL_IOEXP_PA_EN             3         /* PCA9557 IO3 — NS4150B CTRL (speaker/headphone amp enable, active HIGH, 10K pull-down) */
#define HAL_IOEXP_TOUCH_RST         6         /* PCA9557 IO6 */

/* ═══════════════════════════════════════════════════════════════
 *  SPI Flash — W25Q64JVSSIQ (external)
 * ═══════════════════════════════════════════════════════════════ */
#define HAL_PIN_FLASH_MOSI          11        /* DO — data out to flash   */
#define HAL_PIN_FLASH_MISO          10        /* DI — data in from flash  */
#define HAL_PIN_FLASH_SCK           9         /* SCK                      */

/* ═══════════════════════════════════════════════════════════════
 *  GPIO Test Pins (BBT 12 — reuse joystick GPIOs for alt-func)
 * ═══════════════════════════════════════════════════════════════ */
#define HAL_PIN_GPIO_INT            8         /* Interrupt input (rising edge) */
#define HAL_PIN_GPIO_ADC            18        /* ADC input                     */
#define HAL_PIN_GPIO_PWM            17        /* PWM output                    */

/* ═══════════════════════════════════════════════════════════════
 *  Backward-compatible aliases (消除对旧宏名的依赖后可移除)
 * ═══════════════════════════════════════════════════════════════ */
#define HAL_A                       HAL_PIN_BTN_A
#define HAL_B                       HAL_PIN_BTN_B
#define HAL_JOYSTICK_UP             HAL_PIN_JOY_UP
#define HAL_JOYSTICK_DOWN           HAL_PIN_JOY_DOWN
#define HAL_JOYSTICK_LEFT           HAL_PIN_JOY_LEFT
#define HAL_JOYSTICK_RIGHT          HAL_PIN_JOY_RIGHT
#define SDMMC_SD_PIN_CLK            HAL_PIN_SD_CLK
#define SDMMC_SD_PIN_CMD            HAL_PIN_SD_CMD
#define SDMMC_SD_PIN_D0             HAL_PIN_SD_D0
#define I2CSPEED                    HAL_I2C_FREQ
#define MCLKPIN                     HAL_PIN_I2S_MCLK
#define BCLKPIN                     HAL_PIN_I2S_BCLK
#define WSPIN                       HAL_PIN_I2S_WS
#define DOPIN                       HAL_PIN_I2S_DOUT
#define DIPIN                       HAL_PIN_I2S_DIN
#define HAL_PIN_WS2812_LED          HAL_PIN_LED_WS2812
#define HAL_IRQ_PIN                 HAL_PIN_PMU_IRQ
#define BSP_VERISON                 BSP_VERSION   /* typo compat */
