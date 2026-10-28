# ab_starwars - ReXGlue Recompiled Project
#
# SDK boilerplate lives in generated/rexglue.cmake.

cmake_minimum_required(VERSION 3.25)
project(ab_starwars LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(generated/rexglue.cmake)

# Sources
set(ABSTARWARS_SOURCES
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
    src/frame_rate.cpp
    src/live_notices.cpp
    src/guest_exceptions.cpp
    src/no_multiplayer.cpp
    src/xam_quiet.cpp
    src/dialog_pad.cpp
)

if(WIN32)
    add_executable(ab_starwars WIN32 ${ABSTARWARS_SOURCES})
else()
    add_executable(ab_starwars ${ABSTARWARS_SOURCES})
endif()

rexglue_setup_target(ab_starwars GPU_PLUGINS xenos)

if(WIN32)
    # UTF-8 as the program's code page, and a linker map for decoding crash reports.
    target_link_options(ab_starwars PRIVATE "LINKER:/MANIFEST:EMBED" "LINKER:/MANIFESTUAC:NO"
                        "LINKER:/MANIFESTINPUT:${CMAKE_CURRENT_SOURCE_DIR}/src/ab_starwars.manifest"
                        "LINKER:/MAP")
endif()
