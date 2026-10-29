# Makes one version of the collection, laid out as its two GitHub releases serve it (src/project.cpp).
#
#   powershell -ExecutionPolicy Bypass -File tools\make_release.ps1 -Version 0.1.0 -Out ..\releases
#   powershell -ExecutionPolicy Bypass -File tools\make_release.ps1 -Version 0.1.1 -Earlier 0.1.0 -Out ..\releases
#
# Writes <Out>\v<Version>\:
#   AngryBirdsCollection-v<Version>-windows-x64.zip   the release players see (tag v<Version>): the portable launcher
#                                with both programs in its games folder, unzipped anywhere; each game asks for the
#                                player's own disc image the first time
#   SHA256SUMS.txt               for that zip
#   packages\                    the packages release (tag packages-v<Version>), what the launcher downloads:
#     AngryBirdsCollection-v<Version>-windows-x64.zip              the launcher's own update
#     AngryBirdsTrilogy-, AngryBirdsStarWars-v<Version>-windows-x64.zip  the games
#     SHA256SUMS.txt                                                all three
# No game files: each zip holds the programs, the ReXGlue runtime, the Visual C++ runtime and the Add to Steam art,
# with a file list (update-files.txt) made by trg-launcher's make_update_files.py, so updates replace exactly the
# release's files and never the protected ones (disc files, settings, saves, logs). After the trg-launcher Whopping
# script (launcher2/examples/whopping/tools/make_release.ps1). The games must be built first with each game's
# build.bat (abt-release, absw-release), without AB_DEV_TOOLS; the launcher is rebuilt here.
param(
  [Parameter(Mandatory)][string]$Version,
  [Parameter(Mandatory)][string]$Out,
  [string]$Earlier = ''
)
$ErrorActionPreference = 'Stop'
$launcher = Split-Path -Parent $PSScriptRoot
# Two layouts: the repository (games\ab_trilogy, third_party\trg-launcher) and the working folders beside the
# launcher folder (..\abt, ..\absw, trg-launcher).
if (Test-Path (Join-Path $launcher 'games\ab_trilogy')) {
  $trilogy = Join-Path $launcher 'games\ab_trilogy'; $starWars = Join-Path $launcher 'games\ab_starwars'
  $tool = Join-Path $launcher 'third_party\trg-launcher\tools\make_update_files.py'
  $steamArt = Join-Path $launcher 'docs\steam'
} else {
  $root = Split-Path -Parent $launcher
  $trilogy = Join-Path $root 'abt'; $starWars = Join-Path $root 'absw'
  $tool = Join-Path $launcher 'trg-launcher\tools\make_update_files.py'
  $steamArt = Join-Path $launcher 'art\steam'
}
$sdk = Join-Path $trilogy 'tools\rexglue\win-amd64\bin'
$crt = 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Redist\MSVC\14.44.35112\x64\Microsoft.VC143.CRT'
$crtFiles = 'msvcp140.dll', 'msvcp140_atomic_wait.dll', 'vcruntime140.dll', 'vcruntime140_1.dll'
$parts = @(
  @{ Folder = 'AngryBirdsCollection'; Build = Join-Path $launcher 'build'; Exe = 'angry_birds_launcher.exe'; Sdk = $false;
     Extra = @{ 'steam-art' = $steamArt };
     Protect = @('games', 'save-backups', 'cache', 'angry_birds.txt', 'test-userdata', 'Documents') },
  @{ Folder = 'AngryBirdsTrilogy'; Id = 'ab_trilogy'; Build = Join-Path $trilogy 'out\build\abt-release'; Exe = 'ab_trilogy.exe'; Sdk = $true;
     Protect = @('game', 'ab_trilogy.toml', 'logs') },
  @{ Folder = 'AngryBirdsStarWars'; Id = 'ab_starwars'; Build = Join-Path $starWars 'out\build\absw-release'; Exe = 'ab_starwars.exe'; Sdk = $true;
     Protect = @('game', 'ab_starwars.toml', 'logs') }
)
# The launcher reports the VERSION file's version: it must be this release's, and the launcher is rebuilt with it.
$fileVersion = (Get-Content (Join-Path $launcher 'VERSION') -TotalCount 1).Trim()
if ($fileVersion -ne $Version) { throw "VERSION says ${fileVersion}, set it to $Version first" }
cmd /c "`"$(Join-Path $launcher 'build.bat')`"" | Out-Null
if ($LASTEXITCODE -ne 0) { throw 'the launcher did not build' }
$Out = [IO.Path]::GetFullPath($Out)
$top = Join-Path $Out "v$Version"
if (Test-Path $top) { throw "$top already exists" }
$tag = Join-Path $top 'packages'
New-Item -ItemType Directory -Force $tag | Out-Null
$stageRoot = Join-Path $Out "stage\v$Version"
if (Test-Path $stageRoot) { Remove-Item -Recurse -Force $stageRoot }

foreach ($p in $parts) {
  $exe = Join-Path $p.Build $p.Exe
  if (-not (Test-Path $exe)) { throw "$exe is missing: build it first" }
  if ($p.Sdk) {  # no developer aids in a release (AB_DEV_TOOLS)
    $text = [Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes($exe))
    $dev = [regex]::Matches($text, 'ABS?W?T?_DEV_\w+|ABS?W?T?_DUMP_IMAGE').Count
    if ($dev) { throw "$($p.Exe) is a developer build ($dev developer strings)" }
  }
  $stage = Join-Path $stageRoot $p.Folder
  New-Item -ItemType Directory -Force $stage | Out-Null
  Copy-Item $exe $stage
  if ($p.Sdk) { Copy-Item (Join-Path $sdk 'rexruntime.dll'), (Join-Path $sdk 'rexgpu-xenos.dll') $stage }
  foreach ($f in $crtFiles) { Copy-Item (Join-Path $crt $f) $stage }
  if ($p.Extra) { foreach ($k in $p.Extra.Keys) { Copy-Item $p.Extra[$k] (Join-Path $stage $k) -Recurse } }
  $zip = Join-Path $tag "$($p.Folder)-v$Version-windows-x64.zip"
  # Each part's own name pattern, as the launcher's updater picks it from the release.
  $pyArgs = @('-3', '-I', $tool, 'package', $stage, '--version', $Version, '--zip', $zip, '--asset', "$($p.Folder)-*-windows-x64.zip")
  foreach ($x in $p.Protect) { $pyArgs += @('--protect', $x) }
  if ($Earlier) { $pyArgs += @('--earlier', (Join-Path $Out "v$Earlier\packages\$($p.Folder)-v$Earlier-windows-x64.zip")) }
  $log = & py.exe @pyArgs 2>&1
  if ($LASTEXITCODE -ne 0) { $log | Select-Object -Last 10; throw "packaging $($p.Folder) failed" }
  "{0,-48} {1,6:N1} MB" -f (Split-Path -Leaf $zip), ((Get-Item $zip).Length / 1MB)
}

# The players' release: the launcher's zip with each game's zip unpacked into games\<id>, exactly as the
# launcher's own download would put it there (its update list included, so game updates work as usual).
$players = Join-Path $stageRoot 'players'
Expand-Archive (Join-Path $tag "AngryBirdsCollection-v$Version-windows-x64.zip") $players
foreach ($p in $parts | Where-Object { $_.Id }) {
  $unpack = Join-Path $stageRoot "unpack-$($p.Folder)"
  Expand-Archive (Join-Path $tag "$($p.Folder)-v$Version-windows-x64.zip") $unpack
  $dest = Join-Path $players "AngryBirdsCollection\games\$($p.Id)"
  New-Item -ItemType Directory -Force $dest | Out-Null
  Copy-Item (Join-Path $unpack "$($p.Folder)\*") $dest -Recurse
}
# Windows tar writes a real zip with forward slashes and one top folder (playbook, release chapter).
$playersZip = Join-Path $top "AngryBirdsCollection-v$Version-windows-x64.zip"
tar.exe -a -c -f $playersZip -C $players AngryBirdsCollection
if ($LASTEXITCODE -ne 0) { throw 'the players zip failed' }
"{0,-48} {1,6:N1} MB  (players: the launcher and both programs)" -f (Split-Path -Leaf $playersZip), ((Get-Item $playersZip).Length / 1MB)

# SHA256SUMS.txt: Windows line ends, two spaces between hash and name (each package call above wrote one for its own
# zip only, so the packages one is written again for all three).
function Write-Sums($folder) {
  $sums = Get-ChildItem $folder -Filter '*.zip' | Sort-Object Name | ForEach-Object {
    "$((Get-FileHash $_.FullName -Algorithm SHA256).Hash.ToLower())  $($_.Name)"
  }
  [IO.File]::WriteAllText((Join-Path $folder 'SHA256SUMS.txt'), (($sums -join "`r`n") + "`r`n"))
  "SHA256SUMS.txt in $(Split-Path -Leaf $folder): $(@($sums).Count) zips"
}
Write-Sums $tag
Write-Sums $top
"SDK pair: rexruntime $((Get-FileHash (Join-Path $sdk 'rexruntime.dll')).Hash.Substring(0, 8)), " +
  "rexgpu-xenos $((Get-FileHash (Join-Path $sdk 'rexgpu-xenos.dll')).Hash.Substring(0, 8))"
"Next: every zip through VirusTotal before the repository goes public."
