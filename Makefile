OBJS=subs.o init.o graphx.o lab3d.o setup.o adlibemu.o oldlab3d.o

%.o: %.c lab3d.h
	gcc -DUSE_OSS -O3 -Wall -fstrict-aliasing `sdl-config --cflags` -o $@ -c $<

default: $(OBJS)
	gcc -o ken $(OBJS) `sdl-config --libs` -lSDL_image -lGL -lGLU -lm

clean:
	rm -f $(OBJS)
