@echo off
if not exist bin mkdir bin
cl /W4 /Od /Zi /Fe:bin\win-resize.exe win-resize.c /Fd:bin\ /Fo:bin\
