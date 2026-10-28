param([string]$Name = "dev")
# Stops only this run's own process (by the PID tools\run.ps1 saved) and confirms nothing of the run is left.
$run = Join-Path (Resolve-Path (Join-Path $PSScriptRoot '..')) "runs\$Name"
$id = [int](Get-Content "$run\pid.txt" -ErrorAction SilentlyContinue)
if ($id) { Stop-Process -Id $id -Force -ErrorAction SilentlyContinue }
if ($id) { Wait-Process -Id $id -Timeout 15 -ErrorAction SilentlyContinue }
$left = @(Get-CimInstance Win32_Process | Where-Object { $_.ExecutablePath -and $_.ExecutablePath.StartsWith($run, 'OrdinalIgnoreCase') })
foreach ($q in $left) { Stop-Process -Id $q.ProcessId -Force -ErrorAction SilentlyContinue }
"stopped pid=$id, leftover=$($left.Count)"
