#pragma once
#include <cstddef>
#include <cstdint>
typedef int esp_err_t;constexpr int ESP_OK=0;
struct sdmmc_host_t {int max_freq_khz=0,slot=0,flags=0;};
struct sdmmc_slot_config_t {int clk=0,cmd=0,d0=0,width=0,flags=0;};
typedef int gpio_num_t;
#define SDMMC_HOST_DEFAULT() sdmmc_host_t{}
#define SDMMC_SLOT_CONFIG_DEFAULT() sdmmc_slot_config_t{}
#define SDMMC_HOST_SLOT_1 1
#define SDMMC_HOST_FLAG_1BIT 1
#define SDMMC_FREQ_DEFAULT 20000
esp_err_t sdmmc_host_init();esp_err_t sdmmc_host_init_slot(int,const sdmmc_slot_config_t*);esp_err_t sdmmc_host_deinit();

esp_err_t sdmmc_host_set_card_clk(int,uint32_t);
