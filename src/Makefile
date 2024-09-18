cadence.a: *.c
	gcc -O2 context.c gen.c synth.c util.c include/stb/stb_vorbis.c -Iinclude -lm -c
	ar rcs cadence.a context.o gen.o synth.o util.o stb_vorbis.o
	rm context.o gen.o synth.o util.o stb_vorbis.o
