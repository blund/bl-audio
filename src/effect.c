
#include "math.h"
#include "malloc.h"

#include "effect.h"

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
  butlp_t* filter = malloc(sizeof(butlp_t));
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
delay* new_delay(cadence_ctx* ctx) {
  delay* d = (delay*)malloc(sizeof(delay)); // @NOTE - hardcoded max buffer size
  d->buf_size = 10*ctx->sample_rate;
  d->buffer = malloc(d->buf_size * sizeof(float));

  // clear out buffer @NOTE - might be unecessary
  for (int i = 0; i < d->buf_size; i++) {
    d->buffer[i] = 0;
  }
  return d;
}

float apply_delay(cadence_ctx* ctx, delay* d, float sample, float delay_ms, float feedback) {
  d->read_offset = delay_ms * ctx->sample_rate; // @NOTE - hardcoded samplerate
  // read from delay buffer
  uint32_t index = (d->write_head - d->read_offset) % d->buf_size;
  float delayed  = d->buffer[index];

  // write back + feedback!
  d->buffer[d->write_head % d->buf_size] = sample + delayed * feedback;

  // increment delay
  d->write_head++;
  return delayed;
}

