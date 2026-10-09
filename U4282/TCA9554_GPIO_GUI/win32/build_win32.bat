@echo off
rem 在 Windows 上用 MinGW(-w64) 编译 (需要 gcc / windres 在 PATH 中)
windres app.rc -O coff -o app_res.o
gcc -O2 -finput-charset=UTF-8 -fexec-charset=UTF-8 -fwide-exec-charset=UTF-16LE ^
    -o TCA9554_GPIO_GUI.exe gpio_gui_win32.c app_res.o -mwindows -lcomctl32
echo built: TCA9554_GPIO_GUI.exe
