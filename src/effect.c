/*****************************************************************
 *  effect.c                                                      *
 *  Created on 18.09.24                                           *
 *  By Børge Lundsaunet                                           *
 *****************************************************************/


#include <math.h>

#include "util.h"

#include "effect.h"
#include "reverb.h"

// -- butterworth lowpass filter --
// Helper function to calculate coefficiients
static void butlp_set(cadence_ctx* ctx, butlp_t *filter, float cutoff_freq) {
    float pi = 3.141592653589793;
    float omega = 2 * pi * cutoff_freq / ctx->sample_rate;
    float sin_omega = sin(omega);
    float cos_omega = cos(omega);
    float alpha = sin_omega / sqrt(2.0);  // sqrt(2.0) for Butterworth

    // Coefficients calculation
    filter->b0 = (1 - cos_omega) / 2;
    filter->b1 = 1 - cos_omega;
    filter->b2 = (1 - cos_omega) / 2;
    filter->a0 = 1 + alpha;
    filter->a1 = -2 * cos_omega;
    filter->a2 = 1 - alpha;

    // Normalize coefficients
    filter->b0 /= filter->a0;
    filter->b1 /= filter->a0;
    filter->b2 /= filter->a0;
    filter->a1 /= filter->a0;
    filter->a2 /= filter->a0;

    // Initialize previous samples to zero
    filter->x1 = filter->x2 = 0.0;
    filter->y1 = filter->y2 = 0.0;

    filter->cutoff_freq = cutoff_freq;
}

butlp_t* new_butlp(cadence_ctx* ctx, float freq) {
  butlp_t* filter = ctx->alloc(sizeof(butlp_t));
  butlp_set(ctx, filter, freq);
  return filter;
}

float apply_butlp(cadence_ctx* ctx, butlp_t *filter, float input, float cutoff_freq) {
  if (filter->cutoff_freq != cutoff_freq) butlp_set(ctx, filter, cutoff_freq);

    float output = filter->b0 * input +
                    filter->b1 * filter->x1 +
                    filter->b2 * filter->x2 -
                    filter->a1 * filter->y1 -
                    filter->a2 * filter->y2;

    // Shift the past samples
    filter->x2 = filter->x1;
    filter->x1 = input;
    filter->y2 = filter->y1;
    filter->y1 = output;

    return output;
}

// -- delay --
delay_t* new_delay(cadence_ctx* ctx, int samples) {
  delay_t* d = ctx->alloc(sizeof(delay_t));
  d->buf_size = samples;
  d->buffer = ctx->alloc(d->buf_size * sizeof(float));

  // clear out buffer @NOTE - might be unecessary
  for (int i = 0; i < d->buf_size; i++) {
    d->buffer[i] = 0;
  }
  return d;
}

float delay_tap(cadence_ctx* ctx, delay_t* d, float delay_s) {
  float read_offset_target = delay_s * ctx->sample_rate;

  float last_offset = d->read_offset;

  d->read_offset = lerp(last_offset, read_offset_target, 8.0f/44100);
  

  // read from delay buffer

  float index = fabs(fmod((float)d->write_head - d->read_offset, d->buf_size));

  double integral;
  double fractional = modf(index, &integral);

  float sample_a  = d->buffer[(int)floor(index) % d->buf_size];
  float sample_b  = d->buffer[(int)ceil(index) % d->buf_size];

  d->delayed_sample = sample_a * fractional + sample_b * (1.0f - fractional);

  return d->delayed_sample;
}

void delay_write(cadence_ctx* ctx, delay_t* d, float sample, float fb_sig, float feedback) {
  // write back + feedback!
  d->buffer[d->write_head % d->buf_size] = sample + fb_sig * feedback;

  // increment delay
  d->write_head++;
}

// -- reverb --
reverb_t* new_reverb(cadence_ctx* ctx) {
  reverb_t* r = ctx->alloc(sizeof(reverb_t));
  reverbInitialize(&r->rb);
  reverbSetParam(&r->rb, 44100, 0.5, 8.0, 4.0, 18000, 0.05);

  return r;
}

void set_reverb(cadence_ctx* ctx, reverb_t *r, float wet_percent, float time_s, float room_size_s,
		float cutoff_hz, float pre_delay_s) {
  reverbSetParam(&r->rb, 44100, wet_percent, time_s, room_size_s, cutoff_hz, pre_delay_s);
}

float apply_reverb(cadence_ctx* ctx, reverb_t* r, float sample) {
  if (r->chunk_idx < 32) {
    r->chunk[r->chunk_idx] = sample;
    r->chunk_idx++;
  }

  if (r->chunk_idx == 32) {
    r->chunk_idx = 0;
    Reverb(&r->rb, r->chunk, r->chunk);
  }

  return r->rb.left_output[r->chunk_idx]; // + r->rb.right_output[r->chunk_idx] ;
}

