<#
.SYNOPSIS
    Builds a release .pdx and packages it into releaseBuilds/.

.DESCRIPTION
    Bumps buildNumber in Source/pdxinfo, builds both the device (ARM) and
    simulator (Release) binaries, verifies the packaged pdx, zips it into
    releaseBuilds/, and commits the pdxinfo bump.

.PARAMETER Version
    New version string for pdxinfo (e.g. 0.9.1). Omit to keep the current one.

.PARAMETER NoCommit
    Skip the automatic commit of the pdxinfo bump.

.EXAMPLE
    .\tools\release.ps1
    .\tools\release.ps1 -Version 0.9.1
#>
param(
    [string]$Version,
    [switch]$NoCommit
)

$ErrorActionPreference = 'Stop'

$GameName    = 'RunicRogues'
$repoRoot    = Split-Path -Parent $PSScriptRoot
$pdxinfoPath = Join-Path $repoRoot 'Source\pdxinfo'
$pdxDir      = Join-Path $repoRoot "$GameName.pdx"
$outputDir   = Join-Path $repoRoot 'releaseBuilds'

# Both builds write into Source/ and re-run pdc over it, so they must run
# sequentially — in parallel they race on the same .pdx output directory.
$presets = @('device', 'sim-release')

$utf8NoBom = New-Object Text.UTF8Encoding $false

function Write-Step([string]$message)
{
    Write-Host ""
    Write-Host "==> $message" -ForegroundColor Cyan
}

function Invoke-Checked([string]$exe, [string[]]$arguments, [string]$what)
{
    & $exe @arguments
    if ($LASTEXITCODE -ne 0)
    {
        throw "$what failed (exit code $LASTEXITCODE)."
    }
}

# ---------------------------------------------------------------- version bump

if (-not (Test-Path $pdxinfoPath))
{
    throw "Could not find $pdxinfoPath"
}

$originalPdxinfo = [IO.File]::ReadAllText($pdxinfoPath)

$buildMatch = [regex]::Match($originalPdxinfo, '(?m)^buildNumber=[ \t]*(\d+)')
if (-not $buildMatch.Success)
{
    throw "No 'buildNumber=' line in Source/pdxinfo."
}
$oldBuildNumber = [int]$buildMatch.Groups[1].Value
$newBuildNumber = $oldBuildNumber + 1

$versionMatch = [regex]::Match($originalPdxinfo, '(?m)^version=[ \t]*([^\r\n]*)')
if (-not $versionMatch.Success)
{
    throw "No 'version=' line in Source/pdxinfo."
}
$newVersion = $versionMatch.Groups[1].Value.Trim()

if ($Version)
{
    if ($Version -notmatch '^\d+(\.\d+){1,2}$')
    {
        throw "Version '$Version' must look like 1.2 or 1.2.3."
    }
    $newVersion = $Version
}

$updated = [regex]::Replace($originalPdxinfo, '(?m)^buildNumber=[^\r\n]*', "buildNumber=$newBuildNumber")
$updated = [regex]::Replace($updated, '(?m)^version=[^\r\n]*', "version=$newVersion")
[IO.File]::WriteAllText($pdxinfoPath, $updated, $utf8NoBom)

Write-Step "Release $newVersion (build $oldBuildNumber -> $newBuildNumber)"

# ---------------------------------------------------------------------- builds

$sim = Get-Process PlaydateSimulator -ErrorAction SilentlyContinue
if ($sim)
{
    $sim | Stop-Process -Force
    $sim | Wait-Process -Timeout 10 -ErrorAction SilentlyContinue
    Write-Host "Stopped the Playdate Simulator (it holds a lock on pdex.dll)."
}

try
{
    foreach ($preset in $presets)
    {
        if (-not (Test-Path (Join-Path $repoRoot "build\build-$preset\CMakeCache.txt")))
        {
            Write-Step "Configuring '$preset'"
            Invoke-Checked 'cmake' @('--preset', $preset) "cmake --preset $preset"
        }

        Write-Step "Building '$preset'"
        Invoke-Checked 'cmake' @('--build', '--preset', $preset) "cmake --build --preset $preset"
    }
}
catch
{
    [IO.File]::WriteAllText($pdxinfoPath, $originalPdxinfo, $utf8NoBom)
    Write-Host ""
    Write-Host "Build failed - reverted Source/pdxinfo to buildNumber=$oldBuildNumber." -ForegroundColor Yellow
    throw
}

# --------------------------------------------------------------- verify output

Write-Step "Verifying the packaged pdx"

if (-not (Test-Path $pdxDir))
{
    throw "Builds finished but $pdxDir does not exist."
}

foreach ($binary in @(@{ File = 'pdex.bin'; From = 'device' }, @{ File = 'pdex.dll'; From = 'sim-release' }))
{
    if (-not (Test-Path (Join-Path $pdxDir $binary.File)))
    {
        throw "$($binary.File) is missing from the pdx - the '$($binary.From)' build did not produce it."
    }
}

# pdc only re-runs as a post-build step, so a skipped relink would silently
# leave the previous build's pdxinfo inside the pdx.
$builtInfo    = [IO.File]::ReadAllText((Join-Path $pdxDir 'pdxinfo'))
$builtBuild   = [regex]::Match($builtInfo, '(?m)^buildNumber=[ \t]*(\d+)').Groups[1].Value
$builtVersion = [regex]::Match($builtInfo, '(?m)^version=[ \t]*([^\r\n]*)').Groups[1].Value.Trim()

if ($builtBuild -ne "$newBuildNumber" -or $builtVersion -ne $newVersion)
{
    throw "The pdx is stale: it has version=$builtVersion buildNumber=$builtBuild, expected version=$newVersion buildNumber=$newBuildNumber. pdc did not re-run."
}

Write-Host "pdex.bin + pdex.dll present, pdxinfo reports $builtVersion ($builtBuild)."

# ------------------------------------------------------------------- packaging

Write-Step "Packaging"

if (-not (Test-Path $outputDir))
{
    New-Item -ItemType Directory -Path $outputDir | Out-Null
}

$zipPath = Join-Path $outputDir "$GameName$newVersion.pdx.zip"
if (Test-Path $zipPath)
{
    Remove-Item $zipPath -Force
    Write-Host "Overwrote the existing $(Split-Path -Leaf $zipPath)."
}

# Entries are written by hand rather than with Compress-Archive or
# CreateFromDirectory: on Windows PowerShell both emit nested entry names with
# backslash separators, which the zip spec forbids. macOS and Linux then unpack
# the archive as a pile of flat files literally named "RunicRogues.pdx\pdex.bin"
# instead of a folder tree.
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem
$rootPrefix = (Split-Path -Parent $pdxDir).TrimEnd('\').Length + 1
$archive = [IO.Compression.ZipFile]::Open($zipPath, [IO.Compression.ZipArchiveMode]::Create)
try
{
    foreach ($file in Get-ChildItem -LiteralPath $pdxDir -Recurse -File)
    {
        $entryName = $file.FullName.Substring($rootPrefix).Replace('\', '/')
        [IO.Compression.ZipFileExtensions]::CreateEntryFromFile(
            $archive, $file.FullName, $entryName, [IO.Compression.CompressionLevel]::Optimal) | Out-Null
    }
}
finally
{
    $archive.Dispose()
}

# ---------------------------------------------------------------------- commit

if (-not $NoCommit)
{
    if (-not (Get-Command git -ErrorAction SilentlyContinue))
    {
        Write-Host "git is not on PATH - skipping the commit." -ForegroundColor Yellow
    }
    else
    {
        $message = "Release v$newVersion (build $newBuildNumber)"
        & git -C $repoRoot commit -q -m $message -- 'Source/pdxinfo'
        if ($LASTEXITCODE -ne 0)
        {
            Write-Host "Could not commit Source/pdxinfo - commit it manually." -ForegroundColor Yellow
        }
        else
        {
            Write-Host "Committed: $message"
        }
    }
}

$sizeMb = [math]::Round((Get-Item $zipPath).Length / 1MB, 1)
Write-Host ""
Write-Host "Release ready: $zipPath ($sizeMb MB)" -ForegroundColor Green
