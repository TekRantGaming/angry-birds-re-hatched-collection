param([Parameter(Mandatory = $true)][int]$ProcessId, [Parameter(Mandatory = $true)][string]$OutFile)
# Dev helper: writes a minidump of a running process (all thread stacks) without stopping it.
Add-Type @'
using System;
using System.IO;
using System.Runtime.InteropServices;
public static class LiveDump {
    [DllImport("dbghelp.dll", SetLastError = true)]
    public static extern bool MiniDumpWriteDump(IntPtr process, uint pid, IntPtr file, uint type, IntPtr e, IntPtr u, IntPtr c);
}
'@
$p = Get-Process -Id $ProcessId
$fs = [IO.File]::Create($OutFile)
# MiniDumpWithThreadInfo | MiniDumpWithIndirectlyReferencedMemory | MiniDumpScanMemory
$ok = [LiveDump]::MiniDumpWriteDump($p.Handle, [uint32]$ProcessId, $fs.SafeFileHandle.DangerousGetHandle(), 0x1000 -bor 0x40 -bor 0x10 -bor 0x4, [IntPtr]::Zero, [IntPtr]::Zero, [IntPtr]::Zero)
$fs.Close()
"ok=$ok size=$((Get-Item $OutFile).Length)"
