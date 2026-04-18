@echo off
title Telegram Forwarder Bot
cls

python --version >nul 2>&1
if %errorlevel% neq 0 (
    echo Python chua duoc cai dat hoac khong duoc them vao PATH.
    echo Dang tai va cai dat Python 3.12.3...    
    :: Tai file cai dat truc tiep tu trang chu Python bang curl
    curl -# -L -o python_installer.exe https://www.python.org/ftp/python/3.12.3/python-3.12.3-amd64.exe
    
    echo Dang cai dat Python an (Vui long doi vai phut)...
    :: Chay file cai dat an (/quiet), cai cho user hien tai (InstallAllUsers=0) va tu dong Add to PATH (PrependPath=1)
    start /wait python_installer.exe /quiet InstallAllUsers=0 PrependPath=1 Include_test=0
    
    echo Xoa file cai dat tam...
    del python_installer.exe
    
    echo.
    echo =========================================================================
    echo [QUAN TRONG] Cai dat Python hoan tat! 
    echo De he thong nhan dien lenh "python" moi, CMD can duoc khoi dong lai.
    echo VUI LONG TAT CUA SO NAY VA CHAY LAI FILE BAT DE TIEP TUC.
    echo =========================================================================
    pause
    exit
)

if exist requirements.txt (
    echo Dang cai dat thu vien tu requirements.txt...
    pip install -r requirements.txt --quiet --upgrade
) else (
    echo Dang cai dat thu vien mac dinh...
    pip install telethon PySocks --quiet --upgrade
)

echo.
echo ================================================
echo    DANG KHOI DONG BOT CHUYEN TIEP...
echo ================================================
echo.

:: Chay file bot
if exist forward_bot.py (
    python forward_bot.py
) else (
    echo [LOI] Khong tim thay file forward_bot.py trong thu muc nay.
)

echo.
echo Bot da dung lai. Kiem tra loi phia tren (neu co).
pause