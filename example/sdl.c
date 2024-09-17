#include <SDL2/SDL.h>
#include <stdint.h>
#include <stdbool.h>

#include "cadence.h"

#define internal static

typedef float Real32;

static Real32 PI = 3.14159265359;
static Real32 TAU = 2*M_PI;
static uint32_t SDL_AUDIO_TRANSPARENTLY_CONVERT_FORMAT = 0;

///////////////////////////////////////////////////////////////////////////////


// 97 -> 119;
float vals[25] = {
  0,  // 97 = c
  -1, // 98
  -1, // 99
  4,  // 100 = e
  3,  // 101 = d#
  5,  // 102 = f
  7,  // 103 = g
  9,  // 104 = a
  -1, // 105
  11, // 106 = b
  12, // 107 = c2
  -1, // 108
  -1, // 109
  -1, // 110
  -1, // 111
  -1, // 112
  -1, // 113
  -1, // 114
  2,  // 115 = d
  6,  // 116 = f#
  10, // 117 = a#
  -1, // 118
  1,  // 119 = c#
  -1, // 120
  8   // 121 = g#
};

osc_t test_osc;

typedef struct platform_program_state
{
  cadence_ctx* ctx;

  bool IsRunning;
  SDL_Event LastEvent;
} platform_program_state;

typedef struct platform_audio_config
{
  int SampleIndex;
  int SamplesPerSecond;
  int BytesPerSample;
} platform_audio_config;

typedef struct platform_audio_buffer
{
  uint8_t* Buffer;
  int Size;
  int ReadCursor;
  int WriteCursor;
  SDL_AudioDeviceID DeviceID;
  struct platform_audio_config* AudioConfig;
} platform_audio_buffer;

typedef struct platform_audio_thread_context
{
  platform_audio_buffer* AudioBuffer;
  platform_program_state* ProgramState;
} platform_audio_thread_context;

///////////////////////////////////////////////////////////////////////////////

synth* syn;
delay* d;

internal int16_t
proc_loop(cadence_ctx* ctx)
{
  if (!ctx->initialized) {
    syn = new_synth(8, test_osc);
    d   = new_delay(ctx);
    ctx->initialized = 1;
  }

  process_gen_table(ctx);

  float sample  = play_synth(ctx, syn);
  float delayed = apply_delay(ctx, d, sample, 0.3, 0.6);
  
  return (int16_t)16768*(sample+delayed);
}

///////////////////////////////////////////////////////////////////////////////

internal void
SampleIntoAudioBuffer(platform_audio_buffer* AudioBuffer,
                      int16_t (*GetSample)(cadence_ctx*),
		      cadence_ctx* ctx)
{
  int Region1Size = AudioBuffer->ReadCursor - AudioBuffer->WriteCursor;
  int Region2Size = 0;
  if (AudioBuffer->ReadCursor < AudioBuffer->WriteCursor)
  {
    // Fill to the end of the buffer and loop back around and fill to the read
    // cursor.
    Region1Size = AudioBuffer->Size - AudioBuffer->WriteCursor;
    Region2Size = AudioBuffer->ReadCursor;
  }

  platform_audio_config* AudioConfig = AudioBuffer->AudioConfig;

  int Region1Samples = Region1Size / AudioConfig->BytesPerSample;
  int Region2Samples = Region2Size / AudioConfig->BytesPerSample;
  int BytesWritten   = Region1Size + Region2Size;

  int16_t* Buffer = (int16_t*)&AudioBuffer->Buffer[AudioBuffer->WriteCursor];
  for (int SampleIndex = 0;
       SampleIndex < Region1Samples;
       SampleIndex++)
  {
    int16_t SampleValue = (*GetSample)(ctx);
    *Buffer++ = SampleValue;
    *Buffer++ = SampleValue;
    AudioConfig->SampleIndex++;
  }

  Buffer = (int16_t*)AudioBuffer->Buffer;
  for (int SampleIndex = 0;
       SampleIndex < Region2Samples;
       SampleIndex++)
  {
    int16_t SampleValue = (*GetSample)(ctx);
    *Buffer++ = SampleValue;
    *Buffer++ = SampleValue;
    AudioConfig->SampleIndex++;
  }

  AudioBuffer->WriteCursor =
    (AudioBuffer->WriteCursor + BytesWritten) % AudioBuffer->Size;
}

///////////////////////////////////////////////////////////////////////////////

internal void
PlatformFillAudioDeviceBuffer(void* UserData, uint8_t* DeviceBuffer, int Length)
{
  platform_audio_buffer* AudioBuffer = (platform_audio_buffer*)UserData;

  // Keep track of two regions. Region1 contains everything from the current
  // PlayCursor up until, potentially, the end of the buffer. Region2 only
  // exists if we need to circle back around. It contains all the data from the
  // beginning of the buffer up until sufficient bytes are read to meet Length.
  int Region1Size = Length;
  int Region2Size = 0;
  if (AudioBuffer->ReadCursor + Length > AudioBuffer->Size)
  {
    // Handle looping back from the beginning.
    Region1Size = AudioBuffer->Size - AudioBuffer->ReadCursor;
    Region2Size = Length - Region1Size;
  }

  SDL_memcpy(
    DeviceBuffer,
    (AudioBuffer->Buffer + AudioBuffer->ReadCursor),
    Region1Size
  );
  SDL_memcpy(
    &DeviceBuffer[Region1Size],
    AudioBuffer->Buffer,
    Region2Size
  );

  AudioBuffer->ReadCursor =
    (AudioBuffer->ReadCursor + Length) % AudioBuffer->Size;
}

///////////////////////////////////////////////////////////////////////////////

internal void
PlatformInitializeAudio(platform_audio_buffer* AudioBuffer)
{
  SDL_AudioSpec AudioSettings = {};
  AudioSettings.freq = AudioBuffer->AudioConfig->SamplesPerSecond;
  AudioSettings.format = AUDIO_S16LSB;
  AudioSettings.channels = 2;
  AudioSettings.samples = 32;
  AudioSettings.callback = &PlatformFillAudioDeviceBuffer;
  AudioSettings.userdata = AudioBuffer;

  SDL_AudioSpec ObtainedSettings = {};
  AudioBuffer->DeviceID = SDL_OpenAudioDevice(
    NULL, 0, &AudioSettings, &ObtainedSettings,
    SDL_AUDIO_TRANSPARENTLY_CONVERT_FORMAT
  );

  if (AudioSettings.format != ObtainedSettings.format)
  {
    SDL_Log("Unable to obtain expected audio settings: %s", SDL_GetError());
    exit(1);
  }

  // Start playing the audio buffer
  SDL_PauseAudioDevice(AudioBuffer->DeviceID, 0);
}

///////////////////////////////////////////////////////////////////////////////

internal void
PlatformHandleEvent(platform_program_state* ProgramState)
{
  if (ProgramState->LastEvent.type == SDL_QUIT)
  {
    ProgramState->IsRunning = false;
  }
}

///////////////////////////////////////////////////////////////////////////////

internal int
PlatformAudioThread(void* UserData)
{
  platform_audio_thread_context* AudioThread =
    (platform_audio_thread_context*)UserData;

  while (AudioThread->ProgramState->IsRunning)
  {
    SDL_LockAudioDevice(AudioThread->AudioBuffer->DeviceID);
    SampleIntoAudioBuffer(AudioThread->AudioBuffer, &proc_loop, AudioThread->ProgramState->ctx);
    SDL_UnlockAudioDevice(AudioThread->AudioBuffer->DeviceID);
  }

  return 0;
}

///////////////////////////////////////////////////////////////////////////////

int main()
{
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0)
  {
    SDL_Log("Unable to initialized SDL: %s", SDL_GetError());
    return 1;
  }

  SDL_Window* Window = SDL_CreateWindow(
    "Cadence",
    SDL_WINDOWPOS_UNDEFINED,
    SDL_WINDOWPOS_UNDEFINED,
    1280,
    720,
    SDL_WINDOW_OPENGL
  );

  if (!Window)
  {
    SDL_Log("Unable to initialize window: %s", SDL_GetError());
    return 1;
  }

  SDL_Renderer* Renderer = SDL_CreateRenderer(Window, -1, 0);

  platform_audio_config AudioConfig = {};
  AudioConfig.SamplesPerSecond = 44100;
  AudioConfig.BytesPerSample = 2 * sizeof(int16_t);
  AudioConfig.SampleIndex = 0;

  platform_audio_buffer AudioBuffer = {};
  AudioBuffer.Size = 2048; //AudioConfig.SamplesPerSecond * AudioConfig.BytesPerSample; // 2048

  AudioBuffer.Buffer = malloc(sizeof(uint8_t)*AudioBuffer.Size);
  // Two data points per sample. One for the left speaker, one for the right.
  AudioBuffer.ReadCursor = 0;
  // NOTE: Offset by 1 sample in order to cause the circular buffer to
  // initially be filled.
  AudioBuffer.WriteCursor = AudioConfig.BytesPerSample;
  AudioBuffer.AudioConfig = &AudioConfig;
  memset(AudioBuffer.Buffer, 0, AudioBuffer.Size);

  PlatformInitializeAudio(&AudioBuffer);

  platform_program_state ProgramState = {};
  ProgramState.IsRunning = true;

  platform_audio_thread_context AudioThreadContext = {};
  AudioThreadContext.AudioBuffer = &AudioBuffer;
  AudioThreadContext.ProgramState = &ProgramState;
  SDL_Thread* AudioThread = SDL_CreateThread(
    PlatformAudioThread, "Audio", (void*)&AudioThreadContext
  );


  // Set up cadence context
  ProgramState.ctx = cadence_setup(44100);
  
  
  SDL_Event event;
  while (ProgramState.IsRunning)
  {

    while (SDL_PollEvent(&event))
    {
      if (event.type == SDL_KEYDOWN) {

	// Avoid repeats
	if (event.key.repeat) break;
	
	int sym = event.key.keysym.sym;

	// Check if program should quit
	if (sym == SDLK_q) ProgramState.IsRunning = 0;

	// Clean up the inputs
	if (sym > 121 | sym < 97) break;
	int key_idx = vals[sym-97];
	if (key_idx == -1) break;

	// Find freq for note
	float freq = 261.63*pow(1.05946309436, key_idx);

	// Make some sound :)
	synth_register_note(syn, freq, 0.1, NOTE_ON, sym); // Note that notes are keyed by ascii key code (sym)
      }

      if (event.type == SDL_KEYUP) {
	int sym = event.key.keysym.sym;
	synth_register_note(syn, 0, 0, NOTE_OFF, sym);
      }

      if (event.type == SDL_QUIT) {
	ProgramState.IsRunning = 0;
      }
    }

    SDL_SetRenderDrawColor(Renderer, 0, 0, 0, 0);
    SDL_RenderClear(Renderer);
    SDL_RenderPresent(Renderer);
  }

  SDL_WaitThread(AudioThread, NULL);

  SDL_DestroyRenderer(Renderer);
  SDL_DestroyWindow(Window);
  SDL_CloseAudioDevice(AudioBuffer.DeviceID);
  SDL_Quit();

  return 0;
}


// Test osc to demonstrate polyphony
float test_osc(cadence_ctx* ctx, synth* s, int note_index, note* note) {
  static int init = 0;

  // oscillators for each note
  static sine** sines;
  static phasor** phasors;

  // index for oscillator in gen table
  static int sine_i;

  // handle release
  static int release_samples = 5000;
  static int* release_remaining_samples;

  if (!init) {
    init = 1;

    // set up local oscillators
    sines   = malloc(s->poly_count * sizeof(sine));
    phasors = malloc(s->poly_count * sizeof(phasor));
    fori(s->poly_count) sines[i] = new_sine();
    fori(s->poly_count) phasors[i] = new_phasor();


    // set up release table
    release_remaining_samples = malloc(s->poly_count * sizeof(int));
    fori(s->poly_count) release_remaining_samples[i] = 0;

    // and global ones :)
    sine_i = register_gen_table(ctx, GEN_SINE);
    ctx->gt[sine_i].p->freq = 8.0;
  }

  if (check_flag(note, NOTE_RESET)) {
    sines[note_index]->t = 0;
    phasors[note_index]->value = 0;
    release_remaining_samples[note_index] = release_samples;
    
    unset_flag(note, NOTE_RESET);
  }

  phasors[note_index]->freq = 8.0f;
  float phase = gen_phasor(ctx, phasors[note_index]);
  note->amp *= 1.0f-phase;

  float mod = ctx->gt[sine_i].val;

  sines[note_index]->freq = note->freq;// + 15.0*mod;
  float sample = 0.2 * gen_sine(ctx, sines[note_index]);

  if (check_flag(note, NOTE_RELEASE)) {
    sample *= (float)release_remaining_samples[note_index] / (float)release_samples;
    release_remaining_samples[note_index] -= 1;
    if (release_remaining_samples[note_index] == 0) {
      set_flag(note, NOTE_FREE);
      release_remaining_samples[note_index] = release_samples;
    }
  }

  return sample;
}
