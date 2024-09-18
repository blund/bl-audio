
#include "math.h"

float mtof(int midi) {
  return 440.0f*pow(2, (float)(midi-69.0f) / 12.0f);
}
