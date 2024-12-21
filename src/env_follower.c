#include <math.h> // For fabsf(), log10f(), expf()

#include "context.h"

typedef struct {
  float attack;     // Attack time constant in seconds
  float release;    // Release time constant in seconds
  float sample_rate; // Sampling rate in Hz
  float envelope;   // Current logarithmic envelope value (in dB)
} env_follower_t;

// Initialize the envelope follower
env_follower_t* new_env_follower(cadence_ctx* ctx, float attack, float release) {
  env_follower_t* ef = ctx->alloc(sizeof(env_follower_t));

  float sample_rate = ctx->sample_rate;

  ef->attack = expf(-1.0f / (attack * sample_rate));
  ef->release = expf(-1.0f / (release * sample_rate));
  ef->sample_rate = sample_rate;
  ef->envelope = -96.0f; // Initialize to a very low value in dB

  return ef;
}

// Process a single sample
float apply_env_follower(env_follower_t* ef, float input) {
  // Calculate absolute value and convert to decibels (20 * log10(x))
  float abs_input = fabsf(input);
  float input_db = (abs_input > 1e-6f) ? 20.0f * log10f(abs_input) : -96.0f; // Avoid log(0) errors

  float normalized_input = (input_db + 96.0f) / 96.0f; // Map -96 dB to 0.0, 0 dB to 1.0
  if (normalized_input < 0.0f) normalized_input = 0.0f; // Clamp to 0.0

  // Choose attack or release smoothing
  if (normalized_input > ef->envelope) {
    // Attack phase
    ef->envelope += (normalized_input - ef->envelope) * (1.0f - ef->attack);
  } else {
    // Release phase
    ef->envelope += (normalized_input - ef->envelope) * (1.0f - ef->release);
  }

  return ef->envelope; // Return the envelope in dB
}
