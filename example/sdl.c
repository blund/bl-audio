/*****************************************************************
 *  sdl.c                                                         *
 *  Created on 18.09.24                                           *
 *  By Børge Lundsaunet                                           *
 *****************************************************************/



#include <SDL2/SDL.h>
#include <stdint.h>
#include <stdbool.h>

#include <dlfcn.h>

#include "library.h"
#include "load_lib.h"


#include "cadence.h"

#define internal static

typedef float Real32;

static Real32 PI = 3.14159265359;
static Real32 TAU = 2*M_PI;
static uint32_t SDL_AUDIO_TRANSPARENTLY_CONVERT_FORMAT = 0;

///////////////////////////////////////////////////////////////////////////////


// 97 -> 119;
float vals[25] = {
  60, // 97 = c
  -1, // 98
  -1, // 99
  64, // 100 = e
  63, // 101 = d#
  65, // 102 = f
  67, // 103 = g
  69, // 104 = a
  -1, // 105
  71, // 106 = b
  72, // 107 = c2
  -1, // 108
  -1, // 109
  -1, // 110
  -1, // 111
  -1, // 112
  -1, // 113
  -1, // 114
  62, // 115 = d
  66, // 116 = f#
  70, // 117 = a#
  -1, // 118
  61, // 119 = c#
  -1, // 120
  68  // 121 = g#
};

library* lib;

const char* lib_path = "/home/blund/prosjekt/personlig/cadence/example/program.so";
void load_functions(library* lib) {
    load_lib(lib, lib_path);
    program_loop_t* pl =   (program_loop_t*)dlsym(lib->handle, "program_loop");
    lib->functions.program_loop = pl;
    lib->functions.midi_event   = (midi_event_t*)dlsym(lib->handle,   "midi_event");
}

typedef struct platform_program_state
{
  void* program_data;

  bool      IsRunning;
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
  program_loop_t* program_loop;
} platform_audio_thread_context;

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////

internal void
SampleIntoAudioBuffer(platform_audio_buffer* AudioBuffer,
		      program_loop_t program_loop,
                      //int16_t (*GetSample)(void* data),
		      void* data)
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
  for (int SampleIndex = 0; SampleIndex < Region1Samples; SampleIndex++) {
    int16_t SampleValue = (*program_loop)(data);
    *Buffer++ = SampleValue;
    *Buffer++ = SampleValue;
    AudioConfig->SampleIndex++;
  }

  Buffer = (int16_t*)AudioBuffer->Buffer;
  for (int SampleIndex = 0; SampleIndex < Region2Samples; SampleIndex++) {
    int16_t SampleValue = (*program_loop)(data);
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

  int counter = 0;
  while (AudioThread->ProgramState->IsRunning)  {
    if (counter++ % 10000) {
      time_t last_write = get_last_write(lib_path);
      if (last_write != lib->last_write) {
	wait_for_lock();

	int close_result = dlclose(lib->handle);

	load_functions(lib);
      }
    }



    SDL_LockAudioDevice(AudioThread->AudioBuffer->DeviceID);
    SampleIntoAudioBuffer(AudioThread->AudioBuffer, lib->functions.program_loop, AudioThread->ProgramState->program_data);
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

  // Load library functions
  lib = malloc(sizeof(library));
  set_lockfile("/home/blund/prosjekt/personlig/cadence/example/lockfile");
  set_tmp_path("/home/blund/prosjekt/personlig/cadence/example/tmp.so");
  load_functions(lib);

  
  platform_audio_thread_context AudioThreadContext = {};
  AudioThreadContext.AudioBuffer = &AudioBuffer;
  AudioThreadContext.ProgramState = &ProgramState;
  SDL_Thread* AudioThread = SDL_CreateThread(
    PlatformAudioThread, "Audio", (void*)&AudioThreadContext
  );


  SDL_Event event;
  int counter = 0;
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
	if (sym > 121 | sym < 97) continue;
	int midi_note = vals[sym-97];
	if (midi_note == -1) break;

	// registrer event, med synth 0, midi-note, vel og note-on
	lib->functions.midi_event(0, midi_note, 0.1, NOTE_ON);
      }

      if (event.type == SDL_KEYUP) {
	int sym = event.key.keysym.sym;
	if (sym > 121 | sym < 97) continue;
	int midi_note = vals[sym-97];
	// registrer event, med synth 0, midi-note, vel og note-on
	lib->functions.midi_event(0, midi_note, 0.1, NOTE_OFF);
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


