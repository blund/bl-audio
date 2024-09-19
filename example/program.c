
#include "malloc.h"

#include "program.h"
#include "cadence.h"

cadence_ctx* ctx;

synth* s;
delay* d;
butlp_t* butlp;

typedef struct program_data {
} program_data;

osc_t test_osc;

MIDI_EVENT(midi_event) {
  synth_register_note(s, midi_note, 0.1, event_type);
}

cadence_ctx* ctx = NULL;

PROGRAM_LOOP(program_loop) {
  if (!ctx) {
    ctx = cadence_setup(44100);

    s     = new_synth(8, test_osc);
    d     = new_delay(ctx);
    butlp = new_butlp(ctx, 2000);
  }

  process_gen_table(ctx);

  float sample  = play_synth(ctx, s);
  float delayed = apply_delay(ctx, d, sample, 0.3, 0.6);
  delayed       = apply_butlp(ctx, butlp, delayed, 200);
  
  return (int16_t)16768*(sample+delayed);
}

// Test osc to demonstrate polyphony
float test_osc(cadence_ctx* ctx, synth* s, int note_index, note* note) {
  // Variables global to all notes
  static int init = 0;

  // oscillators for each note
  static sine** sines;
  static phasor** phasors;

  // index for oscillator in gen table
  static int sine_i;

  // adsr curve manager per note
  static adsr_t* adsr_arr;

  // initialize variables (allocate synths, initialize them, set up release
  if (!init) {
    init = 1;

    // set up local oscillators
    sines   = malloc(s->poly_count * sizeof(sine));
    phasors = malloc(s->poly_count * sizeof(phasor));
    fori(s->poly_count) sines[i] = new_sine();
    fori(s->poly_count) phasors[i] = new_phasor();

    // set up adsr
    adsr_arr  = malloc(s->poly_count * sizeof(adsr_t));
    fori(s->poly_count) set_line(ctx, &adsr_arr[i].atk, 0.05, 0.0, 1.0);
    fori(s->poly_count) set_line(ctx, &adsr_arr[i].rel, 0.1, 1.0, 0.0);

    // and global ones :)
    sine_i = register_gen_table(ctx, GEN_SINE);
    ctx->gt[sine_i].p->freq = 3.0;
  }

  // handle note resets
  if (check_flag(note, NOTE_RESET)) {
    unset_flag(note, NOTE_RESET);

    sines[note_index]->t = 0;
    phasors[note_index]->value = 0;
    reset_adsr(&adsr_arr[note_index]);
  }

  // Update phasor
  phasors[note_index]->freq = 3.0f;
  float phase = gen_phasor(ctx, phasors[note_index]);
  note->amp *= 1.0f-phase;

  // Load global modulation from gen table
  float mod = ctx->gt[sine_i].val;

  // Calculate freq for and set for generator
  sines[note_index]->freq = mtof(note->midi_note);;

  int release_finished = 0;
  float adsr_curve = adsr(&adsr_arr[note_index], check_flag(note, NOTE_RELEASE), &release_finished);

  if (release_finished) set_flag(note, NOTE_FREE);

  float sample = 0.2 * adsr_curve * mod * gen_sine(ctx, sines[note_index]);
  return sample;
}
