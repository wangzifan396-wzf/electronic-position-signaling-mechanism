@echo off
chcp 65001 >nul
title SDM18 钻具控制系统

echo.
echo ═══════════════════════════════════════
echo   SDM18 钻具控制系统
echo ═══════════════════════════════════════
echo.
echo 正在启动服务器并打开浏览器...
echo.

cd /d "%~dp0"

:: Use pythonw.exe to run without console window
start "" pythonw server.pyw

echo.
echo 服务器已启动，浏览器将自动打开。
echo 如未打开，请手动访问: http://localhost:3000
echo.
echo 关闭此窗口不会停止服务器。
echo 要停止服务器，请在任务管理器中结束 pythonw.exe
echo.

timeout /t 6 >nul
