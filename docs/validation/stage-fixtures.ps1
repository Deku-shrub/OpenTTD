param(
    [Parameter(Mandatory = $true)]
    [string]$Destination
)
$ErrorActionPreference = 'Stop'
New-Item -ItemType Directory -Path $Destination -Force | Out-Null
foreach ($fixture in @('ZoomStatic', 'ZoomStaticExtra', 'ZoomVehicles', 'ZoomCoverage')) {
    $fixtureDirectory = Join-Path $Destination $fixture
    New-Item -ItemType Directory -Path $fixtureDirectory -Force | Out-Null
    Get-ChildItem -LiteralPath (Join-Path $PSScriptRoot $fixture) -File | ForEach-Object {
        Copy-Item -LiteralPath $_.FullName -Destination $fixtureDirectory -Force
    }
    if ($fixture -ne 'ZoomStatic') {
        # NoAI resolves require() within each AI's directory.
        Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'ZoomStatic/main.nut') -Destination (Join-Path $fixtureDirectory 'base.nut') -Force
    }
}
