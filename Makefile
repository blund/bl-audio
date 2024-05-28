
.PHONY: cadence
cadence:
	gcc cadence.c synth.c -g -lasound -lm -o cadence && ./cadence
