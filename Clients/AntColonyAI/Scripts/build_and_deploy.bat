@echo off
REM Build and deploy script for Ant Colony AI
REM Usage: build_and_deploy.bat [Debug|Release] [--test]

SET CONFIG=%1
IF "%CONFIG%"=="" SET CONFIG=Release

SET SOLUTION_DIR=%~dp0..
SET ARENA_DIR=%SOLUTION_DIR%\..\..\Arena\Run_Windows

echo [1/3] Building %CONFIG% x64...
MSBuild "%SOLUTION_DIR%\AntColonyAI.sln" /p:Configuration=%CONFIG% /p:Platform=x64 /t:Build /v:minimal /warnaserror
IF ERRORLEVEL 1 (
    echo BUILD FAILED
    exit /b 1
)

echo [2/3] Deploying DLL to Arena...
copy /Y "%SOLUTION_DIR%\x64\%CONFIG%\AI_YuWeiTseng.dll" "%ARENA_DIR%\Players\"
IF ERRORLEVEL 1 (
    echo DEPLOY FAILED
    exit /b 1
)

echo [3/3] Build and deploy complete.

IF "%2"=="--test" (
    echo Starting Arena for solo survival test...
    start "" "%ARENA_DIR%\AntArena2_x64.exe"
)
