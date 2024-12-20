
#include <stdio.h>
#include <math.h>
#include <time.h>

#include "nuklear/nuklear.h"
#include "../effect.h"

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


// Function to draw a Bezier curve with curvature in a fixed 150x150 region
/*
  void draw_bezier_curve(struct nk_context *ctx, point p1, point p2, float curvature, int steps, struct nk_color color) {
  // Fixed size of the square
  const int square_size = 150;

  // Drawing window
  if (nk_group_begin(ctx, "Bezier Square", NK_WINDOW_NO_SCROLLBAR)) {
  struct nk_rect bounds = nk_widget_bounds(ctx);

  // Transform points to fit within the 150x150 square
  float scale_x = bounds.w / square_size;
  float scale_y = bounds.h / square_size;

  point tp1 = { bounds.x + p1.x * scale_x, bounds.y + p1.y * scale_y };
  point tp2 = { bounds.x + p2.x * scale_x, bounds.y + p2.y * scale_y };

  // Calculate the control point using curvature
  //point tpc = calculate_control_point(tp1, tp2, curvature);

  // Draw the Bezier curve
  point prev = tp1;
  for (int i = 1; i <= steps; i++) {
  float t = (float)i / (float)steps;
  point current = bezier(tp1, tp2, curvature, t);

  nk_stroke_line(canvas,
  (short)prev.x, (short)prev.y,
  (short)current.x, (short)current.y,
  1.0f, color);
  prev = current;
  }

  // Draw control points as circles
  nk_fill_circle(canvas, nk_rect(tp1.x - 2, tp1.y - 2, 5, 5), nk_rgb(0, 255, 0));
  nk_fill_circle(canvas, nk_rect(tp2.x - 2, tp2.y - 2, 5, 5), nk_rgb(0, 255, 0));
  //nk_fill_circle(canvas, nk_rect(tpc.x - 2, tpc.y - 2, 5, 5), nk_rgb(0, 0, 255));

  nk_group_end(ctx);
  }
  }
*/

point scale_point(point p, struct nk_rect bounds) {
  point result;
  result.x = bounds.x + (p.x + 1.0f) * 0.5f * bounds.w;
  result.y = bounds.y + (1.0f - (p.y + 1.0f) * 0.5f) * bounds.h;
  return result;
}

// Helper to unscale a point from widget bounds to [-1, 1]
struct nk_vec2 unscale_vec(struct nk_vec2 p, struct nk_rect bounds) {
  // Unscale x from widget space to [-1, 1]
  p.x = ((p.x - bounds.x) / bounds.w) * 2.0f - 1.0f;

  // Unscale y from widget space to [-1, 1] (account for Nuklear's inverted y-axis)
  p.y = 1.0f - ((p.y - bounds.y) / bounds.h) * 2.0f;

  return p;
}

void draw_bounding_box(struct nk_context *ctx, struct nk_rect bounds) {
  struct nk_command_buffer *canvas = nk_window_get_canvas(ctx);
  struct nk_color border_color = nk_rgb(100, 100, 100); // Red color for the border
  float border_thickness = 0.1f;                    // Thickness of the border

  // Draw the rectangle
  nk_stroke_rect(canvas, bounds, 0.0f, border_thickness, border_color);
}

int rect_contains(struct nk_rect rect, struct nk_vec2 point) {
    return (point.x >= rect.x && point.x <= rect.x + rect.w &&
            point.y >= rect.y && point.y <= rect.y + rect.h);
}

struct nk_vec2 clamp_to_rect(struct nk_vec2 p, struct nk_rect bounds) {
  // Clamp the x-coordinate
  if (p.x < bounds.x) {
    p.x = bounds.x;
  } else if (p.x > bounds.x + bounds.w) {
    p.x = bounds.x + bounds.w;
  }

  // Clamp the y-coordinate
  if (p.y < bounds.y) {
    p.y = bounds.y;
  } else if (p.y > bounds.y + bounds.h) {
    p.y = bounds.y + bounds.h;
  }

  return p;
}


typedef struct waveshaper_editor_state {
  int is_held;
  int which;

  int was_low;
  time_t last_time;
  struct nk_vec2 last_point;
  int last_index;
} waveshaper_editor_state;

waveshaper_editor_state wss = {0,0};
  
void draw_waveshaper(struct nk_context *ctx, waveshaper_t* ws) {
  int steps = 80;

  struct nk_color color = nk_rgb(150, 150, 150);
  struct nk_command_buffer *canvas = nk_window_get_canvas(ctx);

  // Change bounds
  struct nk_rect b = nk_layout_widget_bounds(ctx);
  b.w = 100;
  b.h = 100;
  b.x+=10;
  b.y+=10;

  // Draw bounding box
  draw_bounding_box(ctx, b);

  // Vertical line (for xy axis)
  nk_stroke_line(canvas,
		 (short)b.x+b.w/2,
		 (short)b.y,
		 (short)b.x+b.w/2,
		 (short)b.y+b.h,
		 1.0f, nk_rgb(100,100,100));

  // Horizontal line (for xy axis)
  nk_stroke_line(canvas,
		 (short)b.x,
		 (short)b.y+b.h/2,
		 (short)b.x+b.w,
		 (short)b.y+b.h/2,
		 1.0f, nk_rgb(100,100,100));


  // Copy points to local buffer for processing
  point points[ws->points_used];
  fori(ws->points_used) points[i] = ws->points[i];
  fori(ws->points_used) points[i] = scale_point(points[i], b);

  
  // Render points and lines
  fori(ws->points_used-1) {
    point p1 = points[i];
    point p2 = points[i+1];
    float c  = ws->curves[i];

    point prev = p1; // Start at the first point


    // Draw the curve using small line segments
    for (int i = 1; i <= steps; i++) {
      float t = (float)i / (float)steps;
      point current = bezier(p1, p2, c, t);

      // Draw a line segment between the previous point and the current point
      nk_stroke_line(canvas, (short)prev.x, (short)prev.y,
		     (short)current.x, (short)current.y,
		     1.0f, color);

      prev = current; // Update the previous point
    }

    nk_fill_circle(canvas, nk_rect(p1.x-4, p1.y-4, 8, 8), color);
    nk_fill_circle(canvas, nk_rect(p2.x-4, p2.y-4, 8, 8), color);
  }

  // Edit points:
  // Store Nuklear input for mouse handling
  struct nk_input *input = &ctx->input;
  struct nk_vec2 mouse = {input->mouse.pos.x, input->mouse.pos.y};


  if(!nk_input_is_mouse_down(input, NK_BUTTON_LEFT)) {
    wss.which = -1;
    wss.is_held = 0;
    wss.was_low = 1;
    return;
  }

  if (wss.is_held) goto held;
  
  // 1 - check if we are hovering or clicking a point
  fori(ws->points_used) {
    wss.is_held = 0;
    struct nk_rect point_rect = nk_rect(points[i].x - 4, points[i].y - 4, 8, 8);
    if (nk_input_is_mouse_hovering_rect(input, point_rect)) {
      nk_fill_circle(canvas, nk_rect(points[i].x-4, points[i].y-4, 8, 8), nk_rgb(255, 255/3, 0));
      if(nk_input_is_mouse_down(input, NK_BUTTON_LEFT)) {
	wss.is_held = 1;
	wss.which = i;
	break;
      }
    }
  }

 held:
  // 2 - Check if we should create or delete a point
  int i = wss.which;
  if(wss.was_low && rect_contains(b, mouse)) {
    wss.was_low = 0;
    if (mouse.x == wss.last_point.x && mouse.y == wss.last_point.y) {
      time_t diff = clock() - wss.last_time;
      if (diff < 300000) {
	if (i == -1) {
	  // Add new point
	  struct nk_vec2 scaled = unscale_vec(mouse, b);
	  waveshaper_add_point(ws, (point){scaled.x, scaled.y});

	} else {
	  // Delete existing point
	  waveshaper_del_point(ws, i);
	  wss.is_held = 0;
	  wss.which = -1;

	}
	return;
      }
    }
    wss.last_point = mouse;
    wss.last_time = clock();
  }


  // 3 Draw the held point and edit its position
  // Ensure that we have a point to deal with
  if (i == -1) return;

  nk_fill_circle(canvas, nk_rect(points[i].x-4, points[i].y-4, 8, 8), nk_rgb(255, 255/3, 0));

  if (i < ws->points_used-1 && mouse.x >= points[i+1].x) {
    mouse.x = points[i+1].x;
  }
  if (i > 0 && mouse.x <= points[i-1].x) mouse.x = points[i-1].x;

  if (i == 0) mouse.x = b.x;
  if (i == ws->points_used-1) mouse.x = b.x+b.w;
  
  mouse = clamp_to_rect(mouse, b);
  struct nk_vec2 scaled = unscale_vec(mouse, b);
  ws->points[i].x = scaled.x;
  ws->points[i].y = scaled.y;
}
