@echo off
cd /d "%~dp0"
start "" "%~dp0app\FrameLab.exe" --video "%~dp0app\samples\gta.mp4" --fg --medium
