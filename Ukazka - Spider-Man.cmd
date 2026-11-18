@echo off
cd /d "%~dp0"
start "" "%~dp0app\FrameLab.exe" --video "%~dp0app\samples\combat.mp4" --input60
