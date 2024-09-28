/*****************************************************************
 *  program.c                                                     *
 *  Created on 19.09.24                                           *
 *  By Børge Lundsaunet                                           *
 *****************************************************************/


#include <stdio.h>
#include <complex.h>

#include <GL/glew.h>
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#include "nuklear/nuklear.h"
#include "nuklear/nuklear_sdl_gl3.h"

#define STB_VORBIS_HEADER_ONLY
#include "stb/stb_vorbis.c"

#include "program.h"
#include "ui_elements.h"

#include "cadence.h"


// Forward declare oscs used by the synths
OSC(test_osc);
OSC(sample_player);


// Globals :)
cadence_ctx* ctx;
synth* s;
synth* test_sampler;
delay* d;
butlp_t* butlp;


int should_fft = 1;

// @TODO - Needs mutex :)
fft_t fft_obj;


fft_t fft_tst1;
fft_t fft_tst2;

// Global parameters for GUI
float filter_freq = 400;
float delay_s = 0.3f;
float feedback = 45.0f;


// Switch for which synth to play
enum {SYNTH, SAMPLER};
int op = SYNTH;


// Midi handler called by the platform layer
MIDI_EVENT(midi_event) {
  if (op == SYNTH) 
    synth_register_note(s, midi_note, 0.1, event_type);
  if (op == SAMPLER) 
    synth_register_note(test_sampler, midi_note, 0.1, event_type);
}


void test_fft_process(fft_t* obj) {
  if (!obj->samples_ready) return;
  float sr   = 44100;
  float hz_per_bin = sr/obj->size;

  int bins = obj->size/2;
  
  fori(bins) {
    multiply_bin(obj, i, 0.5);
  }
}

// Main program loop, generating samples for the platform layer
PROGRAM_LOOP(program_loop) {
  if (!ctx) {
    ctx = cadence_setup(44100);

    s     = new_synth(8, test_osc);
    d     = new_delay(ctx);
    butlp = new_butlp(ctx, 1000);

    new_fft(&fft_obj, 512);
    new_fft(&fft_tst1, 128);
    new_fft(&fft_tst2, 512);

    test_sampler = new_synth(8, sample_player);
  }

  process_gen_table(ctx);

  // Play synths
  float sample  = play_synth(ctx, s);
  sample       += play_synth(ctx, test_sampler);

  // Apply effects
  float filtered = apply_butlp(ctx, butlp, sample, filter_freq);
  float delay    = apply_delay(ctx, d, filtered, delay_s, feedback/100.0f);

  // Mix together stuff
  float mix = sample;// + delay;

  if (should_fft) {
    apply_fft(&fft_obj, mix);
    mix = apply_ifft(&fft_obj);
  }

  // Return as 16 bit int for the platform layer
  return (int16_t)16768*(mix);
}

// bands to display fft data
int band_count = 64;
float bands[64];

// Define the gui to draw each frame, called by the platform layer
DRAW_GUI(draw_gui) {
  /* GUI */

  if (nk_begin(ctx, "Cadence", nk_rect(0, 0, 800, 500), NK_WINDOW_TITLE|NK_WINDOW_NO_SCROLLBAR)) {

      // Select what synth to play
      nk_layout_row_static(ctx, 30, 100, 3);
      if (nk_option_label(ctx, "synth", op == SYNTH)) op = SYNTH;
      if (nk_option_label(ctx, "sampler", op == SAMPLER)) op = SAMPLER;

      nk_layout_row_static(ctx, 10, 0, 1);

      // Delay effect parameters
      nk_named_lin_slider(ctx, "delay", 0.01, 3, &delay_s);
      nk_named_lin_slider(ctx, "feedback", 0.1, 100, &feedback);
      nk_named_log_slider(ctx, "cutoff", 10, 20000, &filter_freq);
    
      nk_layout_row_static(ctx, 10, 0, 1);

      nk_layout_row_static(ctx, 30, 80, 1);
      if (nk_checkbox_label(ctx, "fft", &should_fft)) {
	if (should_fft)  puts("fft on");
	if (!should_fft) puts("fft off");
      }

      nk_layout_row_static(ctx, 60, 200, 1);


      // Display spectrum (fix scaling later)
      if (fft_obj.stage == FIRST_ITERATION_DONE) {
	int bins = 256;

	// Use geometric series to define how many bins to put in bands.
	// Uses "band_count" steps, and should add up to "bins";
	double r = 1.0055;   // Growth rate
	double a = 0.053; // Initial value

	int running_index = 0;
	int sum = 0;
	fori(band_count) {
	  //float bins_in_band = pow(k,i);
	  double bins_in_band = a*pow(r, i-1);
	  bands[i] = 0;
	  for(int j = 0; j < bins_in_band; j++) {
	    if (running_index+j > bins) continue;
	    bands[i] += cabs(fft_obj.pers[running_index+j]);
	  }
	  //printf("val: %ff\n", bands[i]);
	  running_index += (int)ceil(bins_in_band);
	}
      }
      nk_plot(ctx, NK_CHART_COLUMN, bands, 64, 0);

      // Button to recompile (and automatically reload) code :)
      nk_layout_row_static(ctx, 30, 80, 1);
      if (nk_button_label(ctx, "recompile")) {
	puts(" -- [recompiling program code]");
	system("make program &");
      }
    }
}

// Sample player osc, used by synth
OSC(sample_player) {
  static int init = 0;

  static sampler* sampler_arr[4];
  static adsr_t adsr_arr[4];

  // Initialize stuff
  if (!init) {
    // Initialize samplers;
    fori(4) sampler_arr[i] = new_sampler();
    sampler_set_sample(sampler_arr[0],"data/bowl1.ogg");
    sampler_set_sample(sampler_arr[1],"data/bowl2.ogg");
    sampler_set_sample(sampler_arr[2],"data/bowl3.ogg");
    sampler_set_sample(sampler_arr[3],"data/bowl4.ogg");

    
    // Initialize adsr curves
    fori(4) {
      set_line(ctx, &adsr_arr[i].atk, 0.05, 0.0, 1.0);
      set_line(ctx, &adsr_arr[i].rel, 0.5, 1.0, 0.0);
    }

    init = 1;
  }

  // Decide which sampler to play for this note
  int which = -1;
  switch(note->midi_note) {
  case 60:
    which = 0; break;
  case 62:
    which = 1; break;
  case 64:
    which = 2; break;
  case 65:
    which = 3; break;
  default:
    return 0;
  }

  // Handle note resets
  if (check_flag(note, NOTE_RESET)) {
    unset_flag(note, NOTE_RESET);
    stb_vorbis_seek_start(sampler_arr[which]->v);
    reset_adsr(&adsr_arr[which]);
  }

  // Manage adsr curve
  int release_finished = 0;
  float adsr_curve = adsr(&adsr_arr[which], check_flag(note, NOTE_RELEASE), &release_finished);
  if (release_finished) set_flag(note, NOTE_FREE);


  // Mix and play
  float sample = play_sampler(sampler_arr[which]);
  return sample * adsr_curve;
}


// Test osc to demonstrate polyphony, used by synth
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
