#include "vu_face.h"
#include <math.h>
#include <stdio.h>

namespace {

/* Palette : panneau vert uni, échelle en deux teintes seulement. */
constexpr uint32_t COL_BG     = 0x0A0B09u;  /* pourtour de l'écran */
constexpr uint32_t COL_PANEL  = 0xBDE700u;  /* vert uni            */
constexpr uint32_t COL_FRAME  = 0x141713u;  /* cadre fin           */
constexpr uint32_t COL_SCALE  = 0x101210u;  /* échelle, zone nominale */
constexpr uint32_t COL_RED    = 0xD8321Eu;  /* échelle, zone de crête */

/* Géométrie de l'aiguille — doit rester en accord avec vu_meter.h/.cpp. */
constexpr float PIVOT_X   = 160.0f;
constexpr float PIVOT_Y   = 239.0f;
/* Balayage symétrique : l'axe de l'arc tombe alors sur la verticale, au centre
 * de l'écran. L'app d'origine utilisait -45°/+35°, ce qui décalait le milieu de
 * l'arc de 9 px vers la gauche. Même amplitude (80°), donc même course. */
constexpr float ANGLE_MIN = -40.0f;   /* degrés, horaire depuis la verticale */
constexpr float ANGLE_MAX =  40.0f;
constexpr float R_SCALE   = 132.0f;   /* rayon de l'arc gradué  */
constexpr float R_TICK_IN = 118.0f;   /* base des graduations   */

/* Plage réellement parcourue par l'aiguille : l'app mappe linéairement
 * kDbFloor..kDbCeil sur ANGLE_MIN..ANGLE_MAX. La graduation reprend donc ces
 * valeurs plutôt que la convention VU de l'illustration d'origine, dont
 * l'échelle (30, 10, 7, 5, 3, 0, 3+) ne correspond pas au comportement réel. */
constexpr float DB_MIN = -36.0f;
constexpr float DB_MAX =  -6.0f;

/* Seuil de la zone de crête : le dernier cinquième du balayage. */
constexpr float DB_RED = -12.0f;

inline float dbToAngle(float db)
{
    const float t = (db - DB_MIN) / (DB_MAX - DB_MIN);
    return ANGLE_MIN + t * (ANGLE_MAX - ANGLE_MIN);
}

/* LovyanGFX compte les angles en degrés, 0° à l'est, sens horaire.
 * L'app les compte depuis la verticale : conversion. */
inline float toGfx(float a) { return a - 90.0f; }

inline void polar(float deg, float r, int32_t ox, int32_t oy, int32_t& x, int32_t& y)
{
    const float rad = deg * (float)M_PI / 180.0f;
    x = ox + (int32_t)lroundf(PIVOT_X + r * sinf(rad));
    y = oy + (int32_t)lroundf(PIVOT_Y - r * cosf(rad));
}

inline uint32_t rgb(LovyanGFX* g, uint32_t c)
{
    return g->color888((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF);
}

} // namespace

void vu_face_draw(LovyanGFX* gfx, int32_t ox, int32_t oy)
{
    if (!gfx) return;

    /* Pourtour, puis panneau vert uni cerné d'un cadre fin — l'illustration
     * d'origine repose sur le même principe, un panneau coloré dans un biseau. */
    /* Encadrement repris de l'illustration d'origine, mesuré dessus :
     * panneau x=40..279, y=14..208 — encastré, et non plein écran. */
    constexpr int32_t PX = 40, PY = 14, PW = 240, PH = 195, PR = 12;

    gfx->fillRect(ox, oy, 320, 240, rgb(gfx, COL_BG));
    gfx->fillSmoothRoundRect(ox + PX, oy + PY, PW, PH, PR, rgb(gfx, COL_PANEL));
    gfx->drawRoundRect(ox + PX, oy + PY, PW, PH, PR, rgb(gfx, COL_FRAME));
    gfx->drawRoundRect(ox + PX - 1, oy + PY - 1, PW + 2, PH + 2, PR + 1, rgb(gfx, COL_FRAME));

    /* L'arc gradué, en deux teintes : nominale puis crête. */
    const int32_t cx = ox + (int32_t)PIVOT_X;
    const int32_t cy = oy + (int32_t)PIVOT_Y;
    gfx->fillArc(cx, cy, (int32_t)R_SCALE - 3, (int32_t)R_SCALE,
                 toGfx(ANGLE_MIN), toGfx(dbToAngle(DB_RED)), rgb(gfx, COL_SCALE));
    gfx->fillArc(cx, cy, (int32_t)R_SCALE - 3, (int32_t)R_SCALE,
                 toGfx(dbToAngle(DB_RED)), toGfx(ANGLE_MAX), rgb(gfx, COL_RED));

    /* Graduations : longue et étiquetée tous les 6 dB, courte tous les 2. */
    gfx->setTextDatum(textdatum_t::middle_center);
    gfx->setTextFont(2);

    for (int i = 0; i <= 15; ++i) {            /* -36 → -6 par pas de 2 dB */
        const float db     = DB_MIN + i * 2.0f;
        const bool  major  = (i % 3) == 0;     /* tous les 6 dB */
        const uint32_t col = rgb(gfx, (db >= DB_RED) ? COL_RED : COL_SCALE);
        const float ang    = dbToAngle(db);

        int32_t x0, y0, x1, y1;
        polar(ang, major ? R_TICK_IN : R_TICK_IN + 9.0f, ox, oy, x0, y0);
        polar(ang, R_SCALE - 4.0f, ox, oy, x1, y1);
        gfx->drawWideLine(x0, y0, x1, y1, major ? 2.5f : 1.2f, col);

        if (major) {
            char buf[8];
            snprintf(buf, sizeof(buf), "%d", (int)-db);
            int32_t lx, ly;
            polar(ang, R_SCALE + 13.0f, ox, oy, lx, ly);
            gfx->setTextColor(col);
            gfx->drawString(buf, lx, ly);
        }
    }

    /* Le balayage est symétrique, le milieu de l'arc tombe donc sur l'axe. */
    gfx->setTextColor(rgb(gfx, COL_SCALE));
    gfx->drawString("dB", ox + (int32_t)PIVOT_X, oy + 160);

    /* À l'intérieur du panneau, comme sur l'illustration d'origine. */
    gfx->setTextDatum(textdatum_t::bottom_left);
    gfx->drawString("VU METER", ox + PX + 14, oy + PY + PH - 12);
    gfx->setTextDatum(textdatum_t::bottom_right);
    gfx->drawString("PEAK", ox + PX + PW - 14, oy + PY + PH - 12);
}
