param([string]$Name = "dev", [string[]]$Extra = @(), [string]$Preset = "abt-release", [int]$Wait = 30, [string]$Shot = "")
# Starts the build windowed without taking focus, in a private run folder (its own copy of the exe and DLLs, its
# own user data), waits, and optionally captures its own window (PrintWindow, no focus change).
# Stop it with tools\stop.ps1 (by PID).
$root = Resolve-Path (Join-Path $PSScriptRoot '..')
$build = Join-Path $root "out\build\$Preset"
$run = Join-Path $root "runs\$Name"
New-Item -ItemType Directory -Force "$run\user", "$root\runs\shots" | Out-Null
Get-ChildItem $build -File | Where-Object { $_.Extension -in '.exe', '.dll' } | Copy-Item -Destination $run -Force
Remove-Item "$run\run.log" -ErrorAction SilentlyContinue
$env:SDL_WINDOW_ACTIVATE_WHEN_SHOWN = '0'; $env:SDL_WINDOW_ACTIVATE_WHEN_RAISED = '0'
$game = Join-Path $root 'assets'
$a = @("--game_data_root=`"$game`"", "--user_data_root=`"$run\user`"", '--log_file=run.log', '--fullscreen=false') + $Extra
$p = Start-Process -FilePath "$run\ab_trilogy.exe" -ArgumentList $a -WorkingDirectory $run -PassThru
$p.Id | Set-Content "$run\pid.txt"
Start-Sleep $Wait
if ($Shot -and -not $p.HasExited) { & "$PSScriptRoot\capture_window.ps1" -ProcessId $p.Id -OutFile "$root\runs\shots\$Shot.png" }
"pid=$($p.Id) alive=$(-not $p.HasExited)" + $(if ($p.HasExited) { " exit=0x{0:X}" -f $p.ExitCode } else { "" })
