# Simulateur de bureau pour l'UI du MeowKit-S3.
#
# Compile la LVGL vendorée (8.3.11) et l'UI générée par SquareLine (src/ui)
# contre SDL2, pour itérer sur l'affichage sans flasher l'appareil.
#
#   cmake -S sim -B sim/build && cmake --build sim/build -j
#   ./sim/build/meowkit-sim
#
# SDL2 est optionnel : sans lui, seules les bibliothèques `lvgl` et `ui` sont
# construites (utile pour valider la compilation).
cmake_minimum_required(VERSION 3.16)
project(meowkit-sim C)

set(CMAKE_C_STANDARD 11)
set(FW_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)

# ── LVGL 8.3.11 vendorée ──────────────────────────────────────────────
file(GLOB_RECURSE LVGL_SRC ${FW_ROOT}/lib/lvgl/src/*.c)
add_library(lvgl STATIC ${LVGL_SRC})
target_include_directories(lvgl PUBLIC
    ${FW_ROOT}/lib/lvgl          # « lvgl.h »
    ${FW_ROOT}/lib               # « lv_conf.h »
    ${CMAKE_CURRENT_SOURCE_DIR}/compat)   # « Arduino.h » (shim millis)
target_compile_definitions(lvgl PUBLIC
    LV_CONF_INCLUDE_SIMPLE=1
    # lv_conf.h fait « #define LV_ATTRIBUTE_FAST_MEM IRAM_ATTR » : IRAM_ATTR est
    # une macro ESP-IDF qui place le code en SRAM interne. Sans objet sur PC — on
    # la vide ici plutôt que de modifier lv_conf.h.
    IRAM_ATTR=)

# ── UI générée par SquareLine ─────────────────────────────────────────
# NB : on globbe au lieu d'utiliser src/ui/CMakeLists.txt, qui est périmé
# (il liste 3 écrans inexistants et en omet 8 qui existent).
file(GLOB UI_SRC
    ${FW_ROOT}/src/ui/*.c
    ${FW_ROOT}/src/ui/screens/*.c
    ${FW_ROOT}/src/ui/components/*.c
    ${FW_ROOT}/src/ui/fonts/*.c
    ${FW_ROOT}/src/ui/images/*.c)
# L'UI de l'app PC Monitor (app_05) est du C LVGL pur, donc simulable aussi.
# Elle embarque sa PROPRE police (ui_font_pc_temp.c), distincte de src/ui/fonts.
file(GLOB APP05_SRC ${FW_ROOT}/src/app/app_05/asset/*.c)

add_library(ui STATIC ${UI_SRC} ${APP05_SRC})
target_link_libraries(ui PUBLIC lvgl)
target_include_directories(ui PUBLIC ${FW_ROOT}/src/ui ${FW_ROOT}/src/app/app_05/asset)

# ── Exécutable du simulateur (si SDL2 présent) ────────────────────────
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(SDL2 QUIET sdl2)
endif()

if(SDL2_FOUND)
    add_executable(meowkit-sim src/main.c src/stubs.c)
    target_link_libraries(meowkit-sim PRIVATE ui ${SDL2_LIBRARIES} m)
    target_include_directories(meowkit-sim PRIVATE ${SDL2_INCLUDE_DIRS} ${FW_ROOT}/src)
    target_compile_options(meowkit-sim PRIVATE ${SDL2_CFLAGS_OTHER})
    message(STATUS "SDL2 ${SDL2_VERSION} trouvé — cible « meowkit-sim » activée")
else()
    message(WARNING "SDL2 absent : seules les bibliothèques lvgl/ui sont construites.\n"
                    "  Fedora : sudo dnf install SDL2-devel")
endif()
