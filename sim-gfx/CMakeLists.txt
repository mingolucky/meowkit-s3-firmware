# Banc d'essai LovyanGFX/SDL — rend les écrans dessinés en direct (hors LVGL).
cmake_minimum_required(VERSION 3.16)
project(vu-face C CXX)
set(CMAKE_CXX_STANDARD 17)
# lgfx_qrcode.c fait « typedef unsigned char bool », interdit depuis C23 —
# GCC utilisant C23 par défaut, on revient à C11 pour les sources C.
set(CMAKE_C_STANDARD 11)
set(FW ${CMAKE_CURRENT_SOURCE_DIR}/..)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

file(GLOB_RECURSE LGFX_SRC ${FW}/lib/LovyanGFX/src/lgfx/v1/*.cpp)
# décodeurs et utilitaires en C (png, jpeg, qrcode…)
file(GLOB LGFX_UTIL ${FW}/lib/LovyanGFX/src/lgfx/utility/*.c)
# données de polices (efont) référencées par lgfx_fonts.cpp
file(GLOB_RECURSE LGFX_FONTS ${FW}/lib/LovyanGFX/src/lgfx/Fonts/*.c)
list(FILTER LGFX_SRC EXCLUDE REGEX "platforms/(esp32|esp8266|samd|stm32|rp2040|spresense|opencv|arduino_default|framebuffer)")

add_executable(vu-face main.cpp ${FW}/src/app/app_03/vu_face.cpp ${LGFX_SRC} ${LGFX_UTIL} ${LGFX_FONTS})
target_include_directories(vu-face PRIVATE ${FW}/lib/LovyanGFX/src ${SDL2_INCLUDE_DIRS})
target_compile_definitions(vu-face PRIVATE LGFX_SDL LGFX_USE_V1)
target_link_libraries(vu-face PRIVATE ${SDL2_LIBRARIES})
