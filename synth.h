
#include "context.h"

typedef struct note {
  float freq;
  float amp;
  float len_samples;

  // @TODO - these should be bit flags
  int free; // denote whether this slot is free or not
  int end;
  int reset;
} note;

typedef struct synth {
  note* notes;
  int poly_count;
  float(*osc)(cae_ctx* ctx, struct synth* s, float freq, float amp, int index, int* reset);
} synth;

typedef enum note_event {
  NOTE_ON,
  NOTE_OFF,
} note_event;

synth* new_synth(int poly_count, float(*osc)(cae_ctx* ctx, synth* s, float, float, int, int*));
void   synth_register_note(synth* s, float freq, float amp, note_event event, int* id);
float  synth_play(cae_ctx* ctx, synth* s);
