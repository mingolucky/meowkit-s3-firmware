/* Banc d'essai LovyanGFX sur SDL : rend la face procédurale du VU-mètre en
 * 320x240, sans matériel ni audio. Sert à mettre au point le dessin.
 *
 *   ./sim-gfx/build/vu-face            fenêtre
 *   ./sim-gfx/build/vu-face out.bmp    rendu puis capture
 */
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/sdl/Panel_sdl.hpp>
#include <cstdio>
#include <cstring>
#include "../src/app/app_03/vu_face.h"

class LGFX : public lgfx::LGFX_Device {
    lgfx::Panel_sdl _panel;
public:
    LGFX(int scale = 3) {
        { auto cfg = _panel.config();
          cfg.memory_width = cfg.panel_width = 320;
          cfg.memory_height = cfg.panel_height = 240;
          _panel.config(cfg); }
        _panel.setScaling(scale, scale);
        setPanel(&_panel);
    }
};

static LGFX lcd;
static const char* g_shot = nullptr;

void setup(void)
{
    lcd.init();
    /* On garde 16 bits, comme le ST7789 de l'appareil : le rendu est fidèle,
     * et la capture convertit. */
    vu_face_draw(&lcd, 0, 0);
    if (g_shot) {
        FILE* f = fopen(g_shot, "wb");
        if (f) {
            const int W = 320, H = 240, rowb = W * 3, pad = (4 - (rowb % 4)) % 4;
            const int data = (rowb + pad) * H, total = 54 + data;
            unsigned char hdr[54] = {0};
            hdr[0]='B'; hdr[1]='M';
            memcpy(hdr+2,&total,4); hdr[10]=54; hdr[14]=40;
            memcpy(hdr+18,&W,4); int hn=-H; memcpy(hdr+22,&hn,4);
            hdr[26]=1; hdr[28]=24; memcpy(hdr+34,&data,4);
            fwrite(hdr,1,54,f);
            for (int y = 0; y < H; y++) {
                for (int x = 0; x < W; x++) {
                    /* readPixel rend du RGB565 : on étend chaque composante. */
                    const uint32_t c = lcd.readPixel(x, y);
                    const unsigned char r = (unsigned char)((((c >> 11) & 0x1F) * 255 + 15) / 31);
                    const unsigned char g = (unsigned char)((((c >>  5) & 0x3F) * 255 + 31) / 63);
                    const unsigned char b = (unsigned char)((( c        & 0x1F) * 255 + 15) / 31);
                    unsigned char px[3] = { b, g, r };   /* BMP = BGR */
                    fwrite(px,1,3,f);
                }
                for (int p = 0; p < pad; p++) fputc(0, f);
            }
            fclose(f);
            printf("capture écrite : %s\n", g_shot);
        }
        exit(0);
    }
}

void loop(void) { lgfx::delay(50); }

int main(int argc, char** argv)
{
    if (argc > 1) g_shot = argv[1];
    return lgfx::Panel_sdl::main([](bool* running) -> int {
        setup();
        while (*running) loop();
        return 0;
    });
}
