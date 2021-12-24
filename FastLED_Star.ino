#define FASTLED_INTERNAL
#include <FastLED.h>

FASTLED_USING_NAMESPACE

#define DATA_PIN_1 2
#define DATA_PIN_2 3
#define DATA_PIN_3 4
#define DATA_PIN_4 5
#define DATA_PIN_5 6

#define BUTTON_PIN 9

#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
#define NUM_LEDS    10
#define NUM_STRIPS  5

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

#define FRAMES_PER_SECOND  120
#define MAX_POWER_MILLIAMPS 500
#define LONG_PRESS_TIME 1000
#define DOUBLE_CLICK_TIME 500

CRGB gLeds[NUM_STRIPS][NUM_LEDS];

int gCurStrip = 0;
int gStripDirection = 1; // 1 or -1
bool gRotate = false;
uint8_t gBrightness[] = { 96, 32, 8, 0 };
int gCurBrightness = 1;

int gLastButton = HIGH; // high in pullup mode
int gCurrButton;
unsigned long gOnTime = 0;
bool gButtonOn = false;
bool gAutoPattern = true;

void setup() {
  delay(3000); // 3 second delay for recovery
  Serial.begin(9600);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE,DATA_PIN_1,COLOR_ORDER>(gLeds[0], NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE,DATA_PIN_2,COLOR_ORDER>(gLeds[1], NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE,DATA_PIN_3,COLOR_ORDER>(gLeds[2], NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE,DATA_PIN_4,COLOR_ORDER>(gLeds[3], NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE,DATA_PIN_5,COLOR_ORDER>(gLeds[4], NUM_LEDS).setCorrection(TypicalLEDStrip);

  // set master gBrightness control
  FastLED.setBrightness(gBrightness[gCurBrightness]);

  Serial.println("Start");
}

// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])(int);
SimplePatternList gPatterns = { rainbow, rainbowWithGlitter, confetti, sinelon, juggle, bpm };

uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t gHue = 0; // rotating "base color" used by many of the patterns

void loop()
{
  doButton();
  
  if (gBrightness[gCurBrightness] == 0) {
    return;
  }

  // pattern functions call
  for(int s = 0; s < NUM_STRIPS; s++) {
    if (gRotate && (gCurStrip != s)) {
      fadeToBlackBy( gLeds[s], NUM_LEDS, 10);
    } else {
      gPatterns[gCurrentPatternNumber](s);
    }
  }


  // send the 'gLeds' array out to the actual LED strip
  FastLED.show();  
  // insert a delay to keep the framerate modest
  delay(1000/FRAMES_PER_SECOND); 

  // do some periodic updates
  EVERY_N_MILLISECONDS( 150 ) { 
    //gCurStrip = abs(gCurStrip + gStripDirection) % NUM_STRIPS; 
    gCurStrip += gStripDirection;
    if (gCurStrip < 0) gCurStrip = (NUM_STRIPS - 1);
    if (gCurStrip >= NUM_STRIPS) gCurStrip = 0;
  } // cycle through led strips
  EVERY_N_SECONDS (5) { gStripDirection = -gStripDirection; } // cycle strip rotation direction
  EVERY_N_MILLISECONDS( 20 ) { gHue++; } // slowly cycle the "base color" through the rainbow
  if (gAutoPattern) {
    EVERY_N_SECONDS( 10 ) { nextPattern(); } // change patterns periodically
  }
}

void doButton() {

  bool b_event = false;
  bool b_double_click = false;
    
  gCurrButton = digitalRead(BUTTON_PIN);
  if (gLastButton == HIGH && gCurrButton == LOW) { // button is pressed
    b_double_click = ((millis() - gOnTime) < DOUBLE_CLICK_TIME);
    gOnTime = millis();
    gButtonOn = true;
    b_event = true;
    //Serial.println("+ press");
  }
  if (gLastButton == LOW && gCurrButton == HIGH) { // button is released
    gButtonOn = false;
    b_event = true;
    //Serial.println("+ release");
  }
  gLastButton = gCurrButton;

  if (b_event) { // press or release
    if (gButtonOn) {
      if (b_double_click) {
        Serial.println("Double Click");
        gAutoPattern = true;
        Serial.println("+ Autopattern");
      } else {
        Serial.println("Click");
        gAutoPattern = false;
        nextPattern();
        Serial.println("+ Next Pattern");
      }
    } else {
    }
  } else if (gButtonOn && (millis() - gOnTime) > LONG_PRESS_TIME) { // long press
    gButtonOn = false;
    Serial.println("Long Click");
    gCurBrightness = (gCurBrightness + 1) % ARRAY_SIZE(gBrightness);
    Serial.print("+ Brightness: ");
    Serial.println(gBrightness[gCurBrightness]);
    FastLED.setBrightness(gBrightness[gCurBrightness]);
    FastLED.show();  
  }
  
}

void nextPattern()
{
  // add one to the current pattern number, and wrap around at the end
  gCurrentPatternNumber += 1;
  if (gCurrentPatternNumber >= ARRAY_SIZE( gPatterns)) {
    gCurrentPatternNumber = 0;
    gRotate = !gRotate;
  }
  //gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE( gPatterns);
}

void rainbow(int s) 
{
  // FastLED's built-in rainbow generator
  fill_rainbow( gLeds[s], NUM_LEDS, gHue, 7);
}

void rainbowWithGlitter(int s) 
{
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow(s);
  addGlitter(s, 80);
}

void addGlitter(int s, fract8 chanceOfGlitter) 
{
  if( random8() < chanceOfGlitter) {
    gLeds[s][ random16(NUM_LEDS) ] += CRGB::White;
  }
}

void confetti(int s) 
{
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( gLeds[s], NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  gLeds[s][pos] += CHSV( gHue + random8(64), 200, 255);
}

void sinelon(int s)
{
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( gLeds[s], NUM_LEDS, 20);
  int pos = beatsin16( 13, 0, NUM_LEDS-1 );
  gLeds[s][pos] += CHSV( gHue, 255, 192);
}

void bpm(int s)
{
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 62;
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for( int i = 0; i < NUM_LEDS; i++) { //9948
    gLeds[s][i] = ColorFromPalette(palette, gHue+(i*2), beat-gHue+(i*10));
  }
}

void juggle(int s) {
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy( gLeds[s], NUM_LEDS, 20);
  byte dothue = 0;
  for( int i = 0; i < 8; i++) {
    gLeds[s][beatsin16( i+7, 0, NUM_LEDS-1 )] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
}
