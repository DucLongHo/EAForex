@echo off
title Telegram Forwarder Bot
cls
python --version >nul 2>&1
if %errorlevel% neq 0 (
    echo [LOI] Python chua duoc cai dat hoac chua them vao PATH.
    pause
    exit
)

if exist requirements.txt (
    echo Dang cai dat thu vien tu requirements.txt...
    pip install -r requirements.txt --quiet --upgrade
) else (
    pip install telethon PySocks --quiet --upgrade
)

echo.
echo ================================================
echo    DANG KHOI DONG BOT CHUYEN TIEP...
echo ================================================
echo.

:: 3. Chay file bot
if exist forward_bot.py (
    python forward_bot.py
) else (
    echo [LOI] Khong tim thay file forward_bot.py trong thu muc nay.
)

echo.
echo Bot da dung lai. Kiem tra loi phia tren (neu co).
pause