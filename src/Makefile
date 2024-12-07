cadence.a: *.c
	gcc -O2 context.c gen.c synth.c effect.c util.c fft.c midi.c reverb.c \
		extra/linalloc.c include/stb/stb_vorbis.c -Iinclude -lm -c -fPIC -ggdb
	ar rcs cadence.a context.o gen.o synth.o effect.o util.o fft.o midi.o linalloc.o stb_vorbis.o reverb.o
	rm context.o gen.o synth.o effect.o util.o fft.o midi.o linalloc.o stb_vorbis.o
