param(
    [string]$Executable = 'C:\ottd-build\Release\openttd.exe',
    [string]$GraphicsArchive = '',
    [string]$GraphicsSet = 'OpenGFX',
    [string]$Savegame = '',
    [switch]$NewMap,
    [string]$Blitter = '32bpp-anim',
    [string]$VideoDriver = 'win32',
    [int]$Width = 1280,
    [int]$Height = 800
)
$ErrorActionPreference = 'Stop'
$navigationRoot = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$navigationWork = Join-Path $navigationRoot 'build-smoothzoom'
$navigationExecutable = (Resolve-Path -LiteralPath $Executable).Path
$navigationBuild = Split-Path (Split-Path $navigationExecutable -Parent) -Parent
foreach ($navigationDirectory in @('lang', 'baseset', 'ai')) {
    New-Item -ItemType Directory -Path (Join-Path $navigationWork $navigationDirectory) -Force | Out-Null
}
Copy-Item -LiteralPath (Join-Path $navigationBuild 'lang\english.lng') -Destination (Join-Path $navigationWork 'lang\english.lng') -Force
Get-ChildItem -LiteralPath (Join-Path $navigationBuild 'baseset') -File | ForEach-Object {
    Copy-Item -LiteralPath $_.FullName -Destination (Join-Path $navigationWork "baseset\$($_.Name)") -Force
}
foreach ($navigationFixture in @('ZoomStatic', 'ZoomStaticExtra', 'ZoomVehicles', 'ZoomCoverage')) {
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot $navigationFixture) -Destination (Join-Path $navigationWork 'ai') -Recurse -Force
}
if ($GraphicsArchive) {
    Copy-Item -LiteralPath (Resolve-Path -LiteralPath $GraphicsArchive).Path -Destination (Join-Path $navigationWork 'baseset') -Force
}
if (!(Test-Path -LiteralPath (Join-Path $navigationWork 'baseset\opengfx-8.0.tar'))) {
    $navigationDefaultArchive = Join-Path ([Environment]::GetFolderPath('MyDocuments')) 'OpenTTD\baseset\opengfx-8.0-all.zip'
    if (!(Test-Path -LiteralPath $navigationDefaultArchive)) {
        throw 'Supply -GraphicsArchive with an installed opengfx-8.0.tar, or install OpenGFX 8.0 first.'
    }
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $navigationZip = [System.IO.Compression.ZipFile]::OpenRead($navigationDefaultArchive)
    try {
        [System.IO.Compression.ZipFileExtensions]::ExtractToFile($navigationZip.GetEntry('opengfx-8.0.tar'), (Join-Path $navigationWork 'baseset\opengfx-8.0.tar'), $true)
    } finally { $navigationZip.Dispose() }
}
if ($NewMap -and $Savegame) { throw 'Choose -NewMap or -Savegame, not both.' }
if (!$NewMap -and !$Savegame) { $Savegame = Join-Path $PSScriptRoot 'zoom-navigation.sav' }
if ($Savegame) { $Savegame = (Resolve-Path -LiteralPath $Savegame).Path }
$navigationConfig = Join-Path $navigationWork 'navigation-demo.cfg'
$navigationConfigText = @'
[misc]
fullscreen = false
sounddriver = null
musicdriver = null
[gui]
smooth_zoom = true
smooth_scroll = true
fly_navigation = true
fly_pan_speed = 120
fly_duration_scale = 100
fly_curve = 142
fly_zoom_out = 2
show_finances = false
autosave = 0
[game_creation]
starting_year = 2050
map_x = 8
map_y = 8
generation_seed = 12345
[graphicsset]
name = OpenGFX
'@
# OpenTTD's INI parser requires the first group header to start at byte zero.
[System.IO.File]::WriteAllText($navigationConfig, $navigationConfigText.Replace('name = OpenGFX', "name = $GraphicsSet"), [System.Text.UTF8Encoding]::new($false))
$navigationArguments = @('-c', $navigationConfig, '-x', '-X', '-r', "${Width}x${Height}", '-s', 'null', '-m', 'null', '-b', $Blitter, '-v', $VideoDriver, '-I', $GraphicsSet, '-g')
if ($Savegame) { $navigationArguments += $Savegame }
$navigationArgumentLine = ($navigationArguments | ForEach-Object { '"' + $_.Replace('"', '\"') + '"' }) -join ' '
$navigationProcess = Start-Process -FilePath $navigationExecutable -ArgumentList $navigationArgumentLine -WorkingDirectory $navigationWork -WindowStyle Normal -PassThru -RedirectStandardOutput (Join-Path $navigationWork "navigation-stdout.log") -RedirectStandardError (Join-Path $navigationWork "navigation-stderr.log")
$navigationProcess.Id | Set-Content -LiteralPath (Join-Path $navigationWork 'game.pid')
Write-Output 'Loaded navigation demo: use toolbar company menus to select the fixture company for station and vehicle lists. Unpause to move vehicles.'
$navigationProcess.WaitForExit()
