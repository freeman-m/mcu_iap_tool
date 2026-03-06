@echo off
chcp 65001 >nul
setlocal

REM ============================================
REM  MCU_IAP 一键打包发布脚本
REM ============================================

REM --- 配置路径（根据实际安装位置修改） ---
set QT_DIR=D:\Qt\Qt5.6.2\5.6\mingw49_32
set MINGW_DIR=D:\Qt\Qt5.6.2\Tools\mingw492_32
set PROJECT_DIR=%~dp0mcu_iap
set BUILD_DIR=%~dp0build_release
set OUTPUT_DIR=%~dp0release_pack

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
echo [2/5] 运行 qmake...
cd /d "%BUILD_DIR%"
"%QT_DIR%\bin\qmake.exe" "%PROJECT_DIR%\MCU_IAP.pro" -spec win32-g++ "CONFIG+=release"
if %errorlevel% neq 0 (
    echo [错误] qmake 失败！
    pause
    exit /b 1
)

REM --- 编译 ---
echo [3/5] 编译项目...
"%MINGW_DIR%\bin\mingw32-make.exe" -j4
if %errorlevel% neq 0 (
    echo [错误] 编译失败！
    pause
    exit /b 1
)

REM --- 准备发布目录 ---
echo [4/5] 准备发布目录...
if exist "%OUTPUT_DIR%" rd /s /q "%OUTPUT_DIR%"
mkdir "%OUTPUT_DIR%"
copy /y "%BUILD_DIR%\release\MCU_IAP.exe" "%OUTPUT_DIR%\"

REM --- windeployqt 收集依赖 ---
echo [5/5] 收集 Qt 依赖...
cd /d "%OUTPUT_DIR%"
"%QT_DIR%\bin\windeployqt.exe" MCU_IAP.exe
if %errorlevel% neq 0 (
    echo [错误] windeployqt 失败！
    pause
    exit /b 1
)

echo.
echo ========================================
echo   打包完成！
echo   输出目录: %OUTPUT_DIR%
echo ========================================
echo.
pause
