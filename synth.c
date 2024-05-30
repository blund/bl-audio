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

float synth_play(cae_ctx* ctx, synth* s) {
  int finished = 1;
  float sample = 0;
  fori(s->poly_count) {
    note* n = &s->notes[i];
    finished &= n->free;
    if (!n->free) {
      sample += s->osc(ctx, s, n->freq, n->amp, i, &n->reset);
      n->len_samples--;
      if (!n->len_samples) {
	n->free = 1;
      }
    }
  }
  return sample;
}

synth* new_synth(int poly_count, float(*osc)(cae_ctx* ctx, synth* s, float, float, int, int*)) {
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
