src = ../src

nuklear.o: nuklear.c
	gcc nuklear.c -Iinclude \
		-lm -lGLEW -lGL -lGLU -lSDL2 \
		-fPIC \
		-c

program: nuklear.o
	gcc program.c ui_elements.c $(src)/cadence.a nuklear.o \
		-I$(src) -I$(src)/include -Iinclude \
		-lm \
		-fPIC -shared -g \
		-o program.so
	touch program.so.done


.PHONY: example
example: nuklear.o
	gcc sdl.c load_lib.c nuklear.o \
		-I$(src) -I$(src)/include -Iinclude \
		-lm -lGLEW -lGL -lGLU -lSDL2 \
		-Wl,-R,'$$ORIGIN'\
		-ggdb \
		-o example

run: example program nuklear.o
	./example
