
#include <stdio.h>
#include <math.h>

#include "nuklear/nuklear.h"

int nk_log_slider_float(struct nk_context *ctx, float min_log, float max_log, float *value, float min_linear, float max_linear) {
    // Step 1: Convert the current logarithmic value to a linear slider position
    float log_min = logf(min_log);
    float log_max = logf(max_log);

    // Ensure the current value is within bounds
    if (*value < min_log) *value = min_log;
    if (*value > max_log) *value = max_log;

    // Convert logarithmic value to linear slider position (within min_linear to max_linear)
    float log_value = logf(*value);
    float linear_position = (log_value - log_min) / (log_max - log_min) * (max_linear - min_linear) + min_linear;

    // Step 2: Pass the linear position as a pointer to nk_slider_float
    float original_linear_position = linear_position;  // Save original linear position for comparison
    int update = nk_slider_float(ctx, min_linear, &linear_position, max_linear, 0.001f);  // Pass pointer to be modified by the slider

    // Step 3: If the slider position changed, map the linear position back to the logarithmic scale
    if (linear_position != original_linear_position) {
        float new_log_value = log_min + (linear_position - min_linear) / (max_linear - min_linear) * (log_max - log_min);
        *value = expf(new_log_value);  // Update the value in logarithmic space
    }

    return update;
}

int nk_named_lin_slider(struct nk_context *ctx, char* name, float min, float max, float *val) {
	char value[30];
	char title[30];
	sprintf(value, "%.2f", *val);
	sprintf(title, "  %s:", name);

	nk_layout_row_static(ctx, 15, 100, 2);
	nk_label(ctx, title, NK_TEXT_LEFT);
	nk_label(ctx, value, NK_TEXT_LEFT);
	nk_layout_row_static(ctx, 22, 200, 1);
	return nk_slider_float(ctx, min, val, max, max/200);
}

int nk_named_log_slider(struct nk_context *ctx, char* name, float min, float max, float *val) {
	char value[30];
	char title[30];
	sprintf(value, "%.2f", *val);
	sprintf(title, "  %s:", name);


	nk_layout_row_static(ctx, 15, 100, 2);
	nk_label(ctx, title, NK_TEXT_LEFT);
	nk_label(ctx, value, NK_TEXT_LEFT);
	nk_layout_row_static(ctx, 22, 200, 1);
	return nk_log_slider_float(ctx, min, max, val, 0.0f, 1.0f);
}

