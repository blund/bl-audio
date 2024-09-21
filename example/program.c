/*****************************************************************
 *  program.c                                                     *
 *  Created on 19.09.24                                           *
 *  By Børge Lundsaunet                                           *
 *****************************************************************/


#include "malloc.h"



#include <GL/glew.h>
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_SDL_GL3_IMPLEMENTATION
#include "nuklear/nuklear.h"
#include "nuklear/nuklear_sdl_gl3.h"

// @NOTE - we must undef to include the header
#undef NK_IMPLEMENTATION
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
    butlp = new_butlp(ctx, 1000);
  }

  process_gen_table(ctx);

  float sample  = play_synth(ctx, s);
  float delayed = apply_delay(ctx, d, sample, 0.3, 0.6);
  delayed       = apply_butlp(ctx, butlp, delayed, 200);
  
  return (int16_t)16768*(sample+delayed);
}

DRAW_GUI(draw_gui) {
    /* GUI */
    if (nk_begin(ctx, "Cadence", nk_rect(50, 50, 230, 250),
		 NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|
		 NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE))
      {
	enum {EASY, HARD};
	static int op = EASY;
	static int property = 20;

	nk_layout_row_static(ctx, 30, 80, 1);
	if (nk_button_label(ctx, "button"))
	  printf("button pressed!\n");
	nk_layout_row_dynamic(ctx, 30, 2);
	if (nk_option_label(ctx, "easy", op == EASY)) op = EASY;
	if (nk_option_label(ctx, "hard", op == HARD)) op = HARD;
	nk_layout_row_dynamic(ctx, 22, 1);
	nk_property_int(ctx, "Compression:", 0, &property, 100, 10, 1);

	nk_layout_row_dynamic(ctx, 20, 1);
	nk_label(ctx, "background:", NK_TEXT_LEFT);
	nk_layout_row_dynamic(ctx, 25, 1);
	/*
	if (nk_combo_begin_color(ctx, nk_rgb_cf(bg), nk_vec2(nk_widget_width(ctx),400))) {
	  nk_layout_row_dynamic(ctx, 120, 1);
	  bg = nk_color_picker(ctx, bg, NK_RGBA);
	  nk_layout_row_dynamic(ctx, 25, 1);
	  bg.r = nk_propertyf(ctx, "#R:", 0, bg.r, 1.0f, 0.01f,0.005f);
	  bg.g = nk_propertyf(ctx, "#G:", 0, bg.g, 1.0f, 0.01f,0.005f);
	  bg.b = nk_propertyf(ctx, "#B:", 0, bg.b, 1.0f, 0.01f,0.005f);
	  bg.a = nk_propertyf(ctx, "#A:", 0, bg.a, 1.0f, 0.01f,0.005f);
	  nk_combo_end(ctx);
	}
	*/
      }
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
