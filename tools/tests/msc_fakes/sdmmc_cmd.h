#pragma once
#include "driver/sdmmc_host.h"
struct sdmmc_card_t {struct {uint32_t capacity,sector_size;} csd;};
esp_err_t sdmmc_card_init(const sdmmc_host_t*,sdmmc_card_t*);
esp_err_t sdmmc_read_sectors(sdmmc_card_t*,void*,size_t,size_t);
esp_err_t sdmmc_write_sectors(sdmmc_card_t*,const void*,size_t,size_t);
