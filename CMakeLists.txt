# Angry Birds: The Re-Hatched Collection's launcher (TRG Launcher 2.0, trg-launcher/launcher2).
cmake_minimum_required(VERSION 3.25)
project(angry_birds_launcher LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)  # the launcher is C++17 (u8string is std::string there)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# The C runtime linked in, so the launcher runs on its own.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

set(TRG_LAUNCHER_STANDALONE ON CACHE BOOL "" FORCE)
set(TRG_LAUNCHER_BUILD_DEMO OFF CACHE BOOL "" FORCE)
set(TRG_LAUNCHER_2 ON CACHE BOOL "" FORCE)
set(TRG_LAUNCHER2_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/trg-launcher)

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" AB_VERSION)
string(STRIP "${AB_VERSION}" AB_VERSION)
string(REPLACE "." "," _commas "${AB_VERSION}")

add_executable(angry_birds_launcher WIN32 src/project.cpp src/launcher.rc)
target_link_libraries(angry_birds_launcher PRIVATE trg::launcher2)
target_compile_definitions(angry_birds_launcher PRIVATE PROJECT_VERSION="${AB_VERSION}"
  PROJECT_VERSION_COMMAS=${_commas},0)
if(MSVC)
  target_compile_options(angry_birds_launcher PRIVATE /W4 /utf-8 /permissive-)
  # A WIN32 program (no console window) that starts at main().
  target_link_options(angry_birds_launcher PRIVATE /ENTRY:mainCRTStartup)
endif()
