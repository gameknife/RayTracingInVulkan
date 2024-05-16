@echo off

mkdir build
cd build || goto :error

IF EXIST vcpkg.android (
	echo "vcpkg.android already exists."
	cd vcpkg.android || goto :error
) ELSE (
	git clone https://github.com/Microsoft/vcpkg.git vcpkg.android || goto :error
	cd vcpkg.android || goto :error
	git checkout 2024.03.25 || goto :error
	call bootstrap-vcpkg.bat || goto :error
)

vcpkg.exe install ^
	boost-exception:arm64-android ^
	boost-program-options:arm64-android ^
	boost-stacktrace:arm64-android ^
	glm:arm64-android ^
	imgui[core,freetype,android-binding,vulkan-binding]:arm64-android ^
	stb:arm64-android ^
	tinyobjloader:arm64-android ^
	curl:arm64-android ^
	|| goto :error

cd ..
cd ..

exit /b

:error
echo Failed with error #%errorlevel%.
exit /b %errorlevel%
