# package-one-s.ps1 - package Eglantine for the Xbox One S (Dev Mode) on the Dell. Runs ON the Dell.
#
#   powershell -File tools\blazing\package-one-s.ps1 -Version 0.1.0.0 [-Game]
#
# Stages the build with NXbox's tools\nxbox\package_uwp.py (--stage-only, kind cpu), then:
#   - adds Mesa's Dozen (vulkan_dzn.dll with the shadps4-xbox DXBC back end), the Vulkan renderer
#     the One S needs (its D3D12 takes Shader Model 5.1 only, so NXbox's OpenGL on d3d12 cannot run);
#   - -Game adds the paddle-test homebrew as game.nro: the app then runs the graphics game view
#     instead of the headless JIT smoke test (boot.nro);
#   - gives the package its own identity (Blazing.Eglantine, publisher CN=Blazing shadPS4) and signs
#     it with the Dell's Dev Mode key, the one the shadps4-xbox packages use (C:\BlazingTools\keys).
# Output: out-one-s\Eglantine_<version>_x64.appx. Install with `xbox install` plus the VCLibs appx.
param([Parameter(Mandatory = $true)][string]$Version, [switch]$Game,
      [string]$Dzn = "C:\BlazingTools\build\mesa\src\build-dozen\src\microsoft\vulkan\vulkan_dzn.dll",
      [string]$MesaLicense = "C:\BlazingTools\build\mesa\src\docs\license.rst",
      [string[]]$Extra = @())
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$out = Join-Path $root "out-one-s"
$keys = "C:\BlazingTools\keys"
$pfx = Join-Path $keys "blazing-shadps4.pfx"
$passFile = Join-Path $keys "blazing-shadps4.pfx.pass"
function Say($m) { Write-Host ("[{0}] {1}" -f (Get-Date -Format HH:mm:ss), $m) }

$exe = Get-ChildItem (Join-Path $root "build-uwp") -Recurse -Filter eden-uwp.exe | Select-Object -First 1
if (-not $exe) { throw "no eden-uwp.exe under build-uwp: build first (tools\nxbox\build-uwp.cmd)" }
Remove-Item -Recurse -Force $out -ErrorAction SilentlyContinue
& python (Join-Path $root "tools\nxbox\package_uwp.py") --stage-only --kind cpu --exe $exe.FullName `
  --version $Version --output $out
if ($LASTEXITCODE -ne 0) { throw "staging failed ($LASTEXITCODE)" }
$stage = Join-Path $out "package"

if (-not (Test-Path $Dzn)) { throw "no ${Dzn}: build Dozen first (shadps4-xbox tools/dozen/build-dozen.ps1)" }
Copy-Item $Dzn $stage
Copy-Item $MesaLicense (Join-Path $stage "Notices\Mesa-LICENSE.rst")
Say ("vulkan_dzn.dll {0:N0} bytes, built {1}" -f (Get-Item $Dzn).Length, (Get-Item $Dzn).LastWriteTime)
# -Extra: more files into the package root, for Dell-only test packages (e.g. the Windows SDK's dxil.dll, which
# Dozen's DXIL path needs and the One S never uses). Never for a package meant for the Xbox.
foreach ($e in ($Extra -split ";" | Where-Object { $_ })) { Copy-Item $e $stage; Say ("extra file {0}" -f $e) }
if ($Game) {
  Copy-Item (Join-Path $root "homebrew\paddle-test\fixtures\game.nro") (Join-Path $stage "game.nro")
  Say "game.nro: the paddle-test homebrew (graphics game view)"
}

$manifestPath = Join-Path $stage "AppxManifest.xml"
$manifest = Get-Content $manifestPath -Raw
$manifest = $manifest.Replace('Name="NSPX.NXbox" Publisher="CN=NXbox"', 'Name="Blazing.Eglantine" Publisher="CN=Blazing shadPS4"')
$manifest = $manifest.Replace("<DisplayName>NXbox</DisplayName>", "<DisplayName>Eglantine</DisplayName>")
$manifest = $manifest.Replace("<PublisherDisplayName>NSPX</PublisherDisplayName>", "<PublisherDisplayName>Blazing</PublisherDisplayName>")
$manifest = $manifest.Replace('DisplayName="NXbox" Description="NXbox"', 'DisplayName="Eglantine" Description="Eglantine (Switch) for the Xbox One S"')
if (-not $manifest.Contains('Name="Blazing.Eglantine"')) { throw "manifest identity not replaced" }
Set-Content -Encoding UTF8 -Path $manifestPath -Value $manifest

$sdk = Get-ChildItem "${env:ProgramFiles(x86)}\Windows Kits\10\bin" -Directory | Where-Object { $_.Name -match '^10\.' } |
  Sort-Object Name -Descending | Select-Object -First 1
$makeappx = Join-Path $sdk.FullName "x64\makeappx.exe"
$signtool = Join-Path $sdk.FullName "x64\signtool.exe"
$appx = Join-Path $out ("Eglantine_{0}_x64.appx" -f $Version)
& $makeappx pack /o /d $stage /p $appx | Out-Host
if ($LASTEXITCODE -ne 0) { throw "makeappx failed ($LASTEXITCODE)" }
& $signtool sign /q /fd SHA256 /f $pfx /p (Get-Content $passFile -Raw) $appx
if ($LASTEXITCODE -ne 0) { throw "signtool failed ($LASTEXITCODE)" }
$hash = (Get-FileHash $appx -Algorithm SHA256).Hash.ToLower()
Say ("BUILT {0} ({1:N0} bytes) sha256 {2}" -f (Split-Path -Leaf $appx), (Get-Item $appx).Length, $hash)
