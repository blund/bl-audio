
#include <math.h>
#include "../util.h"
#include "../effect.h"

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
