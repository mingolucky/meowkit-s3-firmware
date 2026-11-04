Source: https://github.com/stm32duino/ST25R3916
Revision and original per-file Git blob IDs: UPSTREAM.json.
Original ST copyright notices and LICENSE are retained.

Local MeowKit adaptations:
- MeowNfcBus replaces TwoWire with a bounded callback interface. The application
  supplies the existing ESP-IDF I2C driver and shared bus mutex.
- Preserve complete FIFO read lengths (including 256 and 512), propagate I/O
  errors, validate pointers/lengths, support RFAL's NULL-buffer FIFO drain,
  initialize receive buffers, enlarge the
  upstream SPI FIFO temporary to the full FIFO depth.
- int_pin=-1 disables all IRQ GPIO setup/access. Poll all four read-to-clear IRQ
  registers, retain unrelated events in software, stop on bus errors, bound waits.
- I2C IRQ collection follows M5Stack M5Unit-NFC readInterrupts(): read Error/Wakeup
  before Main/Timer, then Target; retain successfully read flags on partial I/O
  failure. Reference (MIT): https://github.com/m5stack/M5Unit-NFC/blob/471a6e02e8052264d39b3ac97005ce4de9d1cb91/src/unit/unit_ST25R3916.cpp
  SPI retains the original four-register burst. ClearInterrupts intentionally
  discards every event and continues to use a burst.
- Route bus failures/cancellation through RFAL transceive failure cleanup so a
  blocking exchange does not keep waiting for an IRQ from a disconnected chip.
- Select original ST25R3916 and RFAL_USE_I2C. Disable AAT in the analog table for
  the fixed matching network whose AAT pins are not connected.
- Yield during millisecond delays. Avoid a duplicate rfalRunBlocking macro and
  remove pointer truncation in analog table arithmetic for host-side tests.
- Added PlatformIO metadata. Source license remains ST MIX MYLIBERTY, see LICENSE.

Radio behavior and analog configuration require validation on the actual PCB.
