<#
.SYNOPSIS
Receives the pinned Filament source/tool/link closure without executing a GPU consumer.
.DESCRIPTION
Run only during an assigned CPU handoff. Sources and artifacts remain in this
checkout's ignored build directory. Existing evidence is never deleted. Maximum
compiler concurrency is three; no tests, rendering or performance runs occur.
#>
[CmdletBinding()]
param(
    [ValidateSet('prepare', 'configure', 'build', 'tools', 'inspect', 'link', 'all')]
    [string]$Stage = 'all',
    [ValidateRange(1, 3)]
    [int]$Jobs = 3,
    [string]$VsInstallationPath = 'C:/Program Files/Microsoft Visual Studio/18/Community',
    [string]$ArtifactDirectory = 'build/phase15-qualification'
)

$ErrorActionPreference = 'Stop'
$PSNativeCommandUseErrorActionPreference = $false
$pin = 'd852e34cd5629a6f3851d613dcd17aa0ffd025a2'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$artifact = [IO.Path]::GetFullPath((Join-Path $repo $ArtifactDirectory))
$buildRoot = [IO.Path]::GetFullPath((Join-Path $repo 'build')) + [IO.Path]::DirectorySeparatorChar
if (-not $artifact.StartsWith($buildRoot, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Artifacts must be inside this checkout/build; external source/cache paths are forbidden.'
}
$cursor = $artifact
while ($cursor -and $cursor -ne $repo) {
    if ((Test-Path $cursor) -and ((Get-Item -LiteralPath $cursor).Attributes -band [IO.FileAttributes]::ReparsePoint)) {
        throw "Artifact ancestors must not be junctions or symlinks: $cursor"
    }
    $cursor = [IO.Path]::GetDirectoryName($cursor)
}
$source = Join-Path $artifact 'source'
$release = Join-Path $artifact 'release'
$logs = Join-Path $artifact 'logs'
New-Item -ItemType Directory -Force -Path $logs | Out-Null
$stamp = (Get-Date).ToUniversalTime().ToString('yyyyMMddTHHmmssfffZ')
$receipt = [Collections.Generic.List[object]]::new()
$lease = [IO.File]::Open((Join-Path $artifact 'qualification.lock'),
    [IO.FileMode]::OpenOrCreate, [IO.FileAccess]::ReadWrite, [IO.FileShare]::None)

function Invoke-Recorded([string]$Name, [string[]]$Command) {
    $log = Join-Path $logs "$stamp-$Name.log"
    $started = (Get-Date).ToUniversalTime().ToString('o')
    Write-Host "$Name -> $log"
    $exe = $Command[0]
    $arguments = @($Command | Select-Object -Skip 1)
    & $exe @arguments 2>&1 | Tee-Object -FilePath $log | Out-Host
    $code = $LASTEXITCODE
    $receipt.Add([pscustomobject]@{
        name = $Name; command = $Command; startedUtc = $started
        endedUtc = (Get-Date).ToUniversalTime().ToString('o')
        exitCode = $code; log = $log
    })
    @{ commands = @($receipt) } | ConvertTo-Json -Depth 5 | Set-Content (Join-Path $logs "$stamp-commands.json")
    if ($code -ne 0) { throw "$Name failed with exit $code; retained $log" }
}

function Assert-PinnedSource {
    $head = (& git -C $source rev-parse HEAD | Out-String).Trim()
    if ($LASTEXITCODE -ne 0 -or $head -ne $pin) { throw "Source must be exact $pin; found $head" }
    $dirty = @(& git -C $source status --porcelain --untracked-files=all)
    if ($LASTEXITCODE -ne 0 -or $dirty.Count) {
        throw 'Source differs from the pin. An adaptation requires its own reviewed qualification receipt.'
    }
    $imgui = Get-Content (Join-Path $source 'third_party/imgui/imgui.h')
    if (-not ($imgui -match '^#define IMGUI_VERSION\s+"1\.92\.5"')) {
        throw 'Expected release-matched ImGui 1.92.5.'
    }
}

function Enter-CompilerEnvironment {
    & (Join-Path $VsInstallationPath 'Common7/Tools/Launch-VsDevShell.ps1') `
        -VsInstallationPath $VsInstallationPath -Arch amd64 -HostArch amd64 `
        -SkipAutomaticLocation -NoLogo
    $compiler = (Get-Command cl.exe).Source
    if ($compiler -notmatch '[\\/]14\.51\.36231[\\/]bin[\\/]Hostx64[\\/]x64[\\/]cl\.exe$') {
        throw "Expected received toolset14.51.36231 x64, found $compiler"
    }
    $env:CMAKE_BUILD_PARALLEL_LEVEL = "$Jobs"
}

Push-Location $repo
try {
    if ($Stage -in @('prepare', 'all')) {
        if (-not (Test-Path $source)) {
            Invoke-Recorded 'source-clone' @('git', '-c', 'core.longpaths=true', 'clone',
                '--depth', '1', '--branch', 'v1.77.3', '--single-branch', '--config',
                'core.longpaths=true', 'https://github.com/google/filament.git', $source)
        }
    }
    Assert-PinnedSource
    if ($Stage -ne 'prepare') { Enter-CompilerEnvironment }
    if ($Stage -in @('configure', 'all')) {
        Invoke-Recorded 'configure-release' @('cmake', '-S', $source, '-B', $release,
            '-G', 'Ninja', '-DCMAKE_BUILD_TYPE=Release', '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON',
            '-DCMAKE_POLICY_DEFAULT_CMP0141=NEW', '-DCMAKE_MSVC_DEBUG_INFORMATION_FORMAT=Embedded',
            '-DUSE_STATIC_CRT=OFF', '-DFILAMENT_WINDOWS_CI_BUILD=ON',
            '-DFILAMENT_SHORTEN_MSVC_COMPILATION=OFF', '-DFILAMENT_SKIP_SAMPLES=ON',
            '-DFILAMENT_SKIP_SDL2=ON', '-DFILAMENT_BUILD_TESTING=OFF',
            '-DFILAMENT_SUPPORTS_VULKAN=ON', '-DFILAMENT_SUPPORTS_OPENGL=OFF',
            '-DFILAMENT_SUPPORTS_METAL=OFF', '-DFILAMENT_SUPPORTS_WEBGPU=OFF',
            '-DFILAMENT_ENABLE_MATDBG=OFF', '-DFILAMENT_ENABLE_FGVIEWER=OFF',
            '-DFILAMENT_ENABLE_EXCEPTIONS=ON', '-DFILAMENT_ENABLE_RTTI=ON', '-DFILAMENT_ENABLE_LTO=OFF')
    }
    if ($Stage -in @('build', 'all')) {
        Invoke-Recorded 'build-release' @('cmake', '--build', $release, '--target',
            'filament', 'matc', 'cmgen', 'resgen', 'imgui', '--parallel', "$Jobs")
    }
    if ($Stage -in @('tools', 'all')) {
        Invoke-Recorded 'matc-material-format-version' @((Join-Path $release 'tools/matc/matc.exe'), '--version')
        Invoke-Recorded 'matc-vulkan-material' @((Join-Path $release 'tools/matc/matc.exe'),
            '-p', 'desktop', '-a', 'vulkan', '-o', (Join-Path $artifact 'qualification.filamat'),
            (Join-Path $PSScriptRoot 'fixtures/qualification.mat'))
        Invoke-Recorded 'matc-metal-material' @((Join-Path $release 'tools/matc/matc.exe'),
            '-p', 'mobile', '-a', 'metal', '-o', (Join-Path $artifact 'qualification-metal.filamat'),
            (Join-Path $PSScriptRoot 'fixtures/qualification.mat'))
        Invoke-Recorded 'cmgen-dfg' @((Join-Path $release 'tools/cmgen/cmgen.exe'), '--quiet',
            '--size=16', "--ibl-dfg=$(Join-Path $artifact 'qualification-dfg.bin')")
        $resourceDirectory = Join-Path $artifact 'resources'
        New-Item -ItemType Directory -Force -Path $resourceDirectory | Out-Null
        Invoke-Recorded 'resgen-material-package' @((Join-Path $release 'tools/resgen/resgen.exe'),
            '--quiet', '--cfile', '--package=phase15_qualification', "--deploy=$resourceDirectory",
            (Join-Path $artifact 'qualification.filamat'))
    }
    if ($Stage -in @('link', 'all')) {
        $link = Join-Path $artifact 'link'
        Invoke-Recorded 'configure-link' @('cmake', '-S', (Join-Path $PSScriptRoot 'fixtures/link'),
            '-B', $link, '-G', 'Ninja', '-DCMAKE_BUILD_TYPE=Release',
            '-DCMAKE_POLICY_DEFAULT_CMP0141=NEW', '-DCMAKE_MSVC_DEBUG_INFORMATION_FORMAT=Embedded',
            "-DFILAMENT_SOURCE=$source", "-DFILAMENT_BUILD=$release")
        Invoke-Recorded 'build-link' @('cmake', '--build', $link, '--parallel', "$Jobs", '--verbose')
    }
    if ($Stage -in @('inspect', 'all')) {
        Invoke-Recorded 'compiler-macros' @('cl.exe', '/nologo', '/std:c++latest', '/EHsc', '/MD',
            '/P', "/Fi$(Join-Path $artifact 'compiler_macros.i')", "/I$(Join-Path $source 'libs/utils/include')",
            (Join-Path $PSScriptRoot 'fixtures/compiler_macros.cpp'))
        Invoke-Recorded 'artifact-inventory' @('python', (Join-Path $PSScriptRoot 'filament_inventory.py'),
            '--root', $artifact)
    }
} finally {
    Pop-Location
    $lease.Dispose()
}
