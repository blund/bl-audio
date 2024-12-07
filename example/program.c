/*****************************************************************
 *  program.c                                                     *
 *  Created on 19.09.24                                           *
 *  By Børge Lundsaunet                                           *
 *****************************************************************/


#include "gen.h"
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
#include "extra/linalloc.h"

// Forward declare oscs used by the synths
OSC(test_osc);
OSC(sample_player);
OSC(granular);

// Globals :)
cadence_ctx* ctx;
synth_t* s;
synth_t* grain_sampler;
synth_t* test_sampler;
delay_t* d;
reverb_t* r;
butlp_t* butlp;

int should_fft = 0;

// @TODO - Needs mutex :)
fft_t fft_obj;

fft_t fft_tst1;
fft_t fft_tst2;

// Global parameters for GUI
float filter_freq = 400;
float delay_s = 0.3f;
float feedback = 45.0f;

int should_update_reverb_params = 0;
float reverb_mix = 65;
float reverb_time = 10.0;
float reverb_room = 3.0;
float reverb_cutoff = 8000.0f;
float reverb_pre = 0.03;

// Switch for which synth to play
enum {SYNTH, SAMPLER, GRAIN};
int op = GRAIN;


// Midi handler called by the platform layer
MIDI_EVENT(midi_event) {
  if (op == SYNTH) 
    synth_register_note(s, midi_note, 0.1, event_type);
  if (op == SAMPLER) 
    synth_register_note(test_sampler, midi_note, 0.1, event_type);
  if (op == GRAIN) 
    synth_register_note(grain_sampler, midi_note, 0.1, event_type);
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

void high_pass_filter(complex float* buf, int fft_size, float cutoff_frequency, float sample_rate) {
    int cutoff_bin = (int)(cutoff_frequency * fft_size / sample_rate);
    float transition_width = 3;  // Transition width in bins for smoothing

    // Apply smooth transition for positive frequencies
    for (int i = 0; i < cutoff_bin; i++) {
        buf[i] *= 0.0f;
        buf[fft_size - i] *= 0.0f;
    }
}


float limit = 0.99;
float damp   = 0.99;
float fft_cutoff   = 500.0;

uint64_t mem[1024*1024*4];
linear_allocator_t al = {
  .base = mem,
  .size = 1024*1024*4,
  .current_offset = 0,
};

LINALLOC(linalloc) {
  return _linalloc(&al, size);
}


int sample_counter = 0;
int control_samples = 44100;

// Main program loop, generating samples for the platform layer
int mod1;
PROGRAM_LOOP(program_loop) {
  if (!ctx) {
    ctx = cadence_setup(44100, linalloc);

    mod1 = register_gen_table(ctx, GEN_SINE);
    ctx->gt[mod1].s->freq = 0.2f;

    s = new_synth(ctx, 8, test_osc);
    d     = new_delay(ctx, 10*ctx->sample_rate);
    butlp = new_butlp(ctx, 1000);

    r = new_reverb(ctx);

    new_fft(&fft_obj, 1024*2);
    new_fft(&fft_tst1, 120);
    new_fft(&fft_tst2, 512);

    test_sampler = new_synth(ctx, 8, sample_player);
    grain_sampler = new_synth(ctx, 8, granular);
  }

  process_gen_table(ctx);

  // Play synths
  float sample  = play_synth(ctx, s);
  sample       += play_synth(ctx, test_sampler);
  sample       += play_synth(ctx, grain_sampler);

  // Apply effects
  float filtered = apply_butlp(ctx, butlp, sample, filter_freq);

  float d_mod = 0.005f * ctx->gt[mod1].val;
  float delay    = apply_delay(ctx, d, filtered, delay_s + d_mod, feedback/100.0f);

  // Mix together stuff
  float mix = sample + delay;

  if (should_update_reverb_params) {
    set_reverb(ctx, r, reverb_mix, reverb_time, reverb_room, reverb_cutoff, reverb_pre);
    should_update_reverb_params = 0;
  }

  mix = apply_reverb(ctx, r, mix);

  if (should_fft) {

    //apply_hann_window(fft_obj.in_buf, 512);
    apply_fft(&fft_obj, mix);

    if (fft_obj.samples_ready) {
      high_pass_filter(fft_obj.buf, fft_obj.size, 1000, 44100);
    }

    mix = apply_ifft(&fft_obj);
  }

  // Return as 16 bit int for the platform layer
  return (int16_t)16768*(mix);
}

// bands to display fft data
int band_count = 64;
float bands[64];


int gui_init;
float samples[1000];
int sample_count = 100;

void display_waveform(struct nk_context *ctx, float *samples, int sample_count, int index_position) {
    struct nk_command_buffer *canvas = nk_window_get_canvas(ctx);
    struct nk_rect bounds = nk_layout_widget_bounds(ctx);

    // Set the range and scale for waveform display
    float x_scale = bounds.w / (float)sample_count; // Horizontal scale based on sample count
    float y_scale = bounds.h / 2.0f;                // Vertical scale (waveform centered)

    // Draw the waveform using a series of lines
    for (int i = 0; i < sample_count - 1; i++) {
        // Calculate x and y positions for the line segment
        float x0 = bounds.x + i * x_scale;
        float y0 = bounds.y + bounds.h / 2 - samples[i] * y_scale;  // Center the waveform vertically
        float x1 = bounds.x + (i + 1) * x_scale;
        float y1 = bounds.y + bounds.h / 2 - samples[i + 1] * y_scale;

        // Draw line segment between (x0, y0) and (x1, y1)
        nk_stroke_line(canvas, x0, y0, x1, y1, 1.0f, nk_rgb(100, 149, 237)); // Red line with 1px thickness
    }

    // Draw the index line at the specified position
    if (index_position >= 0 && index_position < sample_count) {
      float index_x = bounds.x + index_position * x_scale;  // Calculate x position of the index line

      // Draw a vertical line for the index position
      nk_stroke_line(canvas, index_x, bounds.y+10, index_x, bounds.y + bounds.h-10, 2.0f, nk_rgb(255, 165, 0)); // Blue line with 2px thickness
    }
}

int sample_index = 0;

// Define the gui to draw each frame, called by the platform layer
DRAW_GUI(draw_gui) {
  /* GUI */

  if (!gui_init) {
    for (int i = 0; i < 100; i++) {
      samples[i] = (float)sin(2 * 3.1415 * i / 10); // Generate a sine wave for demonstration
    }
  }

  if (nk_begin(ctx, "Cadence", nk_rect(0, 0, 500, 750), NK_WINDOW_TITLE)) {

      // Select what synth to play
      nk_layout_row_dynamic(ctx, 30, 5);
      if (nk_option_label(ctx, "synth",   op == SYNTH)) op = SYNTH;
      if (nk_option_label(ctx, "sampler", op == SAMPLER)) op = SAMPLER;
      if (nk_option_label(ctx, "grain",   op == GRAIN)) op = GRAIN;

      nk_spacing(ctx, 1);
      
      // Button to recompile (and automatically reload) code :)
      if (nk_button_label(ctx, "recompile")) {
	puts(" -- [recompiling program code]");
	system("make program &");
      }

      nk_layout_row_dynamic(ctx, 300, 2);
      if (nk_group_begin(ctx, "Group 1", NK_WINDOW_BORDER)) {
        nk_layout_row_dynamic(ctx, 30, 1); // One item per row
	// Delay effect parameters
	int res = nk_named_lin_slider(ctx, "reverb_mix", 0.0, 100.0f, &reverb_mix);
	res |= nk_named_lin_slider(ctx, "reverb_time", 0.0, 100.0f, &reverb_time);
	res |= nk_named_lin_slider(ctx, "reverb_room", 0.0, 100.0f, &reverb_room);
	res |= nk_named_lin_slider(ctx, "reverb_cutoff", 0.0, 20000.0f, &reverb_cutoff);
	res |= nk_named_lin_slider(ctx, "reverb_pre", 0.0, 1.0f, &reverb_pre);

	if (res) {
	  should_update_reverb_params = 1;
	}
    
        nk_group_end(ctx);
      }
      if (nk_group_begin(ctx, "Group 1", NK_WINDOW_BORDER)) {
        nk_layout_row_dynamic(ctx, 30, 1); // One item per row

	// Delay effect parameters
	nk_named_lin_slider(ctx, "delay", 0.01, 3, &delay_s);
	nk_named_lin_slider(ctx, "feedback", 0.1, 100, &feedback);
	nk_named_log_slider(ctx, "cutoff", 10, 20000, &filter_freq);
    
	nk_layout_row_static(ctx, 10, 0, 1);



        nk_group_end(ctx);
      }
      nk_layout_row_static(ctx, 30, 80, 1);
      if (nk_checkbox_label(ctx, "fft", &should_fft)) {
	if (should_fft)  puts("fft on");
	if (!should_fft) puts("fft off");
      }

      // Define a row layout with 2 columns
      nk_layout_row_dynamic(ctx, 50, 3);

      // Column 1: Waveform display
      if (nk_group_begin(ctx, "Waveform Group", NK_WINDOW_NO_SCROLLBAR)) {
	// Use the waveform display function within the group
	nk_layout_row_dynamic(ctx, 50, 1);  // One row for the waveform in the group
	display_waveform(ctx, samples, 100, sample_index);  // Draw waveform within this layout space
	nk_group_end(ctx);
      }
      sample_index++;
      if (sample_index>100) sample_index = 0;

      if (should_fft) {
	nk_layout_row_dynamic(ctx, 30, 1);

	nk_named_lin_slider(ctx, "limit", 0.0, 1.0, &limit);
	nk_named_lin_slider(ctx, "damp", 0.0, 1.0, &damp);
	nk_named_log_slider(ctx, "fft_cutoff", 10, 20000, &fft_cutoff);

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
      }

      
    }
}

// Sample player osc, used by synth
OSC(sample_player) {
  static int init = 0;

  static sampler_t* sampler_arr[4];
  static adsr_t adsr_arr[4];

  // Initialize stuff
  if (!init) {
    fori(4) sampler_arr[i] = new_sampler(ctx);

    // Initialize samplers;
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
    fori(s->poly_count) set_line(ctx, &adsr_arr[i].rel, 0.1, 1.0, 0.0);
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

  return 0.2 * adsr_curve * sample;
}

/*
// Test osc to demonstrate polyphony, used by synth
OSC(test_osc) {
  // Variables global to all notes
  static int init = 0;

  // oscillators for each note
  static sine_t[8] sines;
  static phasor_t[8] phasors;

  // index for oscillator in gen table
  static int sine_i;

  // adsr curve manager per note
  static adsr_t* adsr_arr;

  // initialize variables (allocate synths, initialize them, set up release
  if (!init) {
    init = 1;

    // set up local oscillators
    fori(s->poly_count) new_sine(&sines[i]);
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
  phasors[note_index]->freq = mtof(note->midi_note);
  float phase = 0.7*gen_phasor(ctx, phasors[note_index]);
  //note->amp *= 1.0f-phase;

  // Load global modulation from gen table
  float mod = ctx->gt[sine_i].val;

  // Calculate freq for and set for generator
  sines[note_index]->freq = mtof(note->midi_note);

  int release_finished = 0;
  float adsr_curve = adsr(&adsr_arr[note_index], check_flag(note, NOTE_RELEASE), &release_finished);

  if (release_finished) set_flag(note, NOTE_FREE);

  float sample = 0.2 * adsr_curve * phase;//;gen_sine(ctx, sines[note_index]);
  return sample;
}
*/

int rand_between(int min, int max) {
  float scale = rand()/(float)RAND_MAX; /* [0, 1.0] */
  return (int)min+scale*(max-min);      /* [min, max] */
}

#define GRAIN_SIZE (4410*2)

// Sample player osc, used by synth
OSC(granular) {
  typedef struct grain {
    int index;
    int end;
    int released;
    int retrigger;
    line_t atk;
    line_t rel;
  } grain_t;
  
  static int init = 0;

  static sampler_t* sampler;
  static adsr_t adsr_arr[8];

  // Buffer to hold the sample we granulate
  static float buffer[44100*2];

  // Initialize our grains (one per note);
  static grain_t grains[8] = {{.retrigger = 1},
			      {.retrigger = 1},
			      {.retrigger = 1},
			      {.retrigger = 1}};

  // Initialize stuff
  if (!init) {
    // Initialize samplers;
    sampler = new_sampler(ctx);
    sampler_set_sample(sampler, "data/bowl3.ogg");

    // Initialize adsr curves
    fori(8) {
      // Initialize grain curves
      set_line(ctx, &grains[i].atk, 0.15, 0.0, 1.0);
      set_line(ctx, &grains[i].rel, 0.01, 1.0, 0.0);

      // Initialize note adsrs
      set_line(ctx, &adsr_arr[i].atk, 0.05, 0.0, 1.0);
      set_line(ctx, &adsr_arr[i].rel, 0.3, 1.0, 0.0);
    }

    // Fill buffer with selection of samples (here at sample 120000, 44100 samples);
    sampler_seek(sampler, 100000);
    fori(44100*2) {
      buffer[i] = play_sampler(sampler);
    }

    init = 1;
  }

  // Handle note resets
  if (check_flag(note, NOTE_RESET)) {
    unset_flag(note, NOTE_RESET);
    reset_adsr(&adsr_arr[note_index]);
    grains[note_index].retrigger = 1;
  }

  // Start playing new grai
  if (grains[note_index].retrigger) {
    grains[note_index].retrigger = 0;

    printf("%d\n", grains[note_index].index);
    // Chose start for new grain
    grains[note_index].index = rand_int(0, 44100/2);
    grains[note_index].end = grains[note_index].index+4410;

    // Reset curves
    reset_line(&grains[note_index].atk);
    reset_line(&grains[note_index].rel);

  }


  float factor = mtof(note->midi_note)/mtof(60);

  float input_index = grains[note_index].index * factor;
  int index_int = (int)input_index;
  float frac = input_index - index_int;

 // Linear interpolation between two adjacent samples

  float sample = lerp(buffer[index_int], buffer[index_int + 1], frac);
  grains[note_index].index++;
  if (grains[note_index].index == grains[note_index].end) {
    grains[note_index].retrigger = 1;
  }
  
  // Manage grain curve
  float curve = line(&grains[note_index].atk, &grains[note_index].released);
  if (grains[note_index].released) {
    int grain_finished = 0;
    curve *= line(&grains[note_index].rel, &grain_finished);
    if (grain_finished) grains[note_index].retrigger = 1;
  }

  // Manage adsr curve
  int release_finished = 0;
  float adsr_curve = adsr(&adsr_arr[note_index], check_flag(note, NOTE_RELEASE), &release_finished);
  if (release_finished) set_flag(note, NOTE_FREE);

  // Mix and play
  return sample * curve * adsr_curve;
}
