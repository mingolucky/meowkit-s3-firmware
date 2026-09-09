/**
 * @file vu_face.h
 * @brief Face de VU-mètre dessinée, pour quand l'illustration manque.
 *
 * L'app VU Meter charge son fond depuis /vu_meter/vu_meter_bg.png sur la carte
 * SD. Sans carte — ou avec une carte neuve — elle échouait sur un « BG PNG not
 * found » rouge. Cette face procédurale prend le relais : l'app fonctionne dès
 * la sortie de boîte, et l'illustration d'origine reste utilisée quand elle est
 * disponible.
 *
 * Le dessin épouse la géométrie de l'aiguille (pivot 160,239 ; rayon 150 ;
 * balayage -45°..+35°) et reprend la palette des autres écrans de l'appareil.
 */
#pragma once
#include <LovyanGFX.hpp>

/* Balayage que cette face suppose. L'app doit s'y conformer quand elle
 * l'affiche, sinon l'aiguille ne pointerait pas ses propres graduations. */
constexpr float VU_FACE_ANGLE_MIN = -40.0f;
constexpr float VU_FACE_ANGLE_MAX =  40.0f;

/** Dessine la face à l'origine du repère fourni (320x240 attendu). */
void vu_face_draw(LovyanGFX* gfx, int32_t ox, int32_t oy);
