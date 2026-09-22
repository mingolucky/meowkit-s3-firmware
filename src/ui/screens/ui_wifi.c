// Wi-Fi screen v2 — scan spinner, sorted list, lock icons, color-coded signal,
// long-press Forget menu, saved-connect failure → T9 pre-fill.
// LVGL 8.3.11 / ESP32-S3

#include <stdlib.h>
#include "../ui.h"
#include "../ui_wifi_bridge.h"

// ── Screen-level objects (extern in ui_wifi.h) ──
lv_obj_t * ui_wifi;
lv_obj_t * ui_wifi_bg;
lv_obj_t * ui_wifi_header_bg;
lv_obj_t * ui_wifi_list_border;
lv_obj_t * ui_wifi_key_prompts_bg;
lv_obj_t * ui_wifi_key_a_bg;
lv_obj_t * ui_wifi_key_b_bg;
lv_obj_t * ui_wifi_header_label;
lv_obj_t * ui_wifi_key_a_home;   // label text is now "Connect"
lv_obj_t * ui_wifi_key_b_back;
lv_obj_t * ui_wifi_scan_switch;

// ── File-scope state ──
static lv_obj_t   * ui_wifi_list  = NULL;
static lv_timer_t * s_scan_timer  = NULL;
static lv_timer_t * s_conn_timer  = NULL;
static lv_obj_t   * s_toast       = NULL;
static int          s_last_count  = -1;
static bool         s_scan_done   = false;
static bool         s_scan_active = false;  // true from start_scan() until complete/stopped
static bool         s_was_saved   = false;  // true while connecting to a saved network

// ── Focused row: physical A button connects this ──
static struct {
    char    ssid[64];
    uint8_t saved;
    uint8_t encrypted;
} s_focused = {{0}, 0, 0};

// ── Long-press menu ──
static lv_obj_t * s_menu      = NULL;
static char       s_menu_ssid[64] = "";

// ── Per-row heap data ──
typedef struct {
    char    ssid[64];
    uint8_t saved;
    uint8_t encrypted;
    int     pct;
} wifi_row_t;

// ── qsort key for AP list: connected > saved > signal ──
static const char * s_sort_cur_ssid = "";

static int _ap_cmp(const void * a, const void * b)
{
    const wifi_row_t * ra = (const wifi_row_t*)a;
    const wifi_row_t * rb = (const wifi_row_t*)b;
    int sa = (strcmp(ra->ssid, s_sort_cur_ssid) == 0) ? 10000 :
             (ra->saved ? 1000 + ra->pct : ra->pct);
    int sb = (strcmp(rb->ssid, s_sort_cur_ssid) == 0) ? 10000 :
             (rb->saved ? 1000 + rb->pct : rb->pct);
    return sb - sa;
}

// ── Forward declarations ──
static void _wifi_build_list(void);
static void _wifi_show_toast(const char * msg, lv_color_t color);
static void _wifi_add_rescan_row(void);
static void _wifi_start_scan(void);
static void _show_menu(const wifi_row_t * row);
static void _hide_menu(void);
static void _do_connect(const char * ssid, bool saved, bool encrypted);
static void _scan_timer_cb(lv_timer_t * t);
static void _conn_poll_cb(lv_timer_t * t);
static void _nav_to_t9_cb(lv_timer_t * t);

// ── Toast ──
static void _wifi_show_toast(const char * msg, lv_color_t color)
{
    if(s_toast) { lv_obj_del(s_toast); s_toast = NULL; }
    if(!ui_wifi) return;
    s_toast = lv_obj_create(ui_wifi);
    lv_obj_set_height(s_toast, 26);
    lv_obj_set_width(s_toast, LV_SIZE_CONTENT);
    lv_obj_set_pos(s_toast, 8, 207);
    lv_obj_set_style_bg_color(s_toast, lv_color_hex(0x1C1C1E), 0);
    lv_obj_set_style_bg_opa(s_toast, 220, 0);
    lv_obj_set_style_radius(s_toast, 6, 0);
    lv_obj_set_style_border_width(s_toast, 0, 0);
    lv_obj_set_style_pad_hor(s_toast, 8, 0);
    lv_obj_set_style_pad_ver(s_toast, 4, 0);
    lv_obj_clear_flag(s_toast, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_t * lbl = lv_label_create(s_toast);
    lv_label_set_text(lbl, msg);
    lv_obj_set_style_text_color(lbl, color, 0);
    lv_obj_set_style_text_font(lbl, &ui_font_name_14, 0);
    lv_obj_move_foreground(s_toast);
}

// ── Start / restart AP scan (safe to call repeatedly) ──
static void _wifi_start_scan(void)
{
    if(s_scan_active) return;
    s_scan_done   = false;
    s_scan_active = true;   /* set before lv_obj_add_state to guard re-entry */
    s_last_count  = -1;
    if(ui_wifi_scan_switch) lv_obj_add_state(ui_wifi_scan_switch, LV_STATE_CHECKED);
    ui_wifi_bridge_start_scan();
    if(!s_scan_timer)
        s_scan_timer = lv_timer_create(_scan_timer_cb, 1000, NULL);
}

// ── Row lifecycle ──
static void _row_delete_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) == LV_EVENT_DELETE)
        free(lv_event_get_user_data(e));
}

// ── Long-press Forget menu ──
static void _forget_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    ui_wifi_bridge_forget(s_menu_ssid);
    _hide_menu();
    s_focused.ssid[0] = '\0';
    _wifi_build_list();
}

static void _menu_cancel_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    _hide_menu();
}

static void _hide_menu(void)
{
    if(s_menu) { lv_obj_del(s_menu); s_menu = NULL; }
}

static void _show_menu(const wifi_row_t * row)
{
    _hide_menu();
    strncpy(s_menu_ssid, row->ssid, sizeof(s_menu_ssid) - 1);
    s_menu_ssid[sizeof(s_menu_ssid) - 1] = '\0';
    if(!ui_wifi) return;

    // Semi-transparent backdrop (blocks touch to list below)
    s_menu = lv_obj_create(ui_wifi);
    lv_obj_set_size(s_menu, 320, 240);
    lv_obj_set_pos(s_menu, 0, 0);
    lv_obj_set_style_bg_color(s_menu, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(s_menu, 130, 0);
    lv_obj_set_style_border_width(s_menu, 0, 0);
    lv_obj_set_style_radius(s_menu, 0, 0);
    lv_obj_set_style_pad_all(s_menu, 0, 0);
    lv_obj_add_flag(s_menu, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(s_menu, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(s_menu, _menu_cancel_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_move_foreground(s_menu);

    // Menu panel
    lv_obj_t * panel = lv_obj_create(s_menu);
    lv_obj_set_size(panel, 200, 106);
    lv_obj_align(panel, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(panel, lv_color_hex(0x1C1C1E), 0);
    lv_obj_set_style_bg_opa(panel, 255, 0);
    lv_obj_set_style_radius(panel, 10, 0);
    lv_obj_set_style_border_color(panel, lv_color_hex(0x444444), 0);
    lv_obj_set_style_border_width(panel, 1, 0);
    lv_obj_set_style_pad_all(panel, 8, 0);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

    // SSID title
    lv_obj_t * title = lv_label_create(panel);
    lv_label_set_long_mode(title, LV_LABEL_LONG_DOT);
    lv_obj_set_width(title, 180);
    lv_label_set_text(title, row->ssid);
    lv_obj_set_style_text_color(title, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(title, &ui_font_name_14, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);

    // Forget button
    lv_obj_t * fbtn = lv_btn_create(panel);
    lv_obj_set_size(fbtn, 180, 30);
    lv_obj_align(fbtn, LV_ALIGN_TOP_MID, 0, 24);
    lv_obj_set_style_bg_color(fbtn, lv_color_hex(0xFF3B30), 0);
    lv_obj_set_style_radius(fbtn, 6, 0);
    lv_obj_set_style_pad_all(fbtn, 0, 0);
    lv_obj_add_event_cb(fbtn, _forget_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * flbl = lv_label_create(fbtn);
    lv_label_set_text(flbl, "Forget Network");
    lv_obj_set_style_text_color(flbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(flbl, &ui_font_name_14, 0);
    lv_obj_align(flbl, LV_ALIGN_CENTER, 0, 0);

    // Cancel button
    lv_obj_t * cbtn = lv_btn_create(panel);
    lv_obj_set_size(cbtn, 180, 30);
    lv_obj_align(cbtn, LV_ALIGN_TOP_MID, 0, 62);
    lv_obj_set_style_bg_color(cbtn, lv_color_hex(0x3A3A3C), 0);
    lv_obj_set_style_radius(cbtn, 6, 0);
    lv_obj_set_style_pad_all(cbtn, 0, 0);
    lv_obj_add_event_cb(cbtn, _menu_cancel_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * clbl = lv_label_create(cbtn);
    lv_label_set_text(clbl, "Cancel");
    lv_obj_set_style_text_color(clbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(clbl, &ui_font_name_14, 0);
    lv_obj_align(clbl, LV_ALIGN_CENTER, 0, 0);
}

// ── Row click / long-press ──
static void _row_click_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    wifi_row_t * row = (wifi_row_t*)lv_event_get_user_data(e);
    if(!row) return;

    strncpy(s_focused.ssid, row->ssid, sizeof(s_focused.ssid) - 1);
    s_focused.ssid[sizeof(s_focused.ssid) - 1] = '\0';
    s_focused.saved     = row->saved;
    s_focused.encrypted = row->encrypted;

    _do_connect(row->ssid, row->saved, row->encrypted);
}

static void _row_longpress_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_LONG_PRESSED) return;
    wifi_row_t * row = (wifi_row_t*)lv_event_get_user_data(e);
    if(!row) return;
    lv_indev_wait_release(lv_indev_get_act());
    _show_menu(row);
}

// ── Connect dispatcher (shared by click and physical A) ──
static void _do_connect(const char * ssid, bool saved, bool encrypted)
{
    if(!ssid || ssid[0] == '\0') return;

    if(saved) {
        ui_wifi_bridge_stop_scan();
        if(s_scan_timer) { lv_timer_del(s_scan_timer); s_scan_timer = NULL; }
        s_scan_active = false;
        s_was_saved = true;
        ui_wifi_bridge_connect_saved(ssid);
        char msg[80];
        lv_snprintf(msg, sizeof(msg), "Connecting: %s", ssid);
        _wifi_show_toast(msg, lv_color_hex(0xFFFFFF));
        if(s_conn_timer) { lv_timer_del(s_conn_timer); s_conn_timer = NULL; }
        s_conn_timer = lv_timer_create(_conn_poll_cb, 500, NULL);
    } else if(!encrypted) {
        ui_wifi_bridge_stop_scan();
        if(s_scan_timer) { lv_timer_del(s_scan_timer); s_scan_timer = NULL; }
        s_scan_active = false;
        s_was_saved = false;
        ui_wifi_bridge_connect(ssid, "");
        char msg[80];
        lv_snprintf(msg, sizeof(msg), "Connecting: %s (open)", ssid);
        _wifi_show_toast(msg, lv_color_hex(0xFFFFFF));
        if(s_conn_timer) { lv_timer_del(s_conn_timer); s_conn_timer = NULL; }
        s_conn_timer = lv_timer_create(_conn_poll_cb, 500, NULL);
    } else {
        // Encrypted network → T9 password entry
        ui_wifi_bridge_set_pending_ssid(ssid);
        ui_t9_keyboard_prepare_for_wifi(ssid);
        _ui_screen_change(&ui_t9_keyboard, LV_SCR_LOAD_ANIM_FADE_ON, 350, 0,
                          &ui_t9_keyboard_screen_init);
    }
}

// ── Rescan row ──
static void _rescan_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    _wifi_start_scan();
    _wifi_build_list();
}

static void _wifi_add_rescan_row(void)
{
    if(!ui_wifi_list) return;
    lv_obj_t * row = lv_obj_create(ui_wifi_list);
    lv_obj_set_size(row, lv_pct(100), 38);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 2, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    // Symbol with default font (LVGL symbols)
    lv_obj_t * sym = lv_label_create(row);
    lv_label_set_text(sym, LV_SYMBOL_REFRESH);
    lv_obj_set_style_text_color(sym, lv_color_hex(0x777777), 0);
    lv_obj_set_style_text_font(sym, LV_FONT_DEFAULT, 0);
    lv_obj_align(sym, LV_ALIGN_LEFT_MID, 6, 0);

    lv_obj_t * lbl = lv_label_create(row);
    lv_label_set_text(lbl, "Rescan");
    lv_obj_set_style_text_color(lbl, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(lbl, &ui_font_name_14, 0);
    lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 24, 0);

    lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(row, _rescan_cb, LV_EVENT_CLICKED, NULL);
}

// ── Build AP list ──
static void _wifi_build_list(void)
{
    if(!ui_wifi_list) return;
    lv_obj_clean(ui_wifi_list);
    if(s_toast) lv_obj_move_foreground(s_toast);

    // s_scan_active drives the spinner: true as soon as we call start_scan(),
    // before WiFi.scanComplete() necessarily transitions to -1.
    int  n        = ui_wifi_bridge_get_scan_count();
    s_last_count  = n;

    // Scanning spinner row — show whenever scan was triggered and not yet done
    if(s_scan_active && !s_scan_done) {
        lv_obj_t * row = lv_obj_create(ui_wifi_list);
        lv_obj_set_size(row, lv_pct(100), 38);
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_pad_all(row, 2, 0);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t * spin = lv_spinner_create(row, 800, 60);
        lv_obj_set_size(spin, 18, 18);
        lv_obj_align(spin, LV_ALIGN_LEFT_MID, 4, 0);
        lv_obj_set_style_arc_color(spin, lv_color_hex(0xC4F000), LV_PART_INDICATOR);
        lv_obj_set_style_arc_width(spin, 3, LV_PART_INDICATOR);
        lv_obj_set_style_arc_color(spin, lv_color_hex(0x444444), LV_PART_MAIN);
        lv_obj_set_style_arc_width(spin, 3, LV_PART_MAIN);

        lv_obj_t * lbl = lv_label_create(row);
        lv_label_set_text(lbl, "Scanning...");
        lv_obj_set_style_text_color(lbl, lv_color_hex(0x888888), 0);
        lv_obj_set_style_text_font(lbl, &ui_font_name_14, 0);
        lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 28, 0);

        if(n == 0) return;  // still scanning, no results to show yet
    }

    // "No networks found" only after scan actually completed with zero results
    if(n == 0 && s_scan_done) {
        lv_obj_t * row = lv_obj_create(ui_wifi_list);
        lv_obj_set_size(row, lv_pct(100), 38);
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_pad_all(row, 2, 0);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_t * lbl = lv_label_create(row);
        lv_label_set_text(lbl, "No networks found");
        lv_obj_set_style_text_color(lbl, lv_color_hex(0x888888), 0);
        lv_obj_set_style_text_font(lbl, &ui_font_name_14, 0);
        lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 4, 0);
        _wifi_add_rescan_row();
        return;
    }

    if(n == 0) {
        /* Switch is off or scan not yet started — show prompt */
        if(!s_scan_active) {
            lv_obj_t * row = lv_obj_create(ui_wifi_list);
            lv_obj_set_size(row, lv_pct(100), 38);
            lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(row, 0, 0);
            lv_obj_set_style_pad_all(row, 2, 0);
            lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

            lv_obj_t * sym = lv_label_create(row);
            lv_label_set_text(sym, LV_SYMBOL_WIFI);
            lv_obj_set_style_text_color(sym, lv_color_hex(0x555555), 0);
            lv_obj_set_style_text_font(sym, LV_FONT_DEFAULT, 0);
            lv_obj_align(sym, LV_ALIGN_LEFT_MID, 6, 0);

            lv_obj_t * lbl = lv_label_create(row);
            lv_label_set_text(lbl, "Toggle switch above to scan");
            lv_obj_set_style_text_color(lbl, lv_color_hex(0x555555), 0);
            lv_obj_set_style_text_font(lbl, &ui_font_name_14, 0);
            lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 24, 0);
        }
        return;
    }

    // Determine connected SSID
    wifi_bridge_status_t cur_st = ui_wifi_bridge_get_status();
    s_sort_cur_ssid = (cur_st == WIFI_BRIDGE_CONNECTED) ? ui_wifi_bridge_get_current_ssid() : "";

    // Collect into temp array for sorting
    wifi_row_t * aps = (wifi_row_t*)malloc((size_t)n * sizeof(wifi_row_t));
    if(!aps) return;

    for(int i = 0; i < n; i++) {
        bool sv = false;
        aps[i].pct = 0;
        aps[i].saved = 0;
        aps[i].encrypted = 0;
        if(!ui_wifi_bridge_get_ap(i, aps[i].ssid, (int)sizeof(aps[i].ssid), &aps[i].pct, &sv)) {
            aps[i].ssid[0] = '\0'; continue;
        }
        aps[i].saved     = sv ? 1u : 0u;
        aps[i].encrypted = ui_wifi_bridge_is_encrypted(i) ? 1u : 0u;
    }

    qsort(aps, (size_t)n, sizeof(wifi_row_t), _ap_cmp);

    // Build LVGL rows
    for(int i = 0; i < n; i++) {
        if(aps[i].ssid[0] == '\0') continue;

        wifi_row_t * data = (wifi_row_t*)malloc(sizeof(wifi_row_t));
        if(!data) continue;
        *data = aps[i];

        bool is_connected = (s_sort_cur_ssid[0] != '\0' &&
                             strcmp(aps[i].ssid, s_sort_cur_ssid) == 0);

        lv_obj_t * row = lv_obj_create(ui_wifi_list);
        lv_obj_set_size(row, lv_pct(100), 38);
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_radius(row, 4, 0);
        lv_obj_set_style_pad_all(row, 2, 0);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

        // Lock icon (default font has LVGL symbols)
        lv_obj_t * lock = lv_label_create(row);
        lv_label_set_text(lock, aps[i].encrypted ? LV_SYMBOL_EYE_CLOSE : " ");
        lv_obj_set_style_text_color(lock, lv_color_hex(0x666666), 0);
        lv_obj_set_style_text_font(lock, LV_FONT_DEFAULT, 0);
        lv_obj_align(lock, LV_ALIGN_LEFT_MID, 2, 0);

        // Connected tick
        int ssid_x = 18;
        if(is_connected) {
            lv_obj_t * tick = lv_label_create(row);
            lv_label_set_text(tick, LV_SYMBOL_OK);
            lv_obj_set_style_text_color(tick, lv_color_hex(0xC4F000), 0);
            lv_obj_set_style_text_font(tick, LV_FONT_DEFAULT, 0);
            lv_obj_align(tick, LV_ALIGN_LEFT_MID, 18, 0);
            ssid_x = 32;
        }

        // SSID label
        lv_obj_t * ssid_lbl = lv_label_create(row);
        lv_label_set_long_mode(ssid_lbl, LV_LABEL_LONG_DOT);
        lv_obj_set_width(ssid_lbl, 150);
        /* LV_LABEL_LONG_DOT only ellipsises when the height is bounded too.
         * Left at LV_SIZE_CONTENT the label grows instead, wrapping a long
         * SSID onto a second line that the 38 px row then clips. Pin it to a
         * single line so the text is truncated horizontally as intended. */
        lv_obj_set_height(ssid_lbl, lv_font_get_line_height(&ui_font_name_14));
        lv_label_set_text(ssid_lbl, aps[i].ssid);
        lv_obj_set_style_text_color(ssid_lbl, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(ssid_lbl, &ui_font_name_14, 0);
        lv_obj_align(ssid_lbl, LV_ALIGN_LEFT_MID, ssid_x, 0);

        // Signal color: ≥70% green, 40–69% white, <40% gray
        lv_color_t sig_color;
        if(aps[i].pct >= 70)      sig_color = lv_color_hex(0xC4F000);
        else if(aps[i].pct >= 40) sig_color = lv_color_hex(0xFFFFFF);
        else                      sig_color = lv_color_hex(0x888888);

        // Right label: pct + status tag
        char right_buf[20];
        if(is_connected) {
            lv_snprintf(right_buf, sizeof(right_buf), "%d%%", aps[i].pct);
        } else if(aps[i].saved) {
            lv_snprintf(right_buf, sizeof(right_buf), "%d%% Saved", aps[i].pct);
            sig_color = lv_color_hex(0x80C800);
        } else if(!aps[i].encrypted) {
            lv_snprintf(right_buf, sizeof(right_buf), "%d%% Open", aps[i].pct);
        } else {
            lv_snprintf(right_buf, sizeof(right_buf), "%d%%", aps[i].pct);
        }

        lv_obj_t * right_lbl = lv_label_create(row);
        lv_label_set_text(right_lbl, right_buf);
        lv_obj_set_style_text_color(right_lbl, sig_color, 0);
        lv_obj_set_style_text_font(right_lbl, &ui_font_name_14, 0);
        lv_obj_align(right_lbl, LV_ALIGN_RIGHT_MID, -4, 0);

        lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(row, _row_click_cb,     LV_EVENT_CLICKED,      data);
        lv_obj_add_event_cb(row, _row_longpress_cb, LV_EVENT_LONG_PRESSED, data);
        lv_obj_add_event_cb(row, _row_delete_cb,    LV_EVENT_DELETE,       data);
    }

    free(aps);

    if(s_scan_done) _wifi_add_rescan_row();
}

// ── Connection status poll (500 ms, list-page connects) ──
static void _nav_to_t9_cb(lv_timer_t * t)
{
    (void)t;
    s_conn_timer = NULL;   /* repeat_count=1 — auto-deleted after this call */
    _ui_screen_change(&ui_t9_keyboard, LV_SCR_LOAD_ANIM_FADE_ON, 350, 0,
                      &ui_t9_keyboard_screen_init);
}

static void _conn_poll_cb(lv_timer_t * t)
{
    (void)t;
    wifi_bridge_status_t st = ui_wifi_bridge_get_status();

    if(st == WIFI_BRIDGE_CONNECTED) {
        if(s_conn_timer) { lv_timer_del(s_conn_timer); s_conn_timer = NULL; }
        s_was_saved = false;
        char msg[80];
        char ip[24] = "";
        ui_wifi_bridge_get_ip(ip, sizeof(ip));
        if(ip[0])
            lv_snprintf(msg, sizeof(msg), "%s  %s", ui_wifi_bridge_get_current_ssid(), ip);
        else
            lv_snprintf(msg, sizeof(msg), "Connected: %s", ui_wifi_bridge_get_current_ssid());
        _wifi_show_toast(msg, lv_color_hex(0xC4F000));
        _wifi_start_scan();   /* refresh list with connected checkmark */
        _wifi_build_list();

    } else if(st == WIFI_BRIDGE_FAILED) {
        if(s_conn_timer) { lv_timer_del(s_conn_timer); s_conn_timer = NULL; }

        if(s_was_saved) {
            /* Saved-network password may have changed → T9 for re-entry */
            s_was_saved = false;
            _wifi_show_toast("Password may have changed", lv_color_hex(0xFF5555));
            char saved_pass[65] = {};
            ui_wifi_bridge_get_saved_pass(saved_pass, sizeof(saved_pass));
            ui_wifi_bridge_set_pending_ssid(s_focused.ssid);
            ui_t9_keyboard_prepare_for_wifi(s_focused.ssid);
            if(saved_pass[0]) ui_t9_keyboard_set_prefill(saved_pass);
            s_conn_timer = lv_timer_create(_nav_to_t9_cb, 1200, NULL);
            lv_timer_set_repeat_count(s_conn_timer, 1);
        } else {
            s_was_saved = false;
            _wifi_show_toast("Connection failed", lv_color_hex(0xFF5555));
            _wifi_start_scan();   /* restore network list */
            _wifi_build_list();
        }
    }
}

// ── Scan result poll (1 s) ──
static void _scan_timer_cb(lv_timer_t * t)
{
    (void)t;
    // The bridge runs a blocking scan in a FreeRTOS task; scan_complete()
    // becomes true only when that task finishes with real results.
    bool complete = ui_wifi_bridge_scan_complete();
    int  n        = ui_wifi_bridge_get_scan_count();

    if(complete && !s_scan_done) {
        s_scan_done   = true;
        s_scan_active = false;
        lv_timer_t * tmp = s_scan_timer;
        s_scan_timer = NULL;
        lv_timer_del(tmp);
        s_last_count = n;
        _wifi_build_list();
        return;
    }

    if(n != s_last_count) {
        s_last_count = n;
        _wifi_build_list();
    }
}

// ── Switch toggle ──
static void _switch_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;
    if(lv_obj_has_state(ui_wifi_scan_switch, LV_STATE_CHECKED)) {
        _wifi_start_scan();
        _wifi_build_list();
    } else {
        ui_wifi_bridge_stop_scan();
        if(s_scan_timer) { lv_timer_del(s_scan_timer); s_scan_timer = NULL; }
        s_scan_done   = false;
        s_scan_active = false;
        s_last_count  = 0;
        _wifi_build_list();
    }
}

// ── Key A → connect focused row ──
static void _key_a_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    ui_wifi_connect_focused();
}

// ── Key B → Settings ──
static void _key_b_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    _ui_screen_change(&ui_settings, LV_SCR_LOAD_ANIM_FADE_ON, 350, 0, &ui_settings_screen_init);
}

// ── Gesture swipe-right → Settings ──
void ui_event_wifi(lv_event_t * e)
{
    if(lv_event_get_code(e) == LV_EVENT_GESTURE &&
       lv_indev_get_gesture_dir(lv_indev_get_act()) == LV_DIR_RIGHT) {
        lv_indev_wait_release(lv_indev_get_act());
        _ui_screen_change(&ui_settings, LV_SCR_LOAD_ANIM_FADE_ON, 350, 0,
                          &ui_settings_screen_init);
    }
}

// ── Public: physical A button connects focused row ──
void ui_wifi_connect_focused(void)
{
    if(!s_focused.ssid[0]) return;
    _do_connect(s_focused.ssid, s_focused.saved != 0, s_focused.encrypted != 0);
}

// ────────────────────────────────────────────────────────────────────────────
void ui_wifi_screen_init(void)
{
    s_scan_timer    = NULL;
    s_conn_timer    = NULL;
    s_toast         = NULL;
    s_last_count    = -1;
    s_scan_done     = false;
    s_scan_active   = false;
    s_was_saved     = false;
    s_focused.ssid[0] = '\0';
    s_menu          = NULL;

    ui_wifi = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_wifi, LV_OBJ_FLAG_SCROLLABLE);

    ui_wifi_bg = lv_img_create(ui_wifi);
    lv_img_set_src(ui_wifi_bg, &ui_img_background_v_png);
    lv_obj_set_width(ui_wifi_bg, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_wifi_bg, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_wifi_bg, 0);
    lv_obj_set_y(ui_wifi_bg, -1);
    lv_obj_set_align(ui_wifi_bg, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_wifi_bg, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_clear_flag(ui_wifi_bg, LV_OBJ_FLAG_SCROLLABLE);

    ui_wifi_header_bg = lv_img_create(ui_wifi);
    lv_img_set_src(ui_wifi_header_bg, &ui_img_header_png);
    lv_obj_set_width(ui_wifi_header_bg, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_wifi_header_bg, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_wifi_header_bg, 10);
    lv_obj_set_y(ui_wifi_header_bg, 3);
    lv_obj_add_flag(ui_wifi_header_bg, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_clear_flag(ui_wifi_header_bg, LV_OBJ_FLAG_SCROLLABLE);

    ui_wifi_list_border = lv_img_create(ui_wifi);
    lv_img_set_src(ui_wifi_list_border, &ui_img_border_png);
    lv_obj_set_width(ui_wifi_list_border, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_wifi_list_border, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_wifi_list_border, 10);
    lv_obj_set_y(ui_wifi_list_border, 47);
    lv_obj_add_flag(ui_wifi_list_border, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_clear_flag(ui_wifi_list_border, LV_OBJ_FLAG_SCROLLABLE);

    ui_wifi_key_prompts_bg = lv_img_create(ui_wifi);
    lv_img_set_src(ui_wifi_key_prompts_bg, &ui_img_key_prompts_bg_png);
    lv_obj_set_width(ui_wifi_key_prompts_bg, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_wifi_key_prompts_bg, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_wifi_key_prompts_bg, 168);
    lv_obj_set_y(ui_wifi_key_prompts_bg, 204);
    lv_obj_add_flag(ui_wifi_key_prompts_bg, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_clear_flag(ui_wifi_key_prompts_bg, LV_OBJ_FLAG_SCROLLABLE);

    ui_wifi_key_a_bg = lv_img_create(ui_wifi);
    lv_img_set_src(ui_wifi_key_a_bg, &ui_img_key_a_png);
    lv_obj_set_width(ui_wifi_key_a_bg, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_wifi_key_a_bg, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_wifi_key_a_bg, 171);
    lv_obj_set_y(ui_wifi_key_a_bg, 207);
    lv_obj_add_flag(ui_wifi_key_a_bg, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(ui_wifi_key_a_bg, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_add_event_cb(ui_wifi_key_a_bg, _key_a_cb, LV_EVENT_CLICKED, NULL);

    ui_wifi_key_b_bg = lv_img_create(ui_wifi);
    lv_img_set_src(ui_wifi_key_b_bg, &ui_img_key_b_png);
    lv_obj_set_width(ui_wifi_key_b_bg, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_wifi_key_b_bg, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_wifi_key_b_bg, 241);
    lv_obj_set_y(ui_wifi_key_b_bg, 207);
    lv_obj_add_flag(ui_wifi_key_b_bg, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(ui_wifi_key_b_bg, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_add_event_cb(ui_wifi_key_b_bg, _key_b_cb, LV_EVENT_CLICKED, NULL);

    ui_wifi_header_label = lv_label_create(ui_wifi);
    lv_obj_set_width(ui_wifi_header_label, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_wifi_header_label, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_wifi_header_label, 80);
    lv_obj_set_y(ui_wifi_header_label, 14);
    lv_label_set_text(ui_wifi_header_label, "WLAN Setup");
    lv_obj_set_style_text_color(ui_wifi_header_label, lv_color_hex(0x000000), 0);
    lv_obj_set_style_text_font(ui_wifi_header_label, &ui_font_name_24, 0);

    // Wi-Fi on/off switch (child of header_bg)
    ui_wifi_scan_switch = lv_switch_create(ui_wifi_header_bg);
    lv_obj_set_size(ui_wifi_scan_switch, 42, 22);
    lv_obj_set_x(ui_wifi_scan_switch, 252);
    lv_obj_set_y(ui_wifi_scan_switch, 12);
    lv_obj_set_style_bg_color(ui_wifi_scan_switch, lv_color_hex(0x3D414A),
                               LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_wifi_scan_switch, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui_wifi_scan_switch, LV_RADIUS_CIRCLE,
                             LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_wifi_scan_switch, lv_color_hex(0xC4F000),
                               LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_style_bg_opa(ui_wifi_scan_switch, 255, LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_style_radius(ui_wifi_scan_switch, LV_RADIUS_CIRCLE,
                             LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_style_border_color(ui_wifi_scan_switch, lv_color_hex(0xFFFFFF),
                                   LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_wifi_scan_switch, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_wifi_scan_switch, lv_color_hex(0xFFFFFF),
                               LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_wifi_scan_switch, 255, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui_wifi_scan_switch, LV_RADIUS_CIRCLE,
                             LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(ui_wifi_scan_switch, _switch_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // AP list container — 152px = 4 rows × 38px
    ui_wifi_list = lv_obj_create(ui_wifi);
    lv_obj_set_size(ui_wifi_list, 290, 152);
    lv_obj_set_x(ui_wifi_list, 15);
    lv_obj_set_y(ui_wifi_list, 52);
    lv_obj_set_style_bg_opa(ui_wifi_list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ui_wifi_list, 0, 0);
    lv_obj_set_style_pad_all(ui_wifi_list, 0, 0);
    lv_obj_set_style_pad_row(ui_wifi_list, 0, 0);
    lv_obj_set_scroll_dir(ui_wifi_list, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(ui_wifi_list, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(ui_wifi_list, LV_FLEX_FLOW_COLUMN);

    // Key labels — screen-level children at unified system coordinates
    ui_wifi_key_a_home = lv_label_create(ui_wifi);
    lv_obj_set_width(ui_wifi_key_a_home, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_wifi_key_a_home, LV_SIZE_CONTENT);
    lv_label_set_text(ui_wifi_key_a_home, "Connect");
    lv_obj_set_style_text_color(ui_wifi_key_a_home, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_wifi_key_a_home, &ui_font_name_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align_to(ui_wifi_key_a_home, ui_wifi_key_a_bg, LV_ALIGN_CENTER, 0, 0);

    ui_wifi_key_b_back = lv_label_create(ui_wifi);
    lv_obj_set_width(ui_wifi_key_b_back, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_wifi_key_b_back, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_wifi_key_b_back, 267);
    lv_obj_set_y(ui_wifi_key_b_back, 210);
    lv_label_set_text(ui_wifi_key_b_back, "Back");
    lv_obj_set_style_text_color(ui_wifi_key_b_back, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_wifi_key_b_back, &ui_font_name_14, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_add_event_cb(ui_wifi, ui_event_wifi, LV_EVENT_ALL, NULL);

    /* ── On-entry state restore ───────────────────────────────── */
    wifi_bridge_status_t st = ui_wifi_bridge_get_status();
    if(st == WIFI_BRIDGE_CONNECTING) {
        /* Returning from T9 while a connection is still in-flight — poll */
        char msg[80];
        lv_snprintf(msg, sizeof(msg), "Connecting: %s",
                    ui_wifi_bridge_get_current_ssid());
        _wifi_show_toast(msg, lv_color_hex(0xFFFFFF));
        s_conn_timer = lv_timer_create(_conn_poll_cb, 500, NULL);
    } else {
        /* Auto-scan on entry (Apple-style: list appears immediately) */
        _wifi_start_scan();
        if(st == WIFI_BRIDGE_CONNECTED) {
            char msg[80];
            char ip[24] = "";
            ui_wifi_bridge_get_ip(ip, sizeof(ip));
            if(ip[0])
                lv_snprintf(msg, sizeof(msg), "%s  %s",
                            ui_wifi_bridge_get_current_ssid(), ip);
            else
                lv_snprintf(msg, sizeof(msg), "Connected: %s",
                            ui_wifi_bridge_get_current_ssid());
            _wifi_show_toast(msg, lv_color_hex(0xC4F000));
        }
    }
    _wifi_build_list();
}

void ui_wifi_screen_destroy(void)
{
    if(s_scan_timer) { lv_timer_del(s_scan_timer); s_scan_timer = NULL; }
    if(s_conn_timer) { lv_timer_del(s_conn_timer); s_conn_timer = NULL; }
    if(ui_wifi)      lv_obj_del(ui_wifi);

    ui_wifi                = NULL;
    ui_wifi_bg             = NULL;
    ui_wifi_header_bg      = NULL;
    ui_wifi_list_border    = NULL;
    ui_wifi_key_prompts_bg = NULL;
    ui_wifi_key_a_bg       = NULL;
    ui_wifi_key_b_bg       = NULL;
    ui_wifi_header_label   = NULL;
    ui_wifi_key_a_home     = NULL;
    ui_wifi_key_b_back     = NULL;
    ui_wifi_scan_switch    = NULL;
    ui_wifi_list           = NULL;
    s_toast                = NULL;
    s_menu                 = NULL;
}
