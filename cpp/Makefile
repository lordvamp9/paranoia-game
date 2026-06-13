CXX     = C:/Users/pc/tools/w64devkit/bin/g++.exe
RAYLIB  = C:/Users/pc/tools/raylib-6.0_win64_mingw-w64
CXXFLAGS = -O2 -std=c++17 -Wall -Wno-missing-field-initializers -I$(RAYLIB)/include
LDFLAGS  = -L$(RAYLIB)/lib -lraylib -lopengl32 -lgdi32 -lwinmm -static -static-libgcc -static-libstdc++

paranoia.exe: src/main.cpp src/*.h paranoia.rc
	C:/Users/pc/tools/w64devkit/bin/windres.exe paranoia.rc -O coff -o paranoia.res
	$(CXX) $(CXXFLAGS) -o paranoia.exe src/main.cpp paranoia.res $(LDFLAGS) -mwindows

debug: src/main.cpp src/*.h
	$(CXX) $(CXXFLAGS) -DDEV_CONSOLE -o paranoia-dev.exe src/main.cpp $(LDFLAGS)

clean:
	rm -f paranoia.exe paranoia-dev.exe
