#include <Arduino.h>
#include "RunningMedian.h"
// Information about hardware connections, functions and definitions
#include <NewPing.h>
#include "FastLED.h"
FASTLED_USING_NAMESPACE

//  Declare pins
#define DATA_PIN 2  // Strip data pin
#define TRIGGER_PIN  12  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN     11  // Arduino pin tied to echo pin on the ultrasonic sensor.
#define MAX_DISTANCE 200 // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.
#define ECHO_TO_SERIAL   1 // echo data to serial port
#define NUM_LEDS 144  // Number of leds in strip
#define BRIGHTNESS 100  // Set LEDS brightness
#define FRAMES_PER_SECOND  120
#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); // NewPing setup of pins and maximum distance.
CRGB leds[NUM_LEDS];  // Define the array of leds

static uint8_t default_hue=50;
static uint8_t hue = default_hue;
/*
 * 50=yellow
 * 100=green
 * 160=blue
 * 0=red
 */
static uint8_t hue_step = 24;
static uint8_t tail = 180;
static int step = -1;
static uint8_t pos = NUM_LEDS;
static uint8_t turned_on_leds = 0;
static uint8_t wait_time = 10;
unsigned long Distance=200, Distance_limit=200, ObstacleDetected=0,NoObstacleDetected=0 ;
uint8_t mapped_distance=0;

RunningMedian DistanceSamples = RunningMedian(5);

void setup(void)
{
  
  Serial.begin(115200);
    
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);    

  alloff();

  Serial.println("hello");
}

// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
//SimplePatternList gPatterns = {sinelon, juggle, bpm, rainbow, rainbowWithGlitter, confetti };
SimplePatternList gPatterns = {sinelon, juggle, bpm, rainbow, confetti };
//char* SimplePatternNames[]={"sinelon", "juggle", "bpm", "rainbow", "rainbowWithGlitter", "confetti" };
char* SimplePatternNames[]={"sinelon", "juggle", "bpm", "rainbow", "confetti" };
uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t gHue = 0; // rotating "base color" used by many of the patterns

void fadeall() {
  for (int i = turned_on_leds+1; i < NUM_LEDS; i++) {
    leds[i].nscale8(tail);
  }
}
void alloff() {
  for (int i = NUM_LEDS; i >=0; i--) {
    leds[i]=CRGB::Black;
    delay(20);
    FastLED.show();
  }
  //FastLED.show();
}

void nextPattern()
{
  // add one to the current pattern number, and wrap around at the end
  gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE( gPatterns);
}

void rainbow() 
{
  // FastLED's built-in rainbow generator
  fill_rainbow( leds, NUM_LEDS, gHue, 7);
}

void rainbowWithGlitter() 
{
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow();
  addGlitter(80);
}

void addGlitter( fract8 chanceOfGlitter) 
{
  if( random8() < chanceOfGlitter) {
    leds[ random16(NUM_LEDS) ] += CRGB::White;
  }
}

void confetti() 
{
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV( gHue + random8(64), 200, 255);
}

void sinelon()
{
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( leds, NUM_LEDS, 20);
  int pos = beatsin16(13,0,NUM_LEDS);
  leds[pos] += CHSV( gHue, 255, 192);
}

void bpm()
{
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 62;
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for( int i = 0; i < NUM_LEDS; i++) { //9948
    leds[i] = ColorFromPalette(palette, gHue+(i*2), beat-gHue+(i*10));
  }
}

void juggle() {
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy( leds, NUM_LEDS, 20);
  byte dothue = 0;
  for( int i = 0; i < 8; i++) {
    leds[beatsin16(i+7,0,NUM_LEDS)] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
}
enum {Demo,DistanceMeter} Mode=Demo;

void loop(void){
  delay (wait_time);
  Distance=sonar.ping_cm();
  Serial.print("Distance=");
  Serial.print(Distance);
    
  if(Distance!=0){ObstacleDetected++;}
  else {NoObstacleDetected++;}

  if(ObstacleDetected>5){Mode=DistanceMeter;ObstacleDetected=0; NoObstacleDetected=0;}
  if(NoObstacleDetected>400){Mode=Demo;ObstacleDetected=0; NoObstacleDetected=0;}

  Serial.print(" Mode=");
  Serial.println(Mode);
  
  switch (Mode){
  
  case DistanceMeter:
    if(Distance!=0){
    DistanceSamples.add(Distance);
    mapped_distance=map(DistanceSamples.getAverage(),0,Distance_limit,NUM_LEDS,0);
    }
    hue=120;
    for(int i=0;i<=mapped_distance;i++){
      hue=hue-1;
      leds[i] = CHSV(hue, 255, 255);
    }
  FastLED.show();
  fadeall();
  break;
  
  case Demo:
  // Call the current pattern function once, updating the 'leds' array
     gPatterns[gCurrentPatternNumber]();

  // send the 'leds' array out to the actual LED strip
  FastLED.show();  
  // insert a delay to keep the framerate modest
  FastLED.delay(1000/FRAMES_PER_SECOND); 

  // do some periodic updates
  EVERY_N_MILLISECONDS( 20 ) { gHue++; } // slowly cycle the "base color" through the rainbow
  EVERY_N_SECONDS( 10 ) { nextPattern(); } // change patterns periodically

  
  break;
  }
}


