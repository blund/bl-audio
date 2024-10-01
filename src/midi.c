
#include "midi.h"
#include "stdio.h"

int read_midi_note_from_buf(unsigned char* buffer, int* index, Midi_Note* note) {

  int i = *index;
  
  while ((buffer[i] & 0xf0) != MIDI_NOTE_ON &&
	 (buffer[i] & 0xf0) != MIDI_NOTE_OFF) {
    i += 1;
    if (i > 32) return 0;
  }

  note->message = buffer[i] & 0xf0;
  note->channel = buffer[i] & 0x0f;
  note->note = buffer[i+1];
  note->vel  = buffer[i+2];

  *index = i+1;

  return 1;
}
