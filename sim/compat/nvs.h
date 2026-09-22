/* Shim de bureau : sous-ensemble de nvs.h utilisé par src/ui.
 * L'UI n'appelle que nvs_get_stats() pour afficher le taux d'occupation
 * du stockage de configuration interne. */
#pragma once
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef int esp_err_t;
#ifndef ESP_OK
#define ESP_OK 0
#endif

typedef struct {
    size_t used_entries;
    size_t free_entries;
    size_t total_entries;
    size_t namespace_count;
} nvs_stats_t;

esp_err_t nvs_get_stats(const char * part_name, nvs_stats_t * stats);

#ifdef __cplusplus
}
#endif
