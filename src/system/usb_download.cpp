#include "usb_download.h"
#include <esp_attr.h>
#include <esp_system.h>
#include <esp_rom_sys.h>
#include <esp32s3/rom/rtc.h>
#include <driver/periph_ctrl.h>
#include <soc/rtc_cntl_reg.h>
#include <soc/usb_serial_jtag_reg.h>

namespace {
constexpr uint32_t DownloadMagic = 0x4D4B444C;
struct DownloadCookie { uint32_t magic, inverse; };
RTC_NOINIT_ATTR volatile DownloadCookie cookie;

// Run during global construction, before app_main()/USB.begin(). In this
// second stage there is no TinyUSB task/ISR to race with the PHY handoff.
__attribute__((constructor(101))) void enter_download_before_usb() {
    const auto reason = rtc_get_reset_reason(0);
    const bool requested = cookie.magic == DownloadMagic && cookie.inverse == ~DownloadMagic;
    cookie.magic = cookie.inverse = 0;
    if (!requested || (reason != RTC_SW_CPU_RESET && reason != RTC_SW_SYS_RESET)) return;

    periph_module_reset(PERIPH_USB_MODULE);
    periph_module_disable(PERIPH_USB_MODULE);
    REG_CLR_BIT(USB_SERIAL_JTAG_CONF0_REG, USB_SERIAL_JTAG_USB_PAD_ENABLE);
    REG_CLR_BIT(RTC_CNTL_USB_CONF_REG,
        RTC_CNTL_SW_HW_USB_PHY_SEL | RTC_CNTL_SW_USB_PHY_SEL | RTC_CNTL_USB_PAD_ENABLE |
        RTC_CNTL_USB_RESET_DISABLE | RTC_CNTL_IO_MUX_RESET_DISABLE);
    REG_CLR_BIT(USB_SERIAL_JTAG_CONF0_REG, USB_SERIAL_JTAG_PHY_SEL);
    esp_rom_delay_us(20000); // detached long enough for the host to see removal
    REG_SET_BIT(USB_SERIAL_JTAG_CONF0_REG, USB_SERIAL_JTAG_USB_PAD_ENABLE);
    REG_SET_BIT(RTC_CNTL_OPTION1_REG, RTC_CNTL_FORCE_DOWNLOAD_BOOT);
    // ROM digital reset is appropriate here: the application and RTOS have
    // not started. This is deliberately not the runtime TinyUSB restart path.
    software_reset();
    for (;;) {}
}
}

void usb_download_restart(void) {
    cookie.magic = DownloadMagic;
    cookie.inverse = ~DownloadMagic;
    esp_restart();
}
