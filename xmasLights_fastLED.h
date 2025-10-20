#ifndef XMASLIGHTS_FASTLED_H
#define XMASLIGHTS_FASTLED_H

#include <FastLED.h>

#define DATA_PIN 22
#define LED_TYPE WS2811
#define COLOR_ORDER GRB
#define NUM_LEDS 16

#define ARRAY_SIZE_FASTLED(A) (sizeof(A) / sizeof((A)[0]))

#define WS2812_PIN DATA_PIN

void setupXmasLights();
void xmasLightsLoop();
void chooseNextColorPalette( CRGBPalette16& pal);
uint8_t attackDecayWave8( uint8_t i);
void coolLikeIncandescent( CRGB& c, uint8_t phase);
CRGB computeOneTwinkle( uint32_t ms, uint8_t salt);

// Forward declarations for pattern functions
void rainbow();
void rainbowWithGlitter();
void confetti();
void sinelon();
void juggle();
void bpm();
void nextPattern();
void addGlitter(fract8 chanceOfGlitter);
void fairyS();
void alt2colorsRedBlue();
void alt2colorsRedGreen();
void alt2colorsBlueGreen();
void fadeRed();
void fadeBlue();
void fadeGreen();
void fadeWhite();
void fadeYellow();
void drawTwinkles(CRGBSet& L);

#endif