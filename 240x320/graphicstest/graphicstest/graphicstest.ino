/***************************************************
  This is our GFX example for the Adafruit ILI9341 Breakout and Shield
  ----> http://www.adafruit.com/products/1651

  Check out the links above for our tutorials and wiring diagrams
  These displays use SPI to communicate, 4 or 5 pins are required to
  interface (RST is optional)
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/


#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"

// For the Adafruit shield, these are the default.
#define TFT_CS        10
#define TFT_RST       9 
#define TFT_DC        8



// Initialize ILI9341 display
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

#define WIDTH tft.width()
#define HEIGHT tft.height()




void setup() {
  Serial.begin(115200);
  tft.begin(); // Initialize the display
    // tft.sendCommand(ILI9341_MADCTL, 0x48); // Swap to RGB



  tft.setRotation(1); // Set rotation (try 0, 1, 2, 3)
  tft.fillScreen(ILI9341_GREEN); // Clear the screen

  // Draw a full-width rectangle
  tft.fillRect(0, 0, WIDTH, HEIGHT, ILI9341_RED); // Should cover the entire screen

  Serial.println(tft.width());
  Serial.println(tft.height());
}

int rot =1;

void loop() {
  // Nothing here
  tft.fillScreen(ILI9341_BLACK);
  tft.setRotation(rot);
  rot = (rot+1)%4;
  tft.fillScreen(ILI9341_DARKGREEN);
  tft.fillCircle(10, 10, 2, ILI9341_RED);
  delay(2000);
  tft.fillRect(0, 0, WIDTH, HEIGHT, ILI9341_RED);

  delay(3000);


}