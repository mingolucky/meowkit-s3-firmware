// Auto-generated/hand-edited header for SquareLine/LVGL UI
#ifndef APP05_ASSET_UI_PC_MONITOR_H
#define APP05_ASSET_UI_PC_MONITOR_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__has_include)
#  if __has_include("lvgl.h")
#    include "lvgl.h"
#  else
	// Fallback for editors/indexers when lvgl.h include path isn't resolved
	typedef struct _lv_obj_t lv_obj_t;
	typedef struct _lv_font_t lv_font_t;
	typedef struct _lv_img_dsc_t lv_img_dsc_t;
	#ifndef LV_FONT_DECLARE
	#define LV_FONT_DECLARE(name) extern const lv_font_t name;
	#endif
	#ifndef LV_IMG_DECLARE
	#define LV_IMG_DECLARE(name) extern const lv_img_dsc_t name;
	#endif
#  endif
#else
#  include "lvgl.h"
#endif

// Fonts
LV_FONT_DECLARE(ui_font_pc_temp);
LV_FONT_DECLARE(ui_font_name_14);
// Images
LV_IMG_DECLARE(ui_img_pc_monitor_bg_png);    // assets/pc_monitor_bg.png
LV_IMG_DECLARE(ui_img_temp1_full_png);    // assets/temp1_full.png
LV_IMG_DECLARE(ui_img_temp2_full_png);    // assets/temp2_full.png

// SCREEN: ui_PC_Monitor
void ui_PC_Monitor_screen_init(void);
/* Pilote les quatre jauges horizontales. Températures en °C, charges en %. */
void ui_pc_monitor_set_gauges(int cpu_temp_c, int cpu_load_pct,
                              int gpu_temp_c, int gpu_load_pct);
/* Barre verticale du bloc « USED MEMORY ». Occupation en %. */
void ui_pc_monitor_set_memory_gauge(int used_pct);
void ui_pc_monitor_init(void); // optional helper to init and load screen

// Widgets
extern lv_obj_t * ui_PC_Monitor;
extern lv_obj_t * ui_pc_monitor_bg;
extern lv_obj_t * ui_temp1;
extern lv_obj_t * ui_temp2;
extern lv_obj_t * ui_cpu_name;
extern lv_obj_t * ui_cpu_temp;
extern lv_obj_t * ui_cpu_percent;
extern lv_obj_t * ui_symbol_1;
extern lv_obj_t * ui_symbol_2;
extern lv_obj_t * ui_gpu_name;
extern lv_obj_t * ui_temp3;
extern lv_obj_t * ui_temp4;
extern lv_obj_t * ui_gpu_temp;
extern lv_obj_t * ui_gpu_percent;
extern lv_obj_t * ui_symbol_3;
extern lv_obj_t * ui_symbol_4;
extern lv_obj_t * ui_Image5;
extern lv_obj_t * ui_RAM;
extern lv_obj_t * ui_gpu_ram;
extern lv_obj_t * ui_mhz;
extern lv_obj_t * ui_mem_symbol;

#ifdef __cplusplus
}
#endif

#endif // APP05_ASSET_UI_PC_MONITOR_H
