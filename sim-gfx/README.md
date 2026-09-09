# Banc d'essai LovyanGFX / SDL

Le simulateur `sim/` rend l'interface **LVGL** (`src/ui`). Plusieurs apps —
VU Meter, Dino, Matrix Rain, Retro TV — ne passent pas par LVGL : elles
dessinent directement en LovyanGFX, avec des sprites. Elles lui échappaient donc
totalement.

LovyanGFX embarque une plateforme SDL (`lib/LovyanGFX/src/lgfx/v1/platforms/sdl`).
Ce banc d'essai s'en sert pour rendre ces dessins sur PC, en 320×240.

```bash
cmake -S sim-gfx -B sim-gfx/build && cmake --build sim-gfx/build -j
./sim-gfx/build/vu-face            # fenêtre
./sim-gfx/build/vu-face out.bmp    # rendu puis capture, sans serveur graphique
```

## Trois obstacles rencontrés

- Les **décodeurs C** (`lgfx/utility/*.c`) et les **données de polices**
  (`lgfx/Fonts/**/*.c`) ne sont pas dans `lgfx/v1/` : il faut les compiler à part.
- `lgfx_qrcode.c` fait `typedef unsigned char bool`, interdit depuis C23 — que
  GCC utilise désormais par défaut. D'où `set(CMAKE_C_STANDARD 11)`.
- `readPixel()` rend la couleur **au format du panneau**, donc du RGB565 ici.
  La capture étend chaque composante ; le panneau reste en 16 bits, comme le
  ST7789 de l'appareil.
