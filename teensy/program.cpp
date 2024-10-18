
extern "C" {
#include "program.h"
}

#include "malloc.h"

OSC(test_osc);
synth_t* synth;
delay_t* del;

void midi_event(int midi_note, note_event event_type) {
  synth_register_note(synth, midi_note, 1.0, event_type);
}

void program_setup(program_state*s, int sample_rate) {
  // Set sample rate for the program
  s->ctx = cadence_setup(44100, &malloc);
  
  synth = new_synth(s->ctx, 8, test_osc);
  del   = new_delay(s->ctx, 1024*16);
}

int16_t program_loop(program_state* s) {
  float sample = play_synth(s->ctx, synth);

  // Add delay :)
  sample += apply_delay(s->ctx, del, sample, 0.3, 0.5);

  // Make sure to get this translation logic correct :)
  return (int16_t)(sample*16768.0f);
}


OSC(test_osc) {
  static int     init = 0;
  static sine_t* sines[8];
  static adsr_t  adsr_arr[8];

  if (!init) {
    fori(8) sines[i] = new_sine(ctx);

    // Set adsr curves
    fori(s->poly_count) set_line(ctx, &adsr_arr[i].atk, 0.05, 0.0, 1.0);
    fori(s->poly_count) set_line(ctx, &adsr_arr[i].rel, 0.3, 1.0, 0.0);

    init = 1;
  }

  // Handle note reset
  if (check_flag(note, NOTE_RESET)) {
    unset_flag(note, NOTE_RESET);

    // Reset sines
    sines[note_index]->t = 0;
    reset_adsr(&adsr_arr[note_index]);
  }

  // Generate tone
  sines[note_index]->freq = mtof(note->midi_note);

  // Handle note release
  int release_finished = 0;
  float adsr_curve = adsr(&adsr_arr[note_index], check_flag(note, NOTE_RELEASE), &release_finished);
  if (release_finished) set_flag(note, NOTE_FREE);

  float sample = adsr_curve*gen_sine(ctx, sines[note_index]);

  return 0.3*sample;
}

