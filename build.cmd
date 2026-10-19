@echo off
chcp 65001 >nul
setlocal
set "GPP=D:\gcc_gpp\x86_64-15.2.0-release-win32-seh-ucrt-rt_v13-rev1\mingw64\bin\g++.exe"
set "WINDRES=D:\gcc_gpp\x86_64-15.2.0-release-win32-seh-ucrt-rt_v13-rev1\mingw64\bin\windres.exe"

if not exist "%GPP%" (
  echo [x] 找不到 g++: %GPP%
  exit /b 1
)

set "RES="
if exist "%WINDRES%" (
  if exist app.rc (
    echo [*] 编译图标资源...
    "%WINDRES%" app.rc -O coff -o app.res.o
    if not errorlevel 1 set "RES=app.res.o"
  )
) else (
  echo [!] 没找到 windres，跳过图标资源
)

echo [*] 编译主程序...
"%GPP%" -std=c++17 -O2 -municode -mwindows -static -static-libgcc -static-libstdc++ src\main.cpp src\apps.cpp src\config.cpp %RES% -o MaxPad.exe -ld2d1 -ldwrite -ldwmapi -lole32 -luuid -lgdi32 -luser32 -lshell32 -lshlwapi -lcomdlg32 -lwinmm
if errorlevel 1 ( echo [x] 编译失败 & exit /b 1 )
echo [ok] 生成 MaxPad.exe
endlocal
