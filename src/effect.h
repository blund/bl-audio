/*****************************************************************
 *  effect.h                                                      *
 *  Created on 18.09.24                                           *
 *  By Børge Lundsaunet                                           *
 *****************************************************************/



#ifndef CADENCE_EFFECT_H
#define CADENCE_EFFECT_H

#include "context.h"
#include "reverb.h"
#include "util.h"

typedef struct butlp_t {
  float cutoff_freq;
  float a0, a1, a2;
  float b0, b1, b2;
  float x1, x2;  // Past input samples
  float y1, y2;  // Past output samples
} butlp_t;

butlp_t* new_butlp(cadence_ctx* ctx, float freq);
float    apply_butlp(cadence_ctx* ctx, butlp_t *filter, float input, float cutoff_freq);

typedef struct delay_t {
  float*   buffer;
  uint32_t buf_size;
  uint32_t write_head;
  float read_offset;
  float last_offset;
  float delayed_sample;
} delay_t;
delay_t* new_delay(cadence_ctx* ctx, int samples);
float delay_tap(cadence_ctx* ctx, delay_t* d, float delay_s);
void delay_write(cadence_ctx* ctx, delay_t* d, float sample, float fb_sig, float feedback);

typedef struct reverb_t {
  reverbBlock rb;
  float chunk[32];
  int chunk_idx; // Used to keep track of filling the chunk
} reverb_t;
reverb_t* new_reverb(cadence_ctx* ctx);
void      set_reverb(cadence_ctx* ctx, reverb_t *r, float wet_percent, float time_s, float room_size_s,
		     float cutoff_hz, float pre_delay_s);
float     apply_reverb(cadence_ctx *ctx, reverb_t* r, float input);

typedef struct waveshaper_t {
  int   points_used;
  point points[16];
  float curves[15];
} waveshaper_t;

float apply_waveshaper(waveshaper_t* w, float a);
void waveshaper_del_point(waveshaper_t* w, int index);
void waveshaper_add_point(waveshaper_t* w, point p);
#endif
