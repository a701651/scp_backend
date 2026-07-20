@echo off
chcp 65001 >nul
cd /d "%~dp0"
echo 正在启动 k6 压测，请稍候...
.\k6.exe run logintest.js
pause