$windowsBuildScript = Join-Path (Split-Path -Parent $PSScriptRoot) 'windows\scripts\build-windows.ps1'
& $windowsBuildScript @args
exit $LASTEXITCODE
