
src = context.c gen.c synth.c util.c fft.c midi.c \
	reverb.c delay.c waveshaper.c filter.c \
	extra/linalloc.c include/stb/stb_vorbis.c

objs = $(patsubst %.c, %.o, $(notdir $(src)))

cadence.a: *.c
	echo $(objs)
	gcc -O2 $(src) -Iinclude -lm -c -fPIC -ggdb
	ar rcs cadence.a $(objs)
	rm $(objs)
