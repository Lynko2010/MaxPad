@echo off
chcp 65001 >nul
setlocal
set "BIN=D:\gcc_gpp\x86_64-15.2.0-release-win32-seh-ucrt-rt_v13-rev1\mingw64\bin"

echo [*] 编译安装包资源...
"%BIN%\windres.exe" --codepage=65001 setup.rc -O coff -o setup.res.o
if errorlevel 1 ( echo [x] 资源编译失败 & exit /b 1 )

echo [*] 编译安装包...
"%BIN%\g++.exe" -std=c++17 -O2 -municode -mwindows -static -static-libgcc -static-libstdc++ setup.cpp setup.res.o -o MaxPadSetup.exe -lole32 -luuid -lshell32 -lshlwapi -luser32 -lgdi32
if errorlevel 1 ( echo [x] 编译失败 & exit /b 1 )

echo [ok] 生成 MaxPadSetup.exe
endlocal
