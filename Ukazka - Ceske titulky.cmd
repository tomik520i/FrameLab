@echo off
cd /d "%~dp0"
start "" "%~dp0app\FrameLab.exe" --video "%~dp0app\samples\ocr-demo.mp4" --subtitles --fg
