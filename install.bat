@echo off

set BUILD=x86_64-msvc,i686-msvc,x86_64-linux-gnu,armv7a-linux-gnueabihf,armv7a-linux-android,aarch64-linux-android

:loop
for /F " tokens=1* delims=," %%a in ("%BUILD%") do (
    cmake --preset=%%a
    cmake --build --preset=Release-%%a
    ninja -C build\%%a-clang install
    set BUILD=%%b
)

if defined BUILD goto :loop

pause
