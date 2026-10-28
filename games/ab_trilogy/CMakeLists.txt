# ab_trilogy - ReXGlue Recompiled Project
#
# SDK boilerplate lives in generated/rexglue.cmake.

cmake_minimum_required(VERSION 3.25)
project(ab_trilogy LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(generated/rexglue.cmake)

# Sources
set(ABTRILOGY_SOURCES
    src/main.cpp
    src/fpe_guard.cpp
    src/platform.cpp
    src/dev_tools.cpp
    src/input.cpp
    src/frame_stats.cpp
    src/port_defaults.cpp
    src/return_to_launcher.cpp
    src/game_window.cpp
    src/achievement_toast.cpp
    src/achievements_export.cpp
    src/render_quality.cpp
    src/skip_intros.cpp
    src/direct_start.cpp
    src/frame_rate.cpp
)

if(WIN32)
    add_executable(ab_trilogy WIN32 ${ABTRILOGY_SOURCES})
else()
    add_executable(ab_trilogy ${ABTRILOGY_SOURCES})
endif()

rexglue_setup_target(ab_trilogy GPU_PLUGINS xenos)

if(WIN32)
    # UTF-8 as the program's code page, and a linker map for decoding crash reports.
    target_link_options(ab_trilogy PRIVATE "LINKER:/MANIFEST:EMBED" "LINKER:/MANIFESTUAC:NO"
                        "LINKER:/MANIFESTINPUT:${CMAKE_CURRENT_SOURCE_DIR}/src/ab_trilogy.manifest"
                        "LINKER:/MAP")
endif()
