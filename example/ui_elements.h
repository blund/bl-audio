#include "nuklear/nuklear.h"

#include "util.h"
#include "effect.h"

int nk_named_log_slider(struct nk_context *ctx, char* name, float min, float max, float *val);
int nk_named_lin_slider(struct nk_context *ctx, char* name, float min, float max, float *val);
void draw_waveshaper(struct nk_context *ctx, waveshaper_t* ws);
