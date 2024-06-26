@echo off

for %%X in (cmake.exe) do (set FOUND=%%~$PATH:X)
if defined FOUND (
set CMAKE=cmake
) ELSE (
set CMAKE="%CD%\build\vcpkg.windows\downloads\tools\cmake-3.27.1-windows\cmake-3.27.1-windows-i386\bin\cmake.exe"
)

set MSBUILD="%programfiles(x86)%\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\amd64\msbuild.exe"
cd build || goto :error
mkdir windows
cd windows || goto :error
%CMAKE% -D VCPKG_TARGET_TRIPLET=x64-windows-static -D CMAKE_TOOLCHAIN_FILE=../vcpkg.windows/scripts/buildsystems/vcpkg.cmake -G "Visual Studio 17 2022" -A "x64" ../.. || goto :error
%MSBUILD% gkNextRenderer.sln /target:Assets:Rebuild /p:Configuration=Release /verbosity:minimal /nologo || goto :error
cd ..
cd ..

exit /b


:error
echo Failed with error #%errorlevel%.
exit /b %errorlevel%