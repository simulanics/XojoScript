windres -O coff -i XGUI.rc -o XGUI.res
g++ -shared -o XGUI.dll XGUI.cpp XGUI.res -mwindows -Wl,--subsystem,windows -luxtheme -lcomctl32 -ldwmapi -lole32