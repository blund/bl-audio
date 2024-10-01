
#include "midi.h"
#include "stdio.h"

void read_midi_note_from_buf(char buffer[3], Midi_Note* note){
	note->message = buffer[0] & 0xf0;
	note->channel = buffer[0] & 0x0f;
	note->note = buffer[1];
	note->vel  = buffer[2];
}

void test() {
  printf("bam");
}
