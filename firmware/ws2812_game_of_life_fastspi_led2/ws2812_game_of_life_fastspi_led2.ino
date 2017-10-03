//#include <FastSPI_LED.h>
#include "FastLED.h"

/*
 * This code base is from noisebridge's website
 * and I cannot rememer the exact URL
 * need to find it out :)
 */

#define ROWS 12
#define COLS 12
#define NUM_LEDS ROWS*COLS

#define WATCHDOG_ROUNDS 10
#define WATCHDOG_DELAY 10

#define MAX_LIFE 30

#define WRAP

// Sometimes chipsets wire in a backwards sort of way
//struct CRGB { unsigned char g; unsigned char r; unsigned char b; };
// struct CRGB { unsigned char r; unsigned char g; unsigned char b; };
CRGB leds[NUM_LEDS];

#define DATA_PIN 9

static signed char state[ROWS][COLS];
static signed char next_state[ROWS][COLS];
static signed char save_state[ROWS][COLS];

static unsigned char led_map[ROWS][COLS] = {
{35, 36, 43, 44, 51, 52, 59, 60, 131, 132, 139, 140},
{34, 37, 42, 45, 50, 53, 58, 61, 130, 133, 138, 141},
{33, 38, 41, 46, 49, 54, 57, 62, 129, 134, 137, 142},
{32, 39, 40, 47, 48, 55, 56, 63, 128, 135, 136, 143},

{19, 20, 27, 28, 67, 68, 75, 76, 115, 116, 123, 124},
{18, 21, 26, 29, 66, 69, 74, 77, 114, 117, 122, 125},
{17, 22, 25, 30, 65, 70, 73, 78, 113, 118, 121, 126},
{16, 23, 24, 31, 64, 71, 72, 79, 112, 119, 120, 127},

{3, 4, 11, 12, 83, 84, 91, 92, 99, 100, 107, 108},
{2, 5, 10, 13, 82, 85, 90, 93, 98, 101, 106, 109},
{1, 6,  9, 14, 81, 86, 89, 94, 97, 102, 105, 110},
{0, 7,  8, 15, 80, 87, 88, 95, 96, 103, 104, 111},
};

static void set_led(int row, int col, signed char life) {
  struct CRGB *led;

  led = &leds[led_map[row][col]];
  
  switch(life){
    case 0:
      led->r = 0;
      led->g = 0;
      led->b = 0;
      break;
    case 1:
      led->r = 0;
      led->g = 40;
      led->b = 0;
      break;
    case 2:
      led->r = 40;
      led->g = 0;
      led->b = 0;
      break;
    case MAX_LIFE:
      led->r = 40;
      led->g = 40;
      led->b = 40;
      break;
    default:
      if (life < 0) {
        led->r = led->r/2;
        led->g = led->g/2;
        led->b = led->b/2;
        break;
      }
      if (life > MAX_LIFE-5){
        led->r = 64;
        led->g = 64;
        led->b = 64;
        break;
      }
      led->r = (40*life)/MAX_LIFE;
      led->g = (40*life)/MAX_LIFE;
      led->b = 40;
      break;
  }
}

static boolean is_dead(signed char state) {
  return (state <= 0);
}

static void gol_init(){
  for(int row = 0; row < ROWS; ++row) {
    for(int col = 0; col < COLS; ++col) {
      int r = random(0,2);
      if (r%2)
        state[row][col] = random(1,MAX_LIFE);
      else
        state[row][col] = 0;
    }
  }
  memcpy(save_state, state, NUM_LEDS);
}

static void watchdog() {
  static int diff_count = 0;
  static bool reset = false;

  if(reset) {
    static int reset_delay = 0;
    ++reset_delay;
    if(reset_delay >= WATCHDOG_DELAY){
      reset_delay = 0;
      reset = false;
      diff_count = 0;
      gol_init();
      return;
    }
  }

  if(!memcmp(save_state, next_state, NUM_LEDS)) {
    reset = true;
    return;
  }

  ++diff_count;
  if(diff_count >= WATCHDOG_ROUNDS) {
    diff_count = 0;
    memcpy(save_state, next_state, NUM_LEDS);
  }
}

void setup()
{
  FastLED.addLeds<WS2811, DATA_PIN, GRB>(leds, NUM_LEDS);
  
  FastLED.show();
  
  randomSeed(analogRead(0));
  
  memset(state, 0, NUM_LEDS);
  memset(next_state, 0, NUM_LEDS);
  memset(save_state, 0, NUM_LEDS);
  gol_init();
}

static int gol_num_neighbors(int row, int col){
  int count = 0;
  for(int i = row-1; i <= row+1; ++i){
    for(int j = col-1; j <= col+1; ++j){
#ifndef WRAP
      if (i < 0 || j < 0)
        continue;
      if (i >= ROWS || j >= COLS)
        continue;
#endif
      if (i == row && j == col)
        continue;
      if (!is_dead(state[i][j]))
        ++count;
    }
  }
  return count;
}
                          
static void gol_handle(int row, int col){
  int neighbor_count = gol_num_neighbors(row, col);
  if (is_dead(state[row][col])){
    if (neighbor_count == 3)
      next_state[row][col] = 1;
    else
      next_state[row][col] = state[row][col];
    return;
  }

  if (neighbor_count < 2 || neighbor_count > 3)
    next_state[row][col] = -1 * state[row][col];
  else {
    next_state[row][col] = state[row][col]+1;
    if(next_state[row][col] > MAX_LIFE)
      next_state[row][col] = MAX_LIFE;
  }
}

static void gol_draw(){
  for(int row = 0; row < ROWS; ++row) {
    for(int col = 0; col < COLS; ++col) {
      set_led(row, col, state[row][col]);
    }
  }
  FastLED.show();
  delay(analogRead(0)/10); // I divided this delay by 10 to speed things up... presumably by a factor of 10
  for(int row = 0; row < ROWS; ++row) {
    for(int col = 0; col < COLS; ++col) {
      set_led(row, col, state[row][col]);
    }
  }
  FastLED.show();
  delay(analogRead(0)/10);
  for(int row = 0; row < ROWS; ++row) {
    for(int col = 0; col < COLS; ++col) {
      set_led(row, col, state[row][col]);
    }
  }
  FastLED.show();
  delay(analogRead(0)/10);
  memcpy(state, next_state, NUM_LEDS);
  watchdog();
}

void loop() { 
  for(int row = 0; row < ROWS; ++row) {
    for(int col = 0; col < COLS; ++col) {
      gol_handle(row, col);
    }
  }
  gol_draw();
}
