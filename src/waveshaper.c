
#include "../effect.h"

float apply_waveshaper(waveshaper_t* w, float a) {
  int n = 10;

  if (a == 0) return 0;
  if (a > 1.0f) a = 1.0f;

  fori(n-1) {
    if (a > w->points[i].x && a < w->points[i+1].x) {
      ws_point p0 = w->points[i];
      ws_point p1 = w->points[i+1];
      return p0.y + (p1.y - p0.y)/(p1.x - p0.x) * (a-p0.x);
    }
  }
  return 0;
}

