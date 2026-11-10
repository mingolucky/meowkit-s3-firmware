/* Shim minimal pour le simulateur de bureau.
 *
 * lv_conf.h du firmware demande LV_TICK_CUSTOM_INCLUDE "Arduino.h" et
 * LV_TICK_CUSTOM_SYS_TIME_EXPR (millis()). Plutôt que de modifier lv_conf.h
 * — ce qui polluerait le firmware avec du code de simulation — on fournit
 * simplement un « Arduino.h » de bureau qui n'expose que millis().
 */
#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
uint32_t millis(void);
void     delay(uint32_t ms);
#ifdef __cplusplus
}
#endif
