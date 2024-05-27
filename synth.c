#include <stdio.h>
#include <stdlib.h>
#include "synth.h"

void synth_register_note(synth* s, float freq, float amp, note_event event, int* id) {
  if (event == NOTE_ON) {
    fori(s->poly_count) {
      note* n = &s->notes[i];
      if (n->free) {
	n->free = 0;
	n->freq = freq;
	n->amp = amp;
	n->reset = 1;
	n->end = 0;
	*id = i;
	return;
      }
    }
  }

  if (event == NOTE_OFF) {
    note* n = &s->notes[*id];
    n->free = 1;
    n->end = 1;
  }
}

float synth_play(synth* s) {
  int finished = 1;
  float sample = 0;
  fori(s->poly_count) {
    note* n = &s->notes[i];
    finished &= n->free;
    if (!n->free) {
      sample += s->osc(n->freq, n->amp, i, &n->reset);
      n->len_samples--;
      if (!n->len_samples) {
	n->free = 1;
      }
    }
  }
  return sample;
}

synth* new_synth(int poly_count, float(*osc)(float, float, int, int*)) {
  synth* s = malloc(sizeof(synth));
  s->poly_count = poly_count;
  s->notes = malloc(s->poly_count*sizeof(note));
  s->osc = osc;

  fori(s->poly_count) {
       s->notes[i].free = 1;
       s->notes[i].reset = 0;
  }
  return s;
}

float osc(float freq, int note_index, int* reset) {
  int amp[8] = {1, 10, 100};
  if (*reset) puts("reset!");
  //printf("note: %d\n", note_index);
  return amp[note_index];
}
/*
int main() {

  synth* s = new_synth(3, osc);
  synth_register_note(s, 220.0f, 4);
  synth_register_note(s, 440.0f, 4);

  for(int j = 0;; j++) {
    if (j == 2) {
      synth_register_note(s, 777.0f, 4);
    }

    float sample = synth_play(s);

    printf("final sample: %f\n", sample);

    if (j == 10) break;
  }
}
*/
