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
#include <Bounce.h>
#include "math.h"

// The LED on the Teensy
int ledPin = 13;

int buttonPin = 0;

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
            block->data[i] = program_loop(&state);
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

void setup() {
  // Initialize audio memory
  AudioMemory(10);

  Serial.begin(9600);

  pinMode(buttonPin, INPUT);
  pinMode(ledPin, OUTPUT);

  // Turn on sound chip
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.2);
 
  // Initialize Cadence (with project sample rate)
  program_setup(&state, AUDIO_SAMPLE_RATE);

  // Have a small delay before audio starts playing
  delay(300);
  
}

void loop() {

  int button_state = digitalRead(buttonPin);

  if (button_state == HIGH) {
    state.vol = 1;
  }
   if (button_state == LOW) {
    state.vol = 0;
  }

  digitalWrite(ledPin, button_state);
  Serial.println(button_state);

}

