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
 #include "src/cadence.h"
}

// Teensy 2.0 has the LED on pin 11
// Teensy++ 2.0 has the LED on pin 6
// Teensy 3.x / Teensy LC have the LED on pin 13
const int ledPin = 13;

// the setup() method runs once, when the sketch starts

cadence_ctx* ctx;
sine_t* sine;

void setup() {
  Serial.begin(9600);

  // initialize the digital pin as an output.
  pinMode(ledPin, OUTPUT);

  ctx = cadence_setup(44100);
  sine = new_sine();
  sine->freq = 100.0f;
}

// the loop() methor runs over and over again,
// as long as the board has power

void loop() {
  float val = gen_sine(ctx, sine);
  delay(1);  
  if (val > 0.5) {
    digitalWrite(ledPin, HIGH);   // set the LED on

  } else {
    digitalWrite(ledPin, LOW);    // set the LED off
       
  }            // wait for a second
  Serial.print("\t output = ");
  Serial.println(val);
}

