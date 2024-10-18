/*****************************************************************
 *  teensy.ino                                                    *
 *  Created on 11.10.24                                           *
 *  By Børge Lundsaunet                                           *
 *****************************************************************/

/*
  This is a demonstration of how to integrate Cadence with Arduino.
  For my own benefit, this example is built using a teensy and its Audio Cape
*/

extern "C" {
#include "program.h"
}

#include <Audio.h>
#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#include <SerialFlash.h>


// Create the state, which contains the Cadence context, as well as values that are
// shared between this platform layer and Cadence
program_state state;

// This is the object that wraps Cadence in Teensy's AudioStream object.
class AudioCadenceEngine : public AudioStream {
 public:
 AudioCadenceEngine() : AudioStream(0, NULL) { }  // No inputs, only output

  // The update() function will be called regularly by the audio library
  void update(void) {
    audio_block_t *block = allocate();  // Allocate a new block of memory to store audio samples
    if (!block) return;  // If memory allocation fails, return

    // Generate samples
    for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
      float sample = 0;

      // Ensure that the context exists before we start playing.
      // The teensy has a branch predictor, so this should be fine :)
      if (state.ctx) {
        sample = program_loop(&state);
      }

      block->data[i] = sample;
    }

    transmit(block);  // Transmit the block to the audio library
    release(block);   // Release the memory block
  }
};


// Declare Cadence Engine object
AudioCadenceEngine cadence;

// Hook it up to the Teensy Audio system :)
AudioOutputI2S        i2s1;
AudioConnection       patchCord1(cadence, 0, i2s1, 0);
AudioConnection       patchCord2(cadence, 0, i2s1, 1);
AudioControlSGTL5000  sgtl5000_1;


struct button {
  int pin;
  int val;
  int last_val = 0;

  int detect_edge() {
    if (val != last_val) {
      last_val = val;
      return 1;
    }
    return 0;
  }
};

button buttons[5];

// The LED on the Teensy
int led_pin = 13;

void setup() {
  // Initialize audio memory
  AudioMemory(10);

  Serial.begin(9600);

  // Initialize input buttons
  fori(5) {
    buttons[i].pin = i;
    pinMode(buttons[i].pin, INPUT);
  }

  pinMode(led_pin, OUTPUT);

  // Turn on sound chip
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.4);
 
  // Initialize Cadence (with project sample rate)
  program_setup(&state, AUDIO_SAMPLE_RATE);

  // Have a small delay before audio starts playing
  digitalWrite(led_pin, HIGH);
  delay(30);
  digitalWrite(led_pin, LOW);
  delay(80);
  digitalWrite(led_pin, HIGH);
  delay(30);
  digitalWrite(led_pin, LOW);
  delay(80);
  
}


// Translate button indices to midi notes (Ocarina of Time scale)
int note_map[5] = {62, 65, 69, 71, 74};

void loop() {

  // Read buttons
  fori(5) buttons[i].val = digitalRead(buttons[i].pin);

  // Blink light if at least one is active
  int led_state = LOW;
  fori(5) if (buttons[i].val == HIGH) led_state = HIGH;
  digitalWrite(led_pin, led_state);

  // Create midi events on button signal edges
  fori(5) {
    int edge = buttons[i].detect_edge();
    if (edge) {
      if (buttons[i].val == HIGH) midi_event(note_map[i], NOTE_ON);
      if (buttons[i].val == LOW)  midi_event(note_map[i], NOTE_OFF);
    }
  }

  delay(10);
}

