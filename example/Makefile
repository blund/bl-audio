

src = ../src

.PHONY: lib
lib:
	gcc program.c $(src)/cadence.a \
		-I$(src) -I$(src)/include -Iinclude \
		-lm -lGLEW -lGL -lGLU -lSDL2 \
		-fPIC -shared -ggdb -o program.so
	touch program.so.done

.PHONY: sdl
sdl:
	gcc sdl.c load_lib.c \
		-I$(src) -I$(src)/include -Iinclude \
		-lm -lGLEW -lGL -lGLU -lSDL2 \
		-ggdb \
		-o sdl
