/*****************************************************************
 *  waveshaper.c                                                  *
 *  Created on 18.12.24                                           *
 *  By Børge Lundsaunet                                           *
 *****************************************************************/

#include "effect.h"

float apply_waveshaper(waveshaper_t* w, float x) {
  int n = 10;

  if (x == 0) return 0;
  if (x > 1.0f) x = 1.0f;

  fori(n-1) {
    if (x >= w->points[i].x && x <= w->points[i+1].x) {
      point p0 = w->points[i];
      point p1 = w->points[i+1];

      //float curvature = w->curves[i];

      float t = (x - p0.x) / (p1.x - p0.x);

      point result;
      result = bezier(p0, p1, 0.0, t);
      return result.y;
    }
  }
  return 0;
}

void init_waveshaper(waveshaper_t* w) {
  w->points_used = 0;
  w->points[0] = (point){-1, -1};
  w->points[1] = (point){1, 1};
}

void waveshaper_add_point(waveshaper_t* w, point p) {
  w->points_used += 1;

  fori(w->points_used-2) {

    point p0 = w->points[i];
    point p1 = w->points[i+1];

    if (p.x > p0.x && p.x < p1.x) {

      // Shift points to make space for the new
      for(int j = w->points_used-1; j > i; j--) {
	w->points[j] = w->points[j-1];
      }

      // Add new point
      w->points[i+1] = p;

      return;
    }
  }
}

void waveshaper_del_point(waveshaper_t* w, int index) {
  for(int i = index; i < w->points_used-1; i++) {
    w->points[i] = w->points[i+1];
  }
  w->points_used--;
}

