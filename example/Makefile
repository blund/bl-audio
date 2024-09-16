

src = ../src

.PHONY: sdl-example
sdl-example:
	make -C ../src
	gcc sdl.c $(src)/cadence.a -I$(src) -I$(src)/include -lm `pkg-config --cflags --libs sdl2` -o sdl && ./sdl
