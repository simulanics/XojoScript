windres -O coff -i XGUI.rc -o XGUI.res
g++ -shared -o XGUI.dll XGUI.cpp XGUI.res -m64 -mwindows -Wl,--subsystem,windows -luxtheme -lcomctl32 -ldwmapi -lole32