cadence.a: *.c
	gcc -O2 gen.c synth.c include/stb/stb_vorbis.c -Iinclude -lasound -lm -c
	ar rcs cadence.a gen.o synth.o stb_vorbis.o
	rm gen.o synth.o stb_vorbis.o
