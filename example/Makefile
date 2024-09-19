

src = ../src

.PHONY: lib
lib:
	gcc program.c $(src)/cadence.a -I$(src) -I$(src)/include -lm -fPIC -shared -ggdb -o program.so

.PHONY: sdl
sdl:
	gcc sdl.c load_lib.c $(src)/cadence.a -I$(src) -I$(src)/include \
		-lm `pkg-config --cflags --libs sdl2` -ggdb \
		-o sdl
