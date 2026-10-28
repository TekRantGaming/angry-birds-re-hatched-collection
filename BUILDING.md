# Building the collection

One repo holds the launcher and both programs:

| Path | What |
|---|---|
| `src/`, `CMakeLists.txt`, `build.bat` | The launcher (Dear ImGui, SDL2, OpenGL; the TRG Launcher 2.0 in `third_party/trg-launcher`) |
| `games/ab_trilogy` | Angry Birds Trilogy: Classic, Seasons and Rio in one program (ReXGlue project) |
| `games/ab_starwars` | Angry Birds Star Wars (ReXGlue project) |
| `tools/` | The script that makes the Add to Steam artwork and the launcher's logo |
| `docs/steam/` | The Add to Steam artwork, shipped beside the launcher as `steam-art/` |

`third_party/trg-launcher` is TRG Launcher 2.0 (v2.0.0) with this collection's additions, which keep every other
project's behaviour: games that share one program and disc (`GameInfo::folder`), a game's own arguments for Play
(`GameInfo::launch_args`), one Resolution row (`Features::output_resolution`), the Frame rate row's choices
(`ProjectConfig::frame_rates`), a controller only project (`Features::keyboard_mouse`), and Add to Steam artwork from
a folder beside the launcher.

## Requirements

* Visual Studio 2022 with the C++ tools (the games build with its LLVM clang), CMake 3.25+, Ninja, Python 3.
* The ReXGlue SDK with TekRantGaming's patches (FSR 1 and NIS among them), installed to an install prefix. Link it
  as `games/<game>/tools/rexglue/win-amd64` (a junction to the SDK's `out/install/win-amd64`).

## The launcher

```
build.bat
```

Output: `build/angry_birds_launcher.exe`.

## A game

Each game needs code generated from your own disc's `default.xex` (never committed: it is derived from the game):

1. Extract the disc image into `games/<game>/assets` (the folder that holds `default.xex`), for example with
   `python games/<game>/tools/xiso_extract.py <image.iso> games/<game>/assets`.
2. Run the ReXGlue codegen with the game's `<game>_manifest.toml` (it writes `games/<game>/generated/`).
3. Build: `games/<game>/build.bat <short>-release`, where short is `abt` (Trilogy) or `absw` (Star Wars).

Output: `games/<game>/out/build/<short>-release/<game>.exe`.

## The folder players get

```
angry_birds_launcher.exe
steam-art/                     cover.png, wide.png, hero.png, logo.png, icon.png (from docs/steam)
games/ab_trilogy/              ab_trilogy.exe, rexruntime.dll, rexgpu-xenos.dll
games/ab_starwars/             ab_starwars.exe, rexruntime.dll, rexgpu-xenos.dll
```

The launcher installs each disc into `games/<game>/game` and writes each game's settings to `games/<game>/<game>.toml`.

## Developer aids

Both games have switches for scripted tests, all off unless their environment variable is set (see each game's
`src/dev_tools.cpp`, `src/input.cpp` and `tools/run.ps1`): a scripted controller, frame captures, a memory search
and a mixer level meter. Trilogy's `tools/luadis.py` lists the game's Lua menu scripts and `tools/find_lua_regs.py`
maps their Lua functions to the game's code.
