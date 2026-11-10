/* Simulateur de bureau pour l'UI du MeowKit-S3.
 *
 * Rend l'interface LVGL du firmware dans une fenêtre SDL2 en 320×240, la
 * résolution exacte de l'écran ST7789. La souris tient lieu de dalle tactile
 * (FT6336).
 *
 *   ./meowkit-sim                          fenêtre interactive
 *   ./meowkit-sim --screen tabview         démarre sur un écran précis
 *   ./meowkit-sim --screen wifi --shot a.bmp   capture hors écran, puis sortie
 *   ./meowkit-sim --list                   liste les écrans disponibles
 *
 * Le mode capture force le pilote vidéo « dummy » : il fonctionne sans serveur
 * graphique, ce qui permet de vérifier un correctif d'affichage en CI ou
 * depuis un terminal.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include "lvgl.h"
#include "ui.h"

/* UI de l'app PC Monitor (src/app/app_05/asset) — déclarée à la main pour
 * éviter un conflit de noms entre les deux « ui.h » du projet. */
void ui_PC_Monitor_screen_init(void);
extern lv_obj_t *ui_PC_Monitor;

#define HOR_RES 320
#define VER_RES 240
#define SCALE   3          /* fenêtre agrandie ×3, sinon c'est un timbre-poste */

static uint16_t      framebuffer[HOR_RES * VER_RES];
static SDL_Window   *window;
static SDL_Renderer *renderer;
static SDL_Texture  *texture;

/* LVGL nous rend une zone rectangulaire ; on la recopie dans le framebuffer.
 * LV_COLOR_DEPTH vaut 16 et LV_COLOR_16_SWAP 0 → RGB565 natif, pas de
 * conversion nécessaire. */
static void flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p)
{
    for (int y = area->y1; y <= area->y2; y++) {
        for (int x = area->x1; x <= area->x2; x++) {
            if (x >= 0 && x < HOR_RES && y >= 0 && y < VER_RES) {
                framebuffer[y * HOR_RES + x] = color_p->full;
            }
            color_p++;
        }
    }
    lv_disp_flush_ready(drv);
}

/* La souris simule le tactile capacitif. */
static void mouse_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    (void)drv;
    int x = 0, y = 0;
    Uint32 buttons = SDL_GetMouseState(&x, &y);
    data->point.x = (lv_coord_t)(x / SCALE);
    data->point.y = (lv_coord_t)(y / SCALE);
    data->state   = (buttons & SDL_BUTTON(SDL_BUTTON_LEFT))
                        ? LV_INDEV_STATE_PRESSED
                        : LV_INDEV_STATE_RELEASED;
}

static void present(void)
{
    SDL_UpdateTexture(texture, NULL, framebuffer, HOR_RES * sizeof(uint16_t));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

static int save_bmp(const char *path)
{
    SDL_Surface *s = SDL_CreateRGBSurfaceWithFormatFrom(
        framebuffer, HOR_RES, VER_RES, 16, HOR_RES * sizeof(uint16_t),
        SDL_PIXELFORMAT_RGB565);
    if (!s) { fprintf(stderr, "surface: %s\n", SDL_GetError()); return 1; }
    int rc = SDL_SaveBMP(s, path);
    SDL_FreeSurface(s);
    if (rc != 0) { fprintf(stderr, "SDL_SaveBMP: %s\n", SDL_GetError()); return 1; }
    printf("capture écrite : %s (%dx%d)\n", path, HOR_RES, VER_RES);
    return 0;
}

/* Table des écrans : chaque entrée associe un nom de ligne de commande à sa
 * fonction d'initialisation et à l'objet racine à charger. Permet de capturer
 * n'importe quel écran sans naviguer à la souris. */
typedef struct {
    const char *name;
    void      (*init)(void);
    lv_obj_t **screen;
} screen_entry_t;

static const screen_entry_t SCREENS[] = {
    { "home",          ui_home_screen_init,          &ui_home          },
    { "apps",          ui_apps_menu_screen_init,     &ui_apps_menu     },
    { "clock",         ui_clock_screen_init,         &ui_clock         },
    { "settings",      ui_settings_screen_init,      &ui_settings      },
    { "tabview",       ui_tabview_screen_init,       &ui_tabview       },
    { "wifi",          ui_wifi_screen_init,          &ui_wifi          },
    { "files",         ui_sd_card_files_screen_init, &ui_sd_card_files },
    { "manual",        ui_manual_screen_init,        &ui_manual        },
    { "usb_msc",       ui_usb_msc_screen_init,       &ui_usb_msc       },
    { "update",        ui_update_screen_init,        &ui_update        },
    { "t9",            ui_t9_keyboard_screen_init,   &ui_t9_keyboard   },
    { "pcmon",         ui_PC_Monitor_screen_init,    &ui_PC_Monitor    },
};
#define SCREEN_COUNT ((int)(sizeof(SCREENS) / sizeof(SCREENS[0])))

int main(int argc, char **argv)
{
    const char *shot_path   = NULL;
    const char *screen_name = NULL;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--shot")   == 0 && i + 1 < argc) shot_path   = argv[++i];
        else if (strcmp(argv[i], "--screen") == 0 && i + 1 < argc) screen_name = argv[++i];
        else if (strcmp(argv[i], "--list") == 0) {
            printf("Écrans disponibles :\n");
            for (int k = 0; k < SCREEN_COUNT; k++) printf("  %s\n", SCREENS[k].name);
            return 0;
        }
    }
    if (shot_path) SDL_setenv("SDL_VIDEODRIVER", "dummy", 1);

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    window = SDL_CreateWindow("MeowKit-S3 — simulateur UI",
                              SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              HOR_RES * SCALE, VER_RES * SCALE, 0);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    texture  = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565,
                                 SDL_TEXTUREACCESS_STREAMING, HOR_RES, VER_RES);
    if (!window || !renderer || !texture) {
        fprintf(stderr, "SDL: %s\n", SDL_GetError());
        return 1;
    }

    lv_init();

    static lv_disp_draw_buf_t draw_buf;
    static lv_color_t         buf[HOR_RES * 40];
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, HOR_RES * 40);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res  = HOR_RES;
    disp_drv.ver_res  = VER_RES;
    disp_drv.flush_cb = flush_cb;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type    = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = mouse_read_cb;
    lv_indev_drv_register(&indev_drv);

    ui_init();

    if (screen_name) {
        const screen_entry_t *sel = NULL;
        for (int k = 0; k < SCREEN_COUNT; k++) {
            if (strcmp(SCREENS[k].name, screen_name) == 0) { sel = &SCREENS[k]; break; }
        }
        if (!sel) {
            fprintf(stderr, "écran inconnu : %s (voir --list)\n", screen_name);
            return 2;
        }
        if (*sel->screen == NULL) sel->init();   /* création paresseuse */
        lv_disp_load_scr(*sel->screen);
    }

    if (shot_path) {
        /* Laisse l'UI se stabiliser (thème, images, animations d'entrée). */
        for (int i = 0; i < 90; i++) { lv_timer_handler(); SDL_Delay(16); }
        int rc = save_bmp(shot_path);
        SDL_Quit();
        return rc;
    }

    bool running = true;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_ESCAPE) running = false;
                if (e.key.keysym.sym == SDLK_s) save_bmp("meowkit-shot.bmp");
            }
        }
        lv_timer_handler();
        present();
        SDL_Delay(5);
    }
    SDL_Quit();
    return 0;
}
