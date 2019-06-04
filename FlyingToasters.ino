#include <SPI.h>
#include <Wire.h>
#include "bitmaps.h" // Toaster graphics data is in this header file

#define UPDATE_DELAY  50
#define U8G   // for M5Stick. If comment this, use SSD1306 oled.

#ifdef U8G
  #include <U8g2lib.h>
#else
  #include <Adafruit_GFX.h>
  #include <Adafruit_SSD1306.h>
#endif

#ifdef U8G
  U8G2_SH1107_64X128_F_4W_HW_SPI u8g2(U8G2_R3, 14, /* dc=*/ 27, /* reset=*/ 33);
#else
  #define OLED_DC    27 // OLED control pins are configurable.
  #define OLED_CS    14 // These are different from other SSD1306 examples
  #define OLED_RESET 33 // because the Pro Trinket has no pin 2 or 7.
  // Adafruit_SSD1306 display(128, 64, &SPI, OLED_DC, OLED_RESET, OLED_CS);
  Adafruit_SSD1306 display(128, 64, &Wire, OLED_RESET); 
#endif 

#define N_FLYERS   5 // Number of flying things

struct Flyer {       // Array of flying things
  int16_t x, y;      // Top-left position * 16 (for subpixel pos updates)
  int8_t  depth;     // Stacking order is also speed, 12-24 subpixels/frame
  uint8_t frame;     // Animation frame; Toasters cycle 0-3, Toast=255
} flyer[N_FLYERS];

void setup() {
  Serial.begin(115200);
  randomSeed(analogRead(36));           // Seed random from unused analog input
  #ifdef U8G
    u8g2.begin();
    u8g2.setFont(u8g2_font_6x12_tr); 
    u8g2.setBitmapMode(0);
  #else
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
      Serial.println(F("SSD1306 allocation failed"));
      for(;;); // Don't proceed, loop forever
    }

    display.clearDisplay(); 
    display.display();
    delay(2000); // Pause for 2 seconds
  #endif

  for(uint8_t i=0; i<N_FLYERS; i++) {  // Randomize initial flyer states
    flyer[i].x     = (-32 + random(160)) * 16;
    flyer[i].y     = (-32 + random( 96)) * 16;
    flyer[i].frame = random(3) ? random(4) : 255; // 66% toaster, else toast
    flyer[i].depth = 10 + random(16);             // Speed / stacking order
  }
  qsort(flyer, N_FLYERS, sizeof(struct Flyer), compare); // Sort depths
}

void loop() {
  uint8_t i, f;
  int16_t x, y;
  boolean resort = false;     // By default, don't re-sort depths

  #ifdef U8G
    u8g2.clearBuffer();
  #else  
    display.display();          // Update screen to show current positions
    display.clearDisplay();     // Start drawing next frame
  #endif  


  for(i=0; i<N_FLYERS; i++) { // For each flyer...
    // First draw each item...
    f = (flyer[i].frame == 255) ? 4 : (flyer[i].frame++ & 3); // Frame #
    x = flyer[i].x / 16;
    y = flyer[i].y / 16;

#ifdef U8G
    u8g2.setDrawColor(0);// Black
    u8g2.drawBitmap(x, y, 4, 32, mask[f]);
    u8g2.setDrawColor(1);// Wite
    u8g2.drawBitmap(x, y, 4, 32, img[ f]);
    u8g2.sendBuffer();
#else
    display.drawBitmap(x, y, mask[f], 32, 32, BLACK);
    display.drawBitmap(x, y, img[ f], 32, 32, WHITE);
#endif

    // Then update position, checking if item moved off screen...
    flyer[i].x -= flyer[i].depth * 2; // Update position based on depth,
    flyer[i].y += flyer[i].depth;     // for a sort of pseudo-parallax effect.
    if((flyer[i].y >= (64*16)) || (flyer[i].x <= (-32*16))) { // Off screen?
      if(random(7) < 5) {         // Pick random edge; 0-4 = top
        flyer[i].x = random(160) * 16;
        flyer[i].y = -32         * 16;
      } else {                    // 5-6 = right
        flyer[i].x = 128         * 16;
        flyer[i].y = random(64)  * 16;
      }
      flyer[i].frame = random(3) ? random(4) : 255; // 66% toaster, else toast
      flyer[i].depth = 10 + random(16);
      resort = true;
    }
    
  }

  // If any items were 'rebooted' to new position, re-sort all depths
  if(resort) qsort(flyer, N_FLYERS, sizeof(struct Flyer), compare);
  delay(UPDATE_DELAY);
}

// Flyer depth comparison function for qsort()
static int compare(const void *a, const void *b) {
  return ((struct Flyer *)a)->depth - ((struct Flyer *)b)->depth;
}