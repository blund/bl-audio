
#include "context.h"
#include "malloc.h"

cadence_ctx* cadence_setup(int sample_rate) {
  cadence_ctx* ctx = malloc(sizeof(cadence_ctx));
  ctx->sample_rate = sample_rate;
  gen_table_init(ctx);

  return ctx;
}
