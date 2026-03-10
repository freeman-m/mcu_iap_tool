@echo off
chcp 65001 >nul
setlocal

REM ============================================
REM  MCU_IAP 一键打包发布脚本
REM  生成单文件自解压 exe（WinRAR SFX）
REM ============================================

REM --- 配置路径（根据实际安装位置修改） ---
set QT_DIR=D:\Qt\Qt5.6.2\5.6\mingw49_32
set MINGW_DIR=D:\Qt\Qt5.6.2\Tools\mingw492_32
set WINRAR="C:\Program Files\WinRAR\WinRAR.exe"
set PROJECT_DIR=%~dp0mcu_iap
set BUILD_DIR=%~dp0build_release
set OUTPUT_DIR=%~dp0release_pack
set SFX_EXE=%~dp0WONZ_Tools.exe

REM --- 设置环境变量 ---
set PATH=%QT_DIR%\bin;%MINGW_DIR%\bin;%PATH%

echo.
echo ========================================
echo   MCU_IAP Release Build
echo ========================================
echo.

REM --- 清理旧的构建目录 ---
if exist "%BUILD_DIR%" (
    echo [1/5] 清理旧构建目录...
    rd /s /q "%BUILD_DIR%"
)
mkdir "%BUILD_DIR%"

REM --- qmake ---
echo [2/6] 运行 qmake...
cd /d "%BUILD_DIR%"
"%QT_DIR%\bin\qmake.exe" "%PROJECT_DIR%\MCU_IAP.pro" -spec win32-g++ "CONFIG+=release"
if %errorlevel% neq 0 (
    echo [错误] qmake 失败！
    pause
    exit /b 1
)

REM --- 编译 ---
echo [3/6] 编译项目...
"%MINGW_DIR%\bin\mingw32-make.exe" -j4
if %errorlevel% neq 0 (
    echo [错误] 编译失败！
    pause
    exit /b 1
)

REM --- 准备发布目录 ---
echo [4/6] 准备发布目录...
if exist "%OUTPUT_DIR%" rd /s /q "%OUTPUT_DIR%"
mkdir "%OUTPUT_DIR%"
copy /y "%BUILD_DIR%\release\MCU_IAP.exe" "%OUTPUT_DIR%\"

REM --- windeployqt 收集依赖 ---
echo [5/6] 收集 Qt 依赖...
cd /d "%OUTPUT_DIR%"
"%QT_DIR%\bin\windeployqt.exe" MCU_IAP.exe
if %errorlevel% neq 0 (
    echo [错误] windeployqt 失败！
    pause
    exit /b 1
)

REM --- WinRAR SFX 打包成单文件 ---
echo [6/6] 生成单文件 exe...
if exist "%SFX_EXE%" del /f "%SFX_EXE%"

REM 创建 SFX 配置文件
set SFX_CFG=%~dp0sfx_config.txt
(
echo Path=MCU_IAP
echo Setup=MCU_IAP.exe
echo TempMode
echo Silent=2
) > "%SFX_CFG%"

%WINRAR% a -r -sfx -z"%SFX_CFG%" -ep1 "%SFX_EXE%" "%OUTPUT_DIR%\*"
if %errorlevel% neq 0 (
    echo [错误] SFX 打包失败！
    del /f "%SFX_CFG%" 2>nul
    pause
    exit /b 1
)
del /f "%SFX_CFG%" 2>nul

echo.
echo ========================================
echo   打包完成！
echo   单文件输出: %SFX_EXE%
echo ========================================
echo.
pause
