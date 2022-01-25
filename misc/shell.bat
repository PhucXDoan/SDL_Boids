@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

REM For finding the `SDL2.dll`.
set PATH=W:\SDL2\lib\x64\;%PATH%
