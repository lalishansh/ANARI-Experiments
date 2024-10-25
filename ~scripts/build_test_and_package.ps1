param(
	[string][Parameter(Mandatory)][ValidateSet("Windows", "Linux")]$OS,
	[string][Parameter(Mandatory)][ValidateSet("Debug", "Release")]$BuildType,

	[string]$VCPKG_ROOT="$env:VCPKG_ROOT",
	[string]$Arch = "x64",
	[string]$Generator = "Ninja",
	[string]$Compiler = "Clang"
)

$VCPKG_ROOT = $VCPKG_ROOT -replace '\\', '/'

$presetName = "$OS $Arch - $Generator - $Compiler @ $BuildType"

Push-Location "$PSScriptRoot/.."
$everything_cool = $true
try {
	if ($VCPKG_ROOT.Length -eq 0) {
		cmake --preset $presetName
	} else {
		cmake --preset $presetName -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
	}
	
	$installers_dir = "$PWD/!installers"
	Push-Location "$PWD/!build/$presetName"

	$everything_cool = $false
	
	cmake --build . --config $BuildType
	cmake --build . --target test
	
	cpack -C $BuildType #OR cpack .
	ctest -C $BuildType --rerun-failed --output-on-failure
	
	$everything_cool = $true
} catch { Write-Error $_ }
finally { if (-not $everything_cool) { Pop-Location } }

if ($everything_cool) {
	Remove-Item -Path "$installers_dir/_CPack_Packages" -Recurse -Force -ErrorAction Ignore
}

Pop-Location

return $installers_dir
