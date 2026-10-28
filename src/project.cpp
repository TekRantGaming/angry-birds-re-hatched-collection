// Angry Birds: The Re-Hatched Collection's launcher, a TRG Launcher 2.0 project (trg-launcher/launcher2): everything
// specific to the collection is in this one file. See trg-launcher/launcher2/README.md.
//
// Four games from two discs. Angry Birds Trilogy is one program (ab_trilogy.exe) holding Classic, Seasons and Rio; its
// three tiles share the folder games/ab_trilogy (GameInfo::folder: one disc install, one settings file, one save) and
// each starts its own game with --abt_start_game (abt/src/direct_start.cpp). Star Wars is ab_starwars.exe.

#include <trg/hub.h>

#include "logo_png.h"

namespace {

trg::hub::ProjectConfig AngryBirds() {
  using trg::hub::GameInfo;
  using trg::hub::InstallSource;

  trg::hub::ProjectConfig p;
  p.name = "Angry Birds: The Re-Hatched Collection";
  p.id = "angry_birds";
  p.version = PROJECT_VERSION;  // from CMake (the VERSION file)
  p.home_line = "Classic, Seasons, Rio and Star Wars";
  p.about = "One launcher for four Angry Birds games from the Xbox 360: Angry Birds Classic, Seasons and Rio from "
            "Angry Birds Trilogy, and Angry Birds Star Wars. Each is a native PC recompilation and needs your own "
            "disc.";
  p.made_by = "Rovio with Housemarque (Trilogy, 2012) and Exient (Star Wars, 2013), for Activision";
  p.region = "USA (NTSC)";
  p.logo_png = kAngryBirdsLogoPng;
  p.logo_png_size = sizeof kAngryBirdsLogoPng;

  // Art from each game's SteamGridDB page (https://www.steamgriddb.com/game/<id>): the part of an image's address
  // after https://cdn2.steamgriddb.com/. The most liked of each kind; others on those pages, for swapping:
  //   Classic   grids bf45563e1c0e8847f218b75bc72a62a3.png, 31eeeb84dd7a35b048c071c63eead7e1.png;
  //             heroes 1fd22b5a1bab767b6650d0290cec2e0b.png, 59843bfb2ca080a23bcbbc5278fa6982.png
  //   Seasons   grids ca73fd0a8a9c9263c48e2029e77a4c40.png, ba50580b3b51d85c69fb484502f9c691.png;
  //             heroes ab1ab2e705f17b51638e0f78ce3bdd83.png, fb30d3b7fa58e575c4bf59bb899c314d.jpg
  //   Rio       grids e3479eb78b49041b1ae76cfe93232e59.png, 7a9f1ac9e589b25c03c62d73c601fa0b.png;
  //             heroes f2bd2dc4768de1aadf66b36c31e29e7c.png
  //   Star Wars grids 2093e245fbe93dad5fff155f253813b0.png; heroes e03936e26cd0d5496431ed281e35dacb.png
  // The disc facts come from the images the ports were made with and their default.xex.
  const char* trilogy_iso = "Angry Birds Trilogy (USA, Europe) (En,Fr,De,Es,It).iso";
  GameInfo classic = {
      "ab_classic", "Angry Birds Classic", "ab_trilogy.exe", "AngryBirdsTrilogy", 0x415608E4,
      ImVec4(0.89f, 0.14f, 0.10f, 1.0f), "The original: Poached Eggs and on.", InstallSource::kDisc,
      5256824, "grid/a654ee305bc41e098906b48bf7596a1c.png", "hero/5da361ad62b75317c5c92cc168f95877.png",
      "logo/b20bd2a22c7f0aef29ab5a4cb1934f71.png",
      "0.0.0.2", trilogy_iso, "7,838,695,424 bytes", "AA9F23082753DCF7FA7A43C1E336BA0FB898F03B",
      "4B32990FFCE22F1DA3064D3AEB89C59A", "278C93DE", "1A15DA04", "about 1.3 GB"};
  classic.folder = "ab_trilogy";
  classic.launch_args = "--abt_start_game=classic";
  // native_heights stays at its default (720): Trilogy does draw at 1080p itself in a 1080p video mode, but there its
  // menus run at about 49 FPS, while 720p times the draw scale holds 60 at 1440p and 4K too (28 Oct 2026).
  GameInfo seasons = classic;
  seasons.id = "ab_seasons";
  seasons.title = "Angry Birds Seasons";
  seasons.accent = ImVec4(0.95f, 0.55f, 0.10f, 1.0f);
  seasons.tagline = "A year of holidays, from Trick or Treat on.";
  seasons.steamgriddb_id = 5256825;
  seasons.art_grid = "grid/b079d4eaa47388a928f09ede6aa04b6b.png";
  seasons.art_hero = "hero/a8de839ca3b50ef2eac03640ccf56afd.png";
  seasons.art_logo = "logo/54b1d475ba3398d7bfd516304215d53d.png";
  seasons.launch_args = "--abt_start_game=seasons";
  GameInfo rio = classic;
  rio.id = "ab_rio";
  rio.title = "Angry Birds Rio";
  rio.accent = ImVec4(0.15f, 0.55f, 0.85f, 1.0f);
  rio.tagline = "Free the caged birds of Rio.";
  rio.steamgriddb_id = 5258598;
  rio.art_grid = "grid/98d757b427efcfc1420b1aca6af8a006.png";
  rio.art_hero = "hero/5732c8f48b699d91047d62ad16125a72.png";
  rio.art_logo = "logo/564ed2dbdd6f0567500e8a1b7981f28b.png";
  rio.launch_args = "--abt_start_game=rio";
  GameInfo star_wars = {
      "ab_starwars", "Angry Birds Star Wars", "ab_starwars.exe", "AngryBirdsStarWars", 0x41560901,
      ImVec4(0.95f, 0.75f, 0.15f, 1.0f), "A long time ago, in a galaxy of pigs.", InstallSource::kDisc,
      34986, "grid/e6e7f1b78581818a296d2078d51d497b.png", "hero/7ce1b2bc1baf1b4ea2027e15fc2ed397.jpg",
      "logo/b578f2a52a0229873fefc2a4b06377fa.png",
      "0.0.0.2", "Angry Birds Star Wars (USA) (En,Fr,De,Es,It,Pt).iso", "7,838,695,424 bytes",
      "076C847F6CCE7BB536D42937C05CB5EA2D02123A", "DE6427E7EE6B2A3381EA2D708381B6FC", "DAA8E8B0", "116000C4",
      "about 1.3 GB"};
  p.games = {classic, seasons, rio, star_wars};

  // Releases (not published yet): vX for players, packages-vX for the launcher's downloads
  // (AngryBirdsCollection-, AngryBirdsTrilogy-, AngryBirdsStarWars-vX-windows-x64.zip, SHA256SUMS.txt).
  p.repo = "TekRantGaming/angry-birds-re-hatched-collection";
  p.packages_tag_prefix = "packages-v";
  p.launcher_zip = "AngryBirdsCollection";
  p.launcher_exe = "angry_birds_launcher";
  p.linux_data_folder = "angry_birds_collection";

  // The games' own launcher settings start with ab_ (both games have return_to_launcher.cpp, game_window.cpp and
  // frame_rate.cpp). The features whose game side hooks they do not have yet are off.
  p.setting_prefix = "ab_";
  p.from_launcher_flag = "--ab_from_launcher=true";
  p.test_background_env = "AB_TEST_BACKGROUND";
  p.release_server_env = "AB_RELEASE_SERVER";
  p.frame_rates = {"30", "60", "90", "120", "0"};  // owner's choice (28 Oct 2026)
  p.features.output_resolution = true;            // one Resolution row: what the game draws at and shows
  p.features.render_quality = true;  // render_quality.cpp in both games: upscaler modes from the chosen resolution
  p.features.controller_options = false;
  p.features.keyboard_mouse = false;  // controller only (owner, 28 Oct 2026)
  p.features.achievement_toasts = true;  // achievement_toast.cpp in both games

  p.discord_url = "https://discord.gg/8eakYSvBZS";
  // Add to Steam: the collection's own artwork (tools/make_steam_art.py) ships beside the launcher in steam-art/.
  p.steam_art_base = "steam-art/";
  return p;
}

}  // namespace

int main(int argc, char** argv) { return trg::hub::Run(AngryBirds(), argc, argv); }
