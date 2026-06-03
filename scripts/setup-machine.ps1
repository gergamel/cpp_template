param(
    [switch]$Ci,
    [string]$ToolsRoot = "C:\tools"
)

$ScriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$ManifestFile = Join-Path $ScriptRoot "..\tools\manifest.yaml"
$ManifestHelper = Join-Path $ScriptRoot "manifest-helper.py"

Write-Host "Tools root: $ToolsRoot"

function Get-ManifestCommands {
    if (Get-Command python3 -ErrorAction SilentlyContinue -or Get-Command python -ErrorAction SilentlyContinue) {
        $python = if (Get-Command python3 -ErrorAction SilentlyContinue) { "python3" } else { "python" }
        & $python $ManifestHelper commands $ManifestFile
    }
    else {
        Get-Content $ManifestFile | ForEach-Object {
            if ($_ -match '^\s*command:\s*(.+)$') {
                $matches[1].Trim()
            }
        }
    }
}

function Check-Program($cmd) {
    if (Get-Command $cmd -ErrorAction SilentlyContinue) {
        Write-Host "Found: $cmd"
    }
    else {
        Write-Warning "Missing required command: $cmd"
    }
}

if ($Ci) {
    Write-Host "CI mode: verify prerequisites in the current container or image."
    Write-Host "Please use a CI image with Visual Studio/Build Tools, CMake, and Python installed."
    exit 0
}

$commands = Get-ManifestCommands
foreach ($cmd in $commands) {
    Check-Program $cmd
}

Write-Host "If tools are not installed system-wide, add $ToolsRoot\bin to PATH or install Visual Studio Build Tools."
