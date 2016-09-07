// Fibonacci Clock
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License Version 2
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
// USA.
//
// You will find the latest version of this code at the following address:
// https://github.com/pchretien/fibo
//
// This project contains code and libraries provided by Adafruit Industries and
// can be found on their Github account at:
// https://github.com/adafruit
//
// Credits:
// See the credit.txt file for the list of all the backers of the Kickstarter
// campaign.
// https://www.kickstarter.com/projects/basbrun/fibonacci-clock-an-open-source-clock-for-nerds-wit/description

#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include "RTClib.h"

#define STRIP_PIN 8
#define HOUR_PIN 3
#define MINUTE_PIN 4
#define BTN_PIN 5
#define SET_PIN 6

#define DEBOUNCE_DELAY 10
#define MAX_BUTTONS_INPUT 20
#define CLOCK_PIXELS 5

#define TIME_CHANGED 1
#define COLOR_CHANGED 2
#define MODE_CHANGED 4

// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(9, STRIP_PIN, NEO_RGB + NEO_KHZ800);

// These are the color pallets for the standard clock. Adding removing or
// changing these will work out of the box.
int palette = 0;
uint32_t colors[][4] = {{
                            // #1 RGB
                            strip.Color(255, 255, 255),  // off
                            strip.Color(255, 10, 10),    // hours
                            strip.Color(10, 255, 10),    // minutes
                            strip.Color(10, 10, 255)     // both;
                        },
                        {
                            // #2 Mondrian
                            strip.Color(255, 255, 255),  
                            strip.Color(255, 10, 10),
                            strip.Color(248, 222, 0),    
                            strip.Color(10, 10, 255)     
                        },
                        {
                            // #3 Basbrun
                            strip.Color(255, 255, 255),  
                            strip.Color(80, 40, 0),      
                            strip.Color(20, 200, 20),    
                            strip.Color(255, 100, 10)    
                        },
                        {
                            // #4 80's
                            strip.Color(255, 255, 255),  
                            strip.Color(245, 100, 201),  
                            strip.Color(114, 247, 54),   
                            strip.Color(113, 235, 219)   
                        },
                        {
                            // #5 Pastel
                            strip.Color(255, 255, 255),  
                            strip.Color(255, 123, 123),  
                            strip.Color(143, 255, 112),  
                            strip.Color(120, 120, 255)   
                        },
                        {
                            // #6 Modern
                            strip.Color(255, 255, 255),  
                            strip.Color(212, 49, 45),    
                            strip.Color(145, 210, 49),   
                            strip.Color(141, 95, 224)    
                        },
                        {
                            // #7 Cold
                            strip.Color(255, 255, 255),  
                            strip.Color(209, 62, 200),   
                            strip.Color(69, 232, 224),   
                            strip.Color(80, 70, 202)     
                        },
                        {
                            // #8 Warm
                            strip.Color(255, 255, 255),  
                            strip.Color(237, 20, 20),    
                            strip.Color(246, 243, 54),   
                            strip.Color(255, 126, 21)    
                        },
                        {
                            // #9 Earth
                            strip.Color(255, 255, 255),  
                            strip.Color(70, 35, 0),      
                            strip.Color(70, 122, 10),    
                            strip.Color(200, 182, 0)     
                        },
                        {
                            // #10 Dark
                            strip.Color(255, 255, 255),  
                            strip.Color(211, 34, 34),    
                            strip.Color(80, 151, 78),    
                            strip.Color(16, 24, 149)    
                        }};

// These are the different modes. A mode is just a function that takes a
// boolean for whether or not it's a first call, and then optionally modifies
// the pixels of the clock.
void display_current_time(byte);
void rainbow_cycle(byte);
void rainbow(byte);
void (*modes[])(byte) = {display_current_time, rainbow_cycle, rainbow};

// ------------------------
// Code for displaying time
// ------------------------
RTC_DS1307 rtc;
byte old_hours = 0;
byte old_minutes = 0;
byte bits[CLOCK_PIXELS];
byte fibs[CLOCK_PIXELS] = {1, 1};
byte cum_fibs[CLOCK_PIXELS] = {0, 1, 2};
byte total = 2;

void display_current_time(byte changed) {
  DateTime now = rtc.now();
  byte hours = now.hour() % 12;
  byte minutes = now.minute() / 5;

  if (!changed && old_hours == hours && old_minutes == minutes) {
    return;
  }

  old_hours = hours;
  old_minutes = minutes;

  for (int i = 0; i < CLOCK_PIXELS; i++) {
    bits[i] = 0;
  }

  set_bits(hours, 1);
  set_bits(minutes, 2);

  for (int i = 0; i < CLOCK_PIXELS; i++) {
    set_pixel(i, colors[palette][bits[i]]);
  }
  strip.show();
}

// TODO This doesn't uniformly select over possible configurations, but it is
// /much/ simpler than the old code for assigning bits
void set_bits(byte value, byte offset) {
  for (int i = CLOCK_PIXELS - 1; i >= 0; --i) {
    if (value > cum_fibs[i] || (value >= fibs[i] && random(2))) {
      bits[i] |= offset;
      value -= fibs[i];
    }
  }
}

// ----------------
// Code or Rainbows
// ----------------
uint16_t j;

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t wheel(byte wheel_pos) {
  if (wheel_pos < 85) {
    return strip.Color(wheel_pos * 3, 255 - wheel_pos * 3, 0);
  } else if (wheel_pos < 170) {
    wheel_pos -= 85;
    return strip.Color(255 - wheel_pos * 3, 0, wheel_pos * 3);
  } else {
    wheel_pos -= 170;
    return strip.Color(0, wheel_pos * 3, 255 - wheel_pos * 3);
  }
}

void rainbow(byte changed) {
  uint16_t i;
  if (changed & MODE_CHANGED) {
      j = 0;
  }
  j %= 256;

  for (i = 0; i < CLOCK_PIXELS; i++) {
      set_pixel(i, wheel((i + j) % 256));
  }

  strip.show();
  delay(20);
  j += 1;
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbow_cycle(byte changed) {
  uint16_t i;
  if (changed & MODE_CHANGED) {
      j = 0;
  }
  j %= 256;

  // 5 cycles of all colors on wheel
  for (i = 0; i < CLOCK_PIXELS; i++) {
      set_pixel(i, wheel(((i * 256 / CLOCK_PIXELS) + j) % 256));
  }
  strip.show();
  delay(20);
  j += 1;
}

// -----------------------
// Code for main operation
// -----------------------
byte pixel_inds[CLOCK_PIXELS + 1] = {0, 1, 2, 3, 5, 10};

int last_button_value[MAX_BUTTONS_INPUT];
int current_button_value[MAX_BUTTONS_INPUT];
boolean on = true;
int mode = 0;

void setup() {
  Serial.begin(9600);

  // Initialize the strip and set all pixels to 'off'
  strip.begin();
  strip.show();

  Wire.begin();
  rtc.begin();

  if (!rtc.isrunning()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // Make the random() function return unpredictable results
  randomSeed(rtc.now().unixtime());

  pinMode(HOUR_PIN, INPUT);
  pinMode(MINUTE_PIN, INPUT);
  pinMode(BTN_PIN, INPUT);
  pinMode(SET_PIN, INPUT);

  pinMode(13, OUTPUT);

  for (int i = 0; i < 4; i++) {
    digitalWrite(13, HIGH);
    delay(250);
    digitalWrite(13, LOW);
    delay(250);
  }

  // Compute important parts of the Fibonacci sequence
  for (int index = 2; index < CLOCK_PIXELS; ++index) {
    fibs[index] = fibs[index - 1] + fibs[index - 2];
    if (index + 1 < CLOCK_PIXELS) {
      total += fibs[index];
      cum_fibs[index + 1] = total;
    }
  }

  // Make sure the time is always displayed the first
  // time the clock is powered on.
  mode = 0;
  old_hours = 99;
}

void loop() {
  // Read buttons
  int set_button = debounce(SET_PIN);
  int hour_button = debounce(HOUR_PIN);
  int minute_button = debounce(MINUTE_PIN);
  int button = debounce(BTN_PIN);
  byte changed = 0;

  if (set_button && hour_button && has_changed(HOUR_PIN)) {
    DateTime newTime = DateTime(rtc.now().unixtime() + 3600);
    rtc.adjust(newTime);
    mode = 0;
    changed = TIME_CHANGED;

  } else if (set_button && minute_button && has_changed(MINUTE_PIN)) {
    DateTime fixTime = rtc.now();
    // FIXME This could probably be simpler
    DateTime newTime =
        DateTime(fixTime.year(), fixTime.month(), fixTime.day(), fixTime.hour(),
                 ((fixTime.minute() - fixTime.minute() % 5) + 5) % 60, 0);
    rtc.adjust(newTime);
    mode = 0;
    changed = TIME_CHANGED;

  } else if (minute_button && has_changed(MINUTE_PIN)) {
    toggle_on_off();

  } else if (hour_button && has_changed(HOUR_PIN)) {
    palette = (palette + 1) % (sizeof(colors) / sizeof(*colors));
    changed = COLOR_CHANGED;

  } else if (button && has_changed(BTN_PIN)) {
    mode = (mode + 1) % (sizeof(modes) / sizeof(*modes));
    changed = MODE_CHANGED;
  }

  // Store buttons new values
  reset_button_values();
  modes[mode](changed);
}

int debounce(int pin) {
  int val = digitalRead(pin);
  if (val == last_button_value[pin]) {
    current_button_value[pin] = val;
    return val;
  }

  delay(DEBOUNCE_DELAY);

  val = digitalRead(pin);
  if (val != last_button_value[pin]) {
    current_button_value[pin] = val;
    return val;
  }

  current_button_value[pin] = last_button_value[pin];
  return last_button_value[pin];
}

boolean has_changed(int pin) {
  return last_button_value[pin] != current_button_value[pin];
}

void reset_button_values() {
  for (int i = 0; i < MAX_BUTTONS_INPUT; i++) {
    last_button_value[i] = current_button_value[i];
  }
}

void set_pixel(byte pixel, uint32_t color) {
  if (on) {
    for (byte pix = pixel_inds[pixel]; pix < pixel_inds[pixel + 1]; ++pix) {
      strip.setPixelColor(pix, color);
    }
  }
}

void toggle_on_off() {
  if (on) {
    for (int i = 0; i < CLOCK_PIXELS; i++) {
      set_pixel(i, strip.Color(0, 0, 0));
    }
    strip.show();
  } else {
    mode = 0;
    old_hours = 99;
  }

  on = !on;
}
