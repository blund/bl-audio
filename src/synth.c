/*****************************************************************
 *  synth.c                                                       *
 *  Created on 30.05.24                                           *
 *  By Børge Lundsaunet                                           *
 *****************************************************************/



#include <stdio.h>

#include "synth.h"


// Helper functions to deal with flags
void set_flag(note_t* note, note_flags flag) {
  note->flags |= (1 << flag);
}

void unset_flag(note_t* note, note_flags flag) {
  note->flags &= ~(1 << flag);
}

int check_flag(note_t* note, note_flags flag) {
  return note->flags & (1 << flag);
}


void synth_register_note(synth_t* s, int midi_note, float amp, note_event event) {
  if (event == NOTE_ON) {
    fori(s->poly_count) {
      note_t* n = &s->notes[i];
      if (n->midi_note == midi_note) {
	//puts("warning: got repeat note");
	unset_flag(n, NOTE_FREE);
      }

      if (check_flag(n, NOTE_FREE)) {
	set_flag(n, NOTE_RESET);
	unset_flag(n, NOTE_FREE);
	unset_flag(n, NOTE_RELEASE);

	n->midi_note = midi_note;
	n->amp  = amp;
	return;
      }
    }
  }

  if (event == NOTE_OFF) {
    fori(s->poly_count) {
      note_t* n = &s->notes[i];
      if (n->midi_note == midi_note) {
	set_flag(n, NOTE_RELEASE);
      }
    }
    //puts("warning: got note off on non-existant key");
  }
}

float play_synth(cadence_ctx* ctx, synth_t* s) {
  float sample = 0;
  fori(s->poly_count) {
    note_t* n = &s->notes[i];

    if (!check_flag(n, NOTE_FREE)) {
      sample += s->osc(ctx, s, i, &s->notes[i]);
    }
  }
  return sample;
}

synth_t* new_synth(cadence_ctx* ctx, int poly_count, osc_t osc){
  synth_t* s = ctx->alloc(sizeof(synth_t));
  s->notes   = ctx->alloc(sizeof(note_t)*poly_count);
  s->poly_count = poly_count;
  s->osc = osc;

  fori(s->poly_count) {
    note_t* n = &s->notes[i];

    set_flag(n, NOTE_FREE);
    unset_flag(n, NOTE_RESET);
    unset_flag(n, NOTE_RELEASE);
  }

  return s;
}
