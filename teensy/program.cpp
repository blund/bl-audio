
extern "C" {
#include "program.h"
}

OSC(test_osc);
synth_t synth;

void midi_event(int midi_note, note_event event_type) {
  synth_register_note(&synth, midi_note, 1.0, event_type);
}

void program_setup(program_state*s, int sample_rate) {
  // Set sample rate for the program
  s->ctx.sample_rate = sample_rate;

  new_synth(&synth, 8, test_osc);
}

int16_t program_loop(program_state* s) {
  float sample = play_synth(&s->ctx, &synth);

  // Make sure to get this translation logic correct :)
  return (int16_t)(sample*16768.0f);
}

OSC(test_osc) {
  // Variables global to all notes
  static int init = 0;

  // oscillators for each note
  static sine_t* sines[8];

  // adsr curve manager per note
  static adsr_t adsr_arr[8];

  // initialize variables (allocate synths, initialize them, set up release
  if (!init) {
    init = 1;

    // set up local oscillators
    fori(8) sines[i] = new_sine(ctx);

    // set up adsr
    fori(s->poly_count) set_line(ctx, &adsr_arr[i].atk, 0.05, 0.0, 1.0);
    fori(s->poly_count) set_line(ctx, &adsr_arr[i].rel, 0.9, 1.0, 0.0);
  }

  // handle note resets
  if (check_flag(note, NOTE_RESET)) {
    unset_flag(note, NOTE_RESET);

    sines[note_index]->t = 0;
    reset_adsr(&adsr_arr[note_index]);
  }

  // Calculate freq for and set for generator
  sines[note_index]->freq = mtof(note->midi_note);
  float sample           = gen_sine(ctx, sines[note_index]);

  // Handle adsr and check if note is finished
  int release_finished = 0;
  float adsr_curve = adsr(&adsr_arr[note_index], check_flag(note, NOTE_RELEASE), &release_finished);
  if (release_finished) set_flag(note, NOTE_FREE);

  return 0.4 * adsr_curve * sample;
}
