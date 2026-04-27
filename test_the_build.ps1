git clean -dfx .
write-host "making build config"
$env:target_version="999.999.999.999"
cmake --preset msvc-x86 -DCPPWINRT_BUILD_VERSION=$env:target_version
if ($LASTEXITCODE -eq 0) {
        write-host "building"
	cmake --build build\msvc-x86 --config Release --target cppwinrt cppwinrt_fast_forwarder -j
}