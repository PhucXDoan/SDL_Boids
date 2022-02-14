@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

REM For finding the `.dll` files.
set PATH=W:\lib\SDL2\lib\x64\;W:\lib\SDL2_ttf\lib\x64\;%PATH%
