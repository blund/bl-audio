#include <stdio.h>
#include <stdlib.h>
#include "synth.h"

void synth_register_note(synth* s, float freq, float amp, note_event event, int key) {
  if (event == NOTE_ON) {
    fori(s->poly_count) {
      note* n = &s->notes[i];
      if (n->free) {
	n->free = 0;
	n->freq = freq;
	n->amp = amp;
	n->reset = 1;
	n->release = 0;
	n->release_remaining_samples = s->release_total_samples;
	n->key = key;
	return;
      }
    }
  }

  if (event == NOTE_OFF) {
    fori(s->poly_count) {
      note* n = &s->notes[i];
      if (n->key == key) {
	n->release = 1;
	return;
      }
    }
    puts("error: got note off on non-existant key");
  }
}

float play_synth(cae_ctx* ctx, synth* s) {
  float sample = 0;
  fori(s->poly_count) {
    note* n = &s->notes[i];
    if (!n->free) {
      sample += s->osc(ctx, s, i, &s->notes[i]);
    }
  }
  return sample;
}

synth* new_synth(int poly_count, osc_t osc){
  synth* s = malloc(sizeof(synth));
  s->poly_count = poly_count;
  s->notes = malloc(s->poly_count*sizeof(note));
  s->osc = osc;

  fori(s->poly_count) {
       s->notes[i].free = 1;
       s->notes[i].reset = 0;
       s->notes[i].release = 0;
  }
  return s;
}
