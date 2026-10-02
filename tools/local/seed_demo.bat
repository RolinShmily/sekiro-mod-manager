@echo off
REM Thin wrapper so the demo seeder can be run from cmd. See seed_demo.ps1 for what it does.
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0seed_demo.ps1" %*
