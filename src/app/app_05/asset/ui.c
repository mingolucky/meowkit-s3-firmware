// Minimal UI globals for SquareLine/LVGL
#include "ui.h"

lv_obj_t * ui_PC_Monitor = NULL;
lv_obj_t * ui_pc_monitor_bg = NULL;
lv_obj_t * ui_temp1 = NULL;
lv_obj_t * ui_temp2 = NULL;
lv_obj_t * ui_cpu_name = NULL;
lv_obj_t * ui_cpu_temp = NULL;
lv_obj_t * ui_cpu_percent = NULL;
lv_obj_t * ui_symbol_1 = NULL;
lv_obj_t * ui_symbol_2 = NULL;
lv_obj_t * ui_gpu_name = NULL;
lv_obj_t * ui_temp3 = NULL;
lv_obj_t * ui_temp4 = NULL;
lv_obj_t * ui_gpu_temp = NULL;
lv_obj_t * ui_gpu_percent = NULL;
lv_obj_t * ui_symbol_3 = NULL;
lv_obj_t * ui_symbol_4 = NULL;
lv_obj_t * ui_Image5 = NULL;
lv_obj_t * ui_RAM = NULL;
lv_obj_t * ui_gpu_ram = NULL;
lv_obj_t * ui_mhz = NULL;
lv_obj_t * ui_mem_symbol = NULL;

void ui_pc_monitor_init(void) {
	ui_PC_Monitor_screen_init();
	lv_scr_load(ui_PC_Monitor);
}

