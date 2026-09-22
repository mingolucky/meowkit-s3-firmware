/* Shim de bureau : sous-ensemble d'esp_system.h utilisé par src/ui.
 * Seul esp_restart() est appelé (écran « système » → redémarrage). */
#pragma once
#ifdef __cplusplus
extern "C" {
#endif
void esp_restart(void);
#ifdef __cplusplus
}
#endif
