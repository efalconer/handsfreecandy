/**
 * Child arduino for lights/FX
 */

#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include <SoftwareSerial.h>
#include "Adafruit_Soundboard.h"

// Which pin on the Arduino is connected to the NeoPixels?
// On a Trinket or Gemma we suggest changing this to 1:
#define LED_PIN    8

// How many NeoPixels are attached to the Arduino?
#define LED_COUNT 30
// Choose any two pins that can be used with SoftwareSerial to RX & TX
#define SFX_TX 5
#define SFX_RX 6

// Connect to the RST pin on the Sound Board
#define SFX_RST 4

const int CHANNEL = 9;
const int MESSAGE_IDLE = 0;
const int MESSAGE_RUNNING = 1;
const int MESSAGE_DISPENSED = 2;
const int MESSAGE_REVERSE = 3;
const int ACT_PIN = 10;
int currentState = MESSAGE_IDLE;
int newState = MESSAGE_IDLE;
bool audioAvailable = false;

unsigned long timeSinceLastAudioTrigger = 0;
const int AUDIO_TRIGGER_COOLDOWN = 50;

const char *ambFilenames[] = {
  "AMB02   OGG",
  "AMB04   OGG" 
};
const char *fxFilenames[] = {
  "FX01    OGG",
  "FX02    OGG",
  "FX03    OGG",
  "FX04    OGG",
  "FX05    OGG",
  "FX06    OGG",
  "FX07    OGG",
  "FX08    OGG"
};
#define numFXFiles (sizeof(fxFilenames)/sizeof(char *)) 
#define numAmbFiles (sizeof(ambFilenames)/sizeof(char *)) 

SoftwareSerial ss = SoftwareSerial(SFX_TX, SFX_RX);
Adafruit_Soundboard sfx = Adafruit_Soundboard(&ss, NULL, SFX_RST);

// Declare our NeoPixel strip object:
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);


// setup() function -- runs once at startup --------------------------------

void setup() {

  Serial.begin(115200);
  randomSeed(analogRead(0));

  // Start the I2C Bus as Slave on address 9
  Wire.begin(CHANNEL); 
  // Attach a function to trigger when something is received.
  Wire.onReceive(receiveEvent);

  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(50); // Set BRIGHTNESS to about 1/5 (max = 255)

  // audio serial interface
  ss.begin(9600);
  pinMode(ACT_PIN, INPUT);   
    
  if (sfx.reset()) {
    audioAvailable = true;
  } else {
    Serial.println("SFX board not found");
  }

  Serial.println("Child setup complete");
}

void receiveEvent(int bytes) {
  newState = Wire.read();    // read one character from the I2C
  Serial.println("Received event");
  Serial.println(newState);
}

void stopAudio() {
  Serial.println("Stopping audio");
  sfx.stop();
}

void playAudioFile(const char* filename, bool force) {

  if (timeSinceLastAudioTrigger != 0 
        && millis() - timeSinceLastAudioTrigger < AUDIO_TRIGGER_COOLDOWN) {
    // don't try to play audio back to back
    return;
  }
  
  flushInput();

  // if something is playing, stop it
  if (digitalRead(ACT_PIN) == LOW && force) {
    stopAudio();
  }

  flushInput();
  
  Serial.print("Playing: ");
  Serial.println(filename);
  if (!sfx.playTrack(const_cast<char *>(filename))) {
    Serial.println("Failed to play audio");
  }

  timeSinceLastAudioTrigger = millis();
  
}

void checkAndPlayAmbience() {
  // if nothing is playing start the ambience track
  if (digitalRead(ACT_PIN) == HIGH) {
    Serial.println("Playing background music");
    playAudioFile(ambFilenames[random(numAmbFiles)], false);
  }
}

void playRandomFX() {
  Serial.println("Playing fx file");
  playAudioFile(fxFilenames[random(numFXFiles)], true);
}

bool stateChanged() {
  if (newState != currentState) {
      return true; // short circuit
  }
  if (currentState == MESSAGE_IDLE) {
    checkAndPlayAmbience();
  }

   return false;
}

/**
 * Delay for a set amount of time, but check for input signals and audio gaps
 */
bool checkDelay(int delay) {
  unsigned long startTime = millis();
  while (millis() - startTime < delay) {
    if (stateChanged()) { 
      Serial.println("State changed during delay");
      return false;
    }
  }

  return true; // full timeout reached
}


void loop() {
  flushInput();
  
  // detect if state changed
  if (newState != currentState) {
    Serial.print("New state: ");
    Serial.println(newState);
    
    currentState = newState;
    if (currentState == MESSAGE_RUNNING) {
      playRandomFX();
    }
  }

  if (currentState == MESSAGE_IDLE) {
    checkAndPlayAmbience();
    rainbow(15); // Flowing rainbow cycle along the whole strip
  } else if (currentState == MESSAGE_RUNNING){
    theaterChase(strip.Color(  0,   0, 127), 50); // Blue, half brightness
  } else if (currentState == MESSAGE_DISPENSED) {
    colorWipe(strip.Color(0, 127, 0), 50); 
  } else if (currentState = MESSAGE_REVERSE) {
    colorWipe(strip.Color(127, 0, 0), 50); 
  } else {
    Serial.print("Unknown state: ");
    Serial.println(currentState);
  }
}


// Some functions of our own for creating animated effects -----------------

// Fill strip pixels one after another with a color. Strip is NOT cleared
// first; anything there will be covered pixel by pixel. Pass in color
// (as a single 'packed' 32-bit value, which you can get by calling
// strip.Color(red, green, blue) as shown in the loop() function above),
// and a delay time (in milliseconds) between pixels.
void colorWipe(uint32_t color, int wait) {
  strip.clear();
  for(int i=0; i<strip.numPixels(); i++) { // For each pixel in strip...
    strip.setPixelColor(i, color);         //  Set pixel's color (in RAM)
    strip.show();                          //  Update strip to match
    if (!checkDelay(wait)) return;         // Pause for a moment
  }
}

// Theater-marquee-style chasing lights. Pass in a color (32-bit value,
// a la strip.Color(r,g,b) as mentioned above), and a delay time (in ms)
// between frames.
void theaterChase(uint32_t color, int wait) {
  for(int a=0; a<10; a++) {  // Repeat 10 times...
    for(int b=0; b<3; b++) { //  'b' counts from 0 to 2...
      strip.clear();         //   Set all pixels in RAM to 0 (off)
      // 'c' counts up from 'b' to end of strip in steps of 3...
      for(int c=b; c<strip.numPixels(); c += 3) {
        if (stateChanged()) return;
        strip.setPixelColor(c, color); // Set pixel 'c' to value 'color'
      }
      strip.show(); // Update strip with new contents
      if (!checkDelay(wait)) return;  // Pause for a moment
    }
  }
}

// Rainbow cycle along whole strip. Pass delay time (in ms) between frames.
void rainbow(int wait) {
  // Hue of first pixel runs 5 complete loops through the color wheel.
  // Color wheel has a range of 65536 but it's OK if we roll over, so
  // just count from 0 to 5*65536. Adding 256 to firstPixelHue each time
  // means we'll make 5*65536/256 = 1280 passes through this outer loop:
  for(long firstPixelHue = 0; firstPixelHue < 5*65536; firstPixelHue += 256) {
    for(int i=0; i<strip.numPixels(); i++) { // For each pixel in strip...
      if (stateChanged()) return;
      // Offset pixel hue by an amount to make one full revolution of the
      // color wheel (range of 65536) along the length of the strip
      // (strip.numPixels() steps):
      int pixelHue = firstPixelHue + (i * 65536L / strip.numPixels());
      // strip.ColorHSV() can take 1 or 3 arguments: a hue (0 to 65535) or
      // optionally add saturation and value (brightness) (each 0 to 255).
      // Here we're using just the single-argument hue variant. The result
      // is passed through strip.gamma32() to provide 'truer' colors
      // before assigning to each pixel:
      strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
    }
    strip.show(); // Update strip with new contents
    if (!checkDelay(wait)) return;  // Pause for a moment
  }
}

// Rainbow-enhanced theater marquee. Pass delay time (in ms) between frames.
void theaterChaseRainbow(int wait) {
  int firstPixelHue = 0;     // First pixel starts at red (hue 0)
  for(int a=0; a<30; a++) {  // Repeat 30 times...
    for(int b=0; b<3; b++) { //  'b' counts from 0 to 2...
      strip.clear();         //   Set all pixels in RAM to 0 (off)
      // 'c' counts up from 'b' to end of strip in increments of 3...
      for(int c=b; c<strip.numPixels(); c += 3) {
        if (stateChanged()) return;
        // hue of pixel 'c' is offset by an amount to make one full
        // revolution of the color wheel (range 65536) along the length
        // of the strip (strip.numPixels() steps):
        int      hue   = firstPixelHue + c * 65536L / strip.numPixels();
        uint32_t color = strip.gamma32(strip.ColorHSV(hue)); // hue -> RGB
        strip.setPixelColor(c, color); // Set pixel 'c' to value 'color'
      }
      strip.show();                // Update strip with new contents
      if (!checkDelay(wait)) return;  // Pause for a moment
      firstPixelHue += 65536 / 90; // One cycle of color wheel over 90 frames
    }
  }
}


void flushInput() {
  // Read all available serial input to flush pending data.
  uint16_t timeoutloop = 0;
  while (timeoutloop++ < 40) {
    while(ss.available()) {
      ss.read();
      timeoutloop = 0;  // If char was received reset the timer
    }
    delay(1);
  }
}
