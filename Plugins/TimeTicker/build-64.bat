g++ -s -shared -fPIC -m64 -O3 -o TimeTickerPlugin.dll TimeTickerPlugin.cpp -static -pthread -static-libgcc -static-libstdc++