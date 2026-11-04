Source: https://github.com/stm32duino/NFC-RFAL
Revision and original per-file Git blob IDs: UPSTREAM.json.
Original ST copyright notices and LICENSE are retained.

Local MeowKit adaptations:
- Added rfal_custom_config.h for poller-only A/B/F/V and ISO-DEP polling;
  disable listen mode, ISO-DEP listen and NFC-DEP/active P2P.
- No custom automatic-antenna-tuning profile is selected.
- Rename the BR identifier to initiatorBitRates to avoid the Xtensa register
  macro named BR. Added PlatformIO metadata.

All other RFAL/NDEF implementations remain upstream code. The original ST MIX
MYLIBERTY license applies; this package has not been relicensed as GPL.
