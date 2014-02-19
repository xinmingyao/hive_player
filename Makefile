LUALIB_MINGW=-I/usr/local/include -L/usr/local/bin -llua52
SDLLIB = -I./sdl2/include -L./sdl2 -lSDL2 
X264LIB = -I./x264 -L./x264 -lx264
FFMPEGLIB = -I./ffmpeg -L./ffmpeg -lavcodec 
SRC=\
src/player.c 

all :
	echo 'make win or make posix or make macosx'

win : hiveplayer.dll hiveplayer.lib
posix : hiveplayer.so
macosx: hiveplayer.dylib

hiveplayer.so : $(SRC)
	gcc -g -Wall --shared -fPIC -o $@ $^ $(SDLLIB) $(X264LIB) $(FFMPEGLIB) -lpthread

hiveplayer.dll : $(SRC)
	gcc -g -Wall -D_GUI --shared -o $@ $^ $(LUALIB_MINGW) $(SDLLIB) $(X264LIB) $(FFMPEGLIB) -L./lua52  -march=i686 -lws2_32


clean :
	rm -rf hiveplayer.dll hiveplayer.so hiveplayer.dylib hiveplayer.dylib.dSYM hiveplayer.lib
hiveplayer.lib :
	dlltool -d hiveplayer.def -l hiveplayer.lib
