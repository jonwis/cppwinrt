git clean -dfx .
write-host "making build config"
cmake --preset msvc-x64
if ($LASTEXITCODE -eq 0) {
        write-host "building"
	cmake --build --preset msvc-x64-release -j
}