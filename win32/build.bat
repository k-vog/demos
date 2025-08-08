@echo off
cl /nologo /fsanitize=address /Od /Zi /Fewin32_windows.exe win32_windows.c
cl /nologo /fsanitize=address /Od /Zi /Fewin32_threading.exe win32_threading.c