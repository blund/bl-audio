
#include "context.h"

typedef struct note {
  float freq;
  float amp;

  int release_remaining_samples;

  int key; // used to identify note on/off-events

  // @TODO - these should be bit flags
  int free;    // denote whether this slot is free or not
  int release; // trigger synth release event
  int reset;   // reset internal parameters
} note;

struct synth; // Forward declare 'synth' for osc_t def
typedef float osc_t (cae_ctx* ctx, struct synth* s, int note_index, note* note);

typedef struct synth {
  osc_t* osc;

  int poly_count;
  note* notes;

  int release_total_samples;
} synth;

typedef enum note_event {
  NOTE_ON,
  NOTE_OFF,
} note_event;

synth* new_synth(int poly_count, osc_t* osc);
void   synth_register_note(synth* s, float freq, float amp, note_event event, int key);
float  play_synth(cae_ctx* ctx, synth* s);
