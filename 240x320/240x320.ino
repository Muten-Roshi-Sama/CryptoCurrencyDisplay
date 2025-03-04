//V.1.0-240x320- ESPC3



// Notes :
//        - at start we update All coins, then one every 5s
//        - we only store 1 candle chart at the time, when changing screens with a different coin selected, the old coin data is overwritten with the new.
//        - we could use SPIFFs for long term storage and OFFLINE use...
//        - to adapt to ILI9341 : swap Width and height definitions
//                                in setRotation change MADCTL_BGR to MADCTL_RGB


// PINOUT:
// Vcc - 3v3,  GND-GND,  CS-10,  RST-9,  AO/DC-8,  SDA-6,  SCK-4,  LED/BLK-Vcc



// TODO: 
//      - Fix lagging after button press
//      - Add analogRead Battery sensor or create battery estimation
//      - Find how to upload wifi on esp... without accessing the code. (SPIFFS)
//      - 
//      - 
//    CryptoView:
//      - Improve wifi display
//      - Improve Cursor Display
//      - 
//      - 
//    SingleCryptoView:
//      - Fix Candles Display
//      - 


      


// =============== LIBRARIES ===============
#include <Arduino.h>
#include <ArduinoJson.h>
#include <EasyButton.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <HTTPClient.h>
#include <esp_http_client.h>
#include <Adafruit_GFX.h>    
// #include <Adafruit_ST7735.h> 
#include <Adafruit_ILI9341.h>
#include <SPI.h>
#include <SPIFFS.h>

// ============== header files ============
#include "settings.h"
#include "bootLogo.h"  // Include the bootLogo image
#include "buttons.h"
// #include "DigitalRainAnim.h"


// =============== LCD CONFIG ===============
#define TFT_CS        10
#define TFT_RST       9 
#define TFT_DC        8
// Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

#define STARTX 0
#define STARTY 0
#define WIDTH  320
#define HEIGHT  240







// =============== STATE VARIABLES ===============
enum State {INIT, BOOT, CRYPTOVIEW, CANDLECHART };
State state = INIT;

enum UpdateState { IDLE, UPDATING_CANDLE, UPDATING_COINS };
UpdateState updateState = IDLE;


int selectedCoinIndex = 0;    
int lastSelectedCoinIndex = 0;
int infoCyclingIndex = 0;




// =============== TIMING VARIABLES ===============
unsigned long previousMillis = 0;  
const unsigned long interval = 5000;

unsigned long previousDataMillis = 0;  
const unsigned long dataUpdateInterval = 5000;  

unsigned long previousCandleMillis = 0;
const unsigned long candleUpdateInterval = 100000;

unsigned long infoCyclingMillis = 0;
const unsigned long InfoCyclingInterval = 4000;

unsigned long debugMillis = 0;
const unsigned long debugInterval = 1000;

int start=0;
int esp = 0;


// =============== BOOT VARIABLES ===============
static unsigned long lastLoadingBar = 0;
unsigned long loadingProgressSpeed = 500;  
int currentProgress = 0;  
bool loadingFinished = false;  

//Secret
// DigitalRainAnim digitalRainAnim = DigitalRainAnim(); 


// need to be after variables declarations
#include "cryptoView.h"   // Here some functions uses textHeight which is defined at the end of the .ino
#include "candleChartView.h"
               
// =================== SETUP FUNCTION ==============================
void setup() {
  Serial.begin(115200);
  
  // TFT
   tft.begin();
   
  tft.setRotation(4);
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextWrap(false);  // Disable text wrapping

  // BOOT
  updateDisplay();   
  

  connectToWiFi();
  buttonsInit();


  // SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("Failed to mount SPIFFS");
    return;
  }
  Serial.println("SPIFFS mounted successfully");

  // digitalRainAnim.init(&tft);
  //   digitalRainAnim.pause();
  //   digitalRainAnim.stop();
  //   tft.fillScreen(TFT_BLACK);
  

  // Time
  timeClient.begin();    // Start NTPClient to get the time
  timeClient.update();  // Set initial time (Optional, but ensures time starts from a known point)
}

             
// ==================== LOOP ======================================

void loop() {


  if (isConnected){
    updateDisplay();
    updateData();
  } else connectToWiFi();
  


    // Memory Usage
    if(millis()-esp > 1000){
        Serial.print("Free Heap: ");
        Serial.println(ESP.getFreeHeap());
        esp=millis();
    }
    
  // serialDebugPrints
  // if(millis()-debugMillis > 1000){
  //   Serial.print("State = ");
  //   Serial.print(state);
  //   Serial.print(", WiFi: ");
  //   Serial.println(isConnected);


  //   debugMillis = millis();
  // }


    // digitalRainAnim.loop();
      
    // delay(50);
}
         
// =================GENERAL_FUNCTIONS================================
void connectToWiFi() {
  bool debug=true;
  // lastWifiCheck = millis();
    static int attempts = 0;  // Track connection attempts
    static bool connecting = false;
    static unsigned long lastAttemptTime = 0;
    loadingBar(); //reload loadingBar()
    
    if(debug){
      Serial.print("Wifi - connecting: ");
      Serial.print(connecting);
      Serial.print(" ,");
    }

    // Check if Wifi are saved or not.
    if (sizeof(wifiList) / sizeof(wifiList[0]) == 0) {
      Serial.println("No Wifi saved.");
      bootLog("No Wifi saved...", ILI9341_RED, 0);
      return;
    }
   
    // Scan for Networks
    if (!networkScanFlag) {  
        bootLog("Scanning for networks...", bootLogColor, 0);
        numNetworks = WiFi.scanNetworks();
        if (numNetworks == 0) {
            bootLog("No networks found.", ILI9341_RED, 0);
            return;
        } else {
            bootLog(String("Found " + String(numNetworks) + " networks."), bootLogColor, 0);
        }
        
        networkScanFlag = true; 
    }
    loadingBar();

    // Find and connect to wifi from given list
    if (!connecting) {  
        for (int i = 0; i < sizeof(wifiList) / sizeof(wifiList[0]); i++) {
            for (int j = 0; j < numNetworks; j++) {

                if (WiFi.SSID(j) == wifiList[i].ssid) {
                    bootLog("Connecting to: ", bootLogColor, 0);
                    bootLog( String(wifiList[i].ssid) + ".", bootLogColor, 1);

                    WiFi.begin(wifiList[i].ssid, wifiList[i].password);

                    connecting = true;
                    attempts = 0;
                    lastAttemptTime = millis();  // Start connection timer
                    return;
                }
                else {
                  bootLog("WiFi not found.", ILI9341_RED, 2);
                }
            }
        }
    }
    loadingBar(); 

    // Attempt
    if (connecting) {
        // Check connection every 500ms without blocking
        if (millis() - lastAttemptTime >= 2000) {
            lastAttemptTime = millis(); 
            if (WiFi.status() == WL_CONNECTED) {
                bootLog("Connected!", bootLogColor, 0);
                
                Serial.print("IP Address: ");
                Serial.println(WiFi.localIP());
                isConnected = true;
                connecting = false;
                
            } else {
                Serial.print(".");
                attempts++;
                if (attempts >= 10) {  // Try for 10 seconds max (10 * 1000ms)
                    // Serial.println("\nFailed to connect.");
                    bootLog(" Wifi Connection Failed.", ILI9341_RED, 2);
                    // drawText(textLength ,rowNum*6,ILI9341_RED,"  Failed to connect.");
                    connecting = false;
                }
            }
        }
    }

    // Data Fetch when connected (ONLY ONCE)
    if(isConnected){
      Serial.print("Fetching Data...");
      bootLog("Fetching Data...", bootLogColor, 0);
      updateALLCoins();
      fetchHistoricalData(selectedCoinIndex);
    }


}



//=================================== FSM =================================================

void updateData() {
  if (millis() - previousDataMillis >= dataUpdateInterval) {
    previousDataMillis = millis();

    switch (updateState) {
      case IDLE:
        updateState = UPDATING_COINS; // Move to the next task
        break;

      case UPDATING_COINS:
        updateNextCoin();
        updateState = UPDATING_CANDLE; // Move to the next task
        break;

      case UPDATING_CANDLE:
        fetchHistoricalData(selectedCoinIndex);
        updateState = UPDATING_COINS; // Cycle back to the first task
        break;

      // Add more cases for additional tasks here
    }
  }
}



void updateDisplay() {
    int start = millis();

  switch (state) {
    case INIT :
      boot();
      state = BOOT;
      break;

    case BOOT:
        loadingBar();
        if(isConnected && loadingFinished){
          state = CRYPTOVIEW;
          cryptoView();
        }
        break;

    case CRYPTOVIEW:
      // BUTTONS
        if(button1Update){
          button1Update = false;
          state = CANDLECHART;
          // singleCryptoView();
          previousMillis = 0;    // force refresh
        } 
        else if(button2Update){
          button2Update = false;
          selectedCoinIndex = (selectedCoinIndex + 1) % numCoins;
          // cryptoView();              // TODO : no need to redraw all when only moving the cursor
          previousMillis = 0;    // force refresh
          infoCyclingMillis = millis();   // prevent double print
          
        }

      // Time Off Update
        if (millis() - previousMillis >= interval){
          cryptoView(); 
          previousMillis = millis();

        }
      // Battery Time Cycling
        if (millis() - infoCyclingMillis >= InfoCyclingInterval){
          infoCyclingIndex = (infoCyclingIndex + 1) % 2;
          infoCyclingMillis = millis();
          displayTitle();
          Serial.println("Battery/WiFi/Time cycled.");
        }
      break;


    case CANDLECHART:
      // BUTTONS
        if(button1Update){
          button1Update = false;
          state = CRYPTOVIEW;
          // cryptoView();
          previousMillis = 0;    // force refresh
          infoCyclingMillis = millis();   // prevent double print
        } 

      // Time Off Update
        if (millis() - previousMillis >= interval){
          singleCryptoView(); 
          previousMillis = millis();
        } 


        break;
  }

  if(millis()-start>100) {
      Serial.print("Long run time : ");
      Serial.println(millis()-start);  // print run time if function is blocking.
    }
}

  



//===================================BOOT()=================================================
void boot() {
  tft.fillScreen(ILI9341_BLACK);
    int xPos = (WIDTH - HEIGHT) / 2;//(tft.width() - IMG_WIDTH) / 2;
    int yPos = 0;

    tft.drawRGBBitmap(xPos, yPos, bootLogo, img_width, img_height);
    loadingBar();
}
// Non-blocking loading bar function (15 segments)
void loadingBar() {
  // previous dimensions : 160x128 to 320x240 so factors x2 and x1.875
    int fX = 2;
    int fY = 1.8;
    int startY = HEIGHT *3/4;  // Don't touch
    int startX = 30;   // Padding
    int endY = startY + 20;
    int endX = startX+ (WIDTH - 2*startX);

    int barHeight = (endY - startY) - 2;  // Height of the progress bar
    int barWidth = (endX - startX);       // Total width of the progress bar

    // Serial.print("Current Progress : ");
    // Serial.println(currentProgress);
    
    if(isConnected) loadingProgressSpeed = 1;

    // Calculate the current progress width
      int progressWidth = map(currentProgress, 0, 10, startX, barWidth); // map currentProgress value from 0-100 to startX-barWidth.
      Serial.print("progressWidth : ");
      Serial.println(progressWidth);

    // Stop drawing if progress reaches 80% and not connected
        // if (progressWidth >= 0.8 * barWidth) {
        //     if (!isConnected) {
        //         // Serial.println("Progress paused at 80% - waiting for connection.");
        //         return;  // Exit the function without incrementing last 80% progress (wifi not connected).
        //     }
        // }

    // Check if loading is finished
      if (progressWidth == barWidth){
        if(isConnected) loadingFinished = true;
        else currentProgress = 0;

      }

    // Clamp the progress width to stay within bounds
      // if (progressWidth > barWidth) {
      //     progressWidth = barWidth;
      // }

    // Draw the outer rectangle of the bar
      tft.drawRect(startX -1, startY -1, barWidth + 1, endY - startY, ILI9341_WHITE);

    // PROGRESS
      if (millis() - lastLoadingBar >= loadingProgressSpeed) {
          lastLoadingBar = millis();  // Update the time when the progress is updated

          // Draw the continuous progress bar
          tft.fillRect(
              startX,              // X position (start at the left edge)
              startY,              // Y position
              progressWidth,           // Width of the progress bar
              barHeight,               // Height of the progress bar
              ILI9341_GREEN             // Color
          );

          // Increment progress
          currentProgress += 1;      
      }
}













// =================================================
//                 DISPLAY FUNCTIONS
// =================================================
void drawText(int x, int y, int textColor, String text) {
    tft.setCursor(x, y);
    tft.setTextColor(textColor);
    tft.println(text);
}
void bootLog(String text, int textColor, int Row){
  Serial.println(text);
  tft.setTextSize(2);

  int margin = 2;
  int startY = Row * textHeight("A", tft, 2)+margin;
  int endY = startY + textHeight("A", tft, 2) + Row*margin;

  // CLEAR
  tft.fillRect(0, startY - margin, WIDTH, endY, ILI9341_BLACK);

  // WRITE
  drawText(0, startY, textColor, text);

} // MAX 4 ROWS



//------Helpers-------
int textWidth(String text, Adafruit_GFX &tft, int textSize) {
  int16_t x1, y1;
  uint16_t w, h;
  tft.setTextSize(textSize);
  tft.getTextBounds(text.c_str(), 0, 0, &x1, &y1, &w, &h);
  return w;
}
int textHeight(String text, Adafruit_GFX &tft, int textSize) {
    int16_t x1, y1;
    uint16_t w, h;
    tft.setTextSize(textSize);
    tft.getTextBounds(text.c_str(), 0, 0, &x1, &y1, &w, &h);
    return h;  // Return the computed text height
}




