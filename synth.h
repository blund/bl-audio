#define fori(lim) for(int i = 0; i < lim; i++)

typedef struct note {
  float freq;
  float amp;
  float len_samples;
  int free; // denote whether this slot is free or not
  int reset;
} note;

typedef struct synth {
  note* notes;
  int poly_count;
  float(*osc)(float freq, float amp, int index, int* reset);
} synth;


synth* new_synth(int poly_count, float(*osc)(float, float, int, int*));
void synth_register_note(synth* s, float freq, float amp, int len_samples);
float synth_play(synth* s);
