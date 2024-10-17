/*****************************************************************
 *  context.c                                                     *
 *  Created on 18.09.24                                           *
 *  By Børge Lundsaunet                                           *
 *****************************************************************/


#include "context.h"
#include "extra/alloc.h"

cadence_ctx* cadence_setup(int sample_rate, void* alloc(uint64_t)) {
  cadence_ctx* ctx = alloc(sizeof(cadence_ctx));
  ctx->sample_rate = sample_rate;
  ctx->alloc       = alloc;
  gen_table_init(ctx);

  return ctx;
}
