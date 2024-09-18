
#include "context.h"

typedef struct butlp {
  float cutoff_freq;
  float a0, a1, a2;
  float b0, b1, b2;
  float x1, x2;  // Past input samples
  float y1, y2;  // Past output samples
} butlp;

butlp* new_butlp(cadence_ctx* ctx, float freq);
float apply_butlp(cadence_ctx* ctx, butlp *filter, float input, float cutoff_freq);

