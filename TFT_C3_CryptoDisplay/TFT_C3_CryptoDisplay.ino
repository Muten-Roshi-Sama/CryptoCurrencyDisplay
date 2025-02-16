//V.1.2-128x160- ESPC3

// 
//
//
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
#include <Adafruit_ST7735.h> 
#include <SPI.h>

// ============== header files ============
#include "settings.h"
#include "bootLogo.h"  // Include the bootLogo image
#include "buttons.h"
// #include "DigitalRainAnim.h"


// =============== LCD CONFIG ===============
#define TFT_CS        10
#define TFT_RST       9 
#define TFT_DC        8
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

#define STARTX 0
#define STARTY 0
#define WIDTH  160
#define HEIGHT  128







// =============== STATE VARIABLES ===============
enum State {INIT, BOOT, CRYPTOVIEW, CANDLECHART };
State state = INIT;

int selectedCoinIndex = 0;    
int lastSelectedCoinIndex = 0;
int infoCyclingIndex = 0;




// =============== TIMING VARIABLES ===============
unsigned long previousMillis = 0;  
const unsigned long interval = 5000;

unsigned long previousDataMillis = 0;  
const unsigned long dataUpdateInterval = 8000;  

unsigned long previousCandleMillis = 0;
const unsigned long candleUpdateInterval = 100000;

unsigned long infoCyclingMillis = 0;
const unsigned long InfoCyclingInterval = 4000;




// =============== BOOT VARIABLES ===============
static unsigned long lastLoadingBar = 0;
unsigned long loadingProgressSpeed = 200;  
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
  tft.initR(INITR_BLACKTAB);  
  tft.setRotation(1);
  tft.fillScreen(ST77XX_BLACK);

  // Boot()
  // screenIndex = 99;
  updateDisplay();   // BOOT
  

  connectToWiFi();
  buttonsInit();


  // digitalRainAnim.init(&tft);
  //   digitalRainAnim.pause();
  //   digitalRainAnim.stop();
  //   tft.fillScreen(TFT_BLACK);
  

  // Time
  timeClient.begin();    // Start NTPClient to get the time
  timeClient.update();  // Set initial time (Optional, but ensures time starts from a known point)
}

             
// ==================== LOOP ======================================
int start=0;
int esp = 0;
void loop() {


  if (!isConnected) connectToWiFi();
  else updateData();
    
    start = millis();
  updateDisplay();
    if(millis()-start>5) Serial.println(millis()-start);


    // Memory Usage
    if(millis()-esp > 1000){
        Serial.print("Free Heap: ");
        Serial.println(ESP.getFreeHeap());
        esp=millis();
    }
    

  



    // digitalRainAnim.loop();
      
    // delay(50);
}
         
// =================GENERAL_FUNCTIONS================================
void connectToWiFi() {
  // lastWifiCheck = millis();
    static int attempts = 0;  // Track connection attempts
    static bool connecting = false;
    static unsigned long lastAttemptTime = 0;
    loadingBar(); //reload loadingBar()
    
    
    // Scan for Networks
    if (!networkScanFlag) {  
        bootLog("Scanning for networks...", bootLogColor, 0);
        numNetworks = WiFi.scanNetworks();
        if (numNetworks == 0) {
            bootLog("No networks found.", ST77XX_RED, 0);
            return;
        } else {
            bootLog(String("Found " + String(numNetworks) + " networks."), bootLogColor, 0);
        }
        
        networkScanFlag = true; 
    }
    loadingBar(); //reload loadingBar()

    // Find and connect to wifi from given list
    if (!connecting) {  
        for (int i = 0; i < sizeof(wifiList) / sizeof(wifiList[0]); i++) {
            for (int j = 0; j < numNetworks; j++) {
                if (WiFi.SSID(j) == wifiList[i].ssid) {

                    bootLog("Connecting to: ", ST77XX_GREEN, 0);
                    bootLog( String(wifiList[i].ssid) + ".", bootLogColor, 1);

                    // String message = String("Connecting to: " + String(wifiList[i].ssid) + ".");
                    // textLength = textWidth(message, tft);
                    // rowNum = (j+1);

                    // Serial.printf("Connecting to SSID: %s\n", wifiList[i].ssid);
                    // bootLog(message, ST77XX_GREEN, 0);

                    // drawText(0,rowNum*6,ST77XX_GREEN ,message);
                    WiFi.begin(wifiList[i].ssid, wifiList[i].password);

                    connecting = true;
                    attempts = 0;
                    lastAttemptTime = millis();  // Start connection timer
                    return;
                }
            }
        }
    }
    loadingBar(); //reload loadingBar()

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
                    bootLog("  Connection Failed.", ST77XX_RED, 2);
                    // drawText(textLength ,rowNum*6,ST77XX_RED,"  Failed to connect.");
                    connecting = false;
                }
            }
        }
    }

    // Data Fetch when connected (ONLY ONCE)
    if(isConnected){
      updateALLCoins();
      fetchHistoricalData(selectedCoinIndex);
    }


}



//=================================== FSM =================================================
void updateData(){
  //"updateCandle"
    if(millis() - previousCandleMillis >= candleUpdateInterval){
        fetchHistoricalData(selectedCoinIndex);
        previousCandleMillis = millis();
        Serial.println("Fetched historical data.");
    }

  //"updateCoins"
    if (millis() - previousDataMillis > dataUpdateInterval){
        updateALLCoins();
        previousDataMillis = millis();
        Serial.println("Updated all coins.");
    }
}
void updateDisplay() {
  switch (state) {
    case INIT :
      boot();
      // screenIndex = 98;
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
          singleCryptoView();
        } 
        else if(button2Update){
          button2Update = false;
          selectedCoinIndex = (selectedCoinIndex + 1) % numCoins;
          cryptoView();              // TODO : no need to redraw all when only moving the cursor
          
        }

      // Time Off Update
        if (millis() - previousMillis >= interval) cryptoView(); 
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
          cryptoView();
        } 
        break;
}
}

  





//   // BOOT
//   if(!isLoadingFinished()){
//     myFSM();
//     return;
//   } 

//   // Primary Tasks
//   if (lastScreenIndex != screenIndex){
//       myFSM();
//         lastScreenIndex = screenIndex;  // Update Index
//   } //toUpdate = 0;   //"ScreenToggleUpdate"
//   else if (cursorButtonFlag){
//       myFSM();
//         cursorButtonFlag = false;
//         Serial.println("Cursor moved.");
//   } //toUpdate = 1;           //"CursorUpdate"

//   yield();

//   int toUpdate = -1;
//   unsigned long currentMillis = millis();
  
//   // Assign the highest priority task first
//   if (currentMillis - previousMillis >= interval) toUpdate = 2; //"timeOffUpdate"
//   else if (currentMillis - previousDataMillis > dataUpdateInterval && isConnected) toUpdate = 3; //"updateCoins"
//   else if (currentMillis - previousCandleMillis >= candleUpdateInterval && isConnected) toUpdate = 4; //"updateCandleChart"
//   else if (currentMillis - infoCyclingMillis >= InfoCyclingInterval && screenIndex == 0) toUpdate = 5; //"screen1_InfoCycling"

//   // if(InterruptTask())return;

//   switch (toUpdate) {
//   // BUTTONS UPDATE
//     case 0://"ScreenToggleUpdate":  // B1 : change screens
//       myFSM();
//         lastScreenIndex = screenIndex;  // Update Index
        
//       break;

//     case 1://"CursorUpdate":  // B2 : Cursor button pressed
//       myFSM();
//         cursorButtonFlag = false;
//         Serial.println("Cursor moved.");
//       break;
//   // TIME-OFF UPDATE
//     case 2://"timeOffUpdate":
//       myFSM();
//         previousMillis = currentMillis;
//         Serial.println("Screen updated via timer.");
//       break;

  


//   // DATA UPDATE
//     case 3://"updateCoins":
//       updateALLCoins();
//         previousDataMillis = currentMillis;
//         Serial.println("Updated all coins.");
//       break;

//     case 4://"updateCandleChart":
//       fetchHistoricalData(selectedCoinIndex);
//         previousCandleMillis = currentMillis;
//         Serial.println("Fetched historical data.");
//       break;

//   // SCREEN INFO Cycling : Battery/Wifi/Time Cycling
//     case 5://"screenInfoCycling":
//       infoCyclingIndex = (infoCyclingIndex + 1) % 2;
//       infoCyclingMillis = currentMillis;
//       displayTitle();
//       Serial.println("Battery/WiFi/Time cycled.");
//       break;


//     default:
//       // Serial.println("PASS");
//       break;
//   }
// }
// void myFSM(){
//   // Serial.print("SreenIndex-FSM: ");
//   // Serial.println(screenIndex);
//   // readButtons();
//   yield();

//   switch (screenIndex) {
//       case 0: 
//         cryptoView();  // Coin info screen
//         break;
//       case 1: 
//         singleCryptoView();  // Single crypto info screen
//         break;


//       case 97:
//         // digitalRainAnim.loop();
//         break;
        
//       case 98:
//         loadingBar();
//         // Serial.println("OK2");
//         if(isConnected && loadingFinished)screenIndex = 0;
//         break;
//       case 99:  
//         boot();
//         screenIndex = 98;
//         break;
//       default: 
//         break;
//     }
// }


//===================================BOOT()=================================================
void boot() {
  tft.fillScreen(ST77XX_BLACK);
    int xPos = (160 - 128) / 2;//(tft.width() - IMG_WIDTH) / 2;
    int yPos = 0;

    tft.drawRGBBitmap(xPos, yPos, bootLogo, img_width, img_height);
    loadingBar();
}
// Non-blocking loading bar function (15 segments)
void loadingBar() {
  // ScreenWidth= 160 = barWidth*100 + 2*batterySides + 2 Padding

    // int barNum = 100;
    // int batterySide = 1; //battery white sides
    // int barWidth = 
    // int startX = //round(WIDTH * 1/3.5);  //160pixels * 1cm/3.5cm = 46
    // int endX = WIDTH - startX;

    int startY = 105;  // don't touch
    int startX = 30;  //padding
    int endY = 115;
    int endX = 130;


    int barNum = 30;  // Total number of bars (segments)
    int barHeight = (endY - startY) - 2;  // Height of the progress bar
    int barWidth = (endX - startX) * -1;  // Width of the progress bar (75% of the rectangle width)
    
    if(currentProgress > barNum)loadingFinished = true;
    //  if(isLoadingFinished()) screenIndex = 0;
    // Draw progress bar
    else {
        tft.drawRect(startX, startY, endX - startX + 1, endY - startY, ST77XX_WHITE);  // Draw outer rectangle of the bar
        if (millis() - lastLoadingBar >= loadingProgressSpeed){
            lastLoadingBar = millis();  // Update the time when the progress is updated
            int barSegmentWidth = barWidth / barNum;  // Width of each bar segment
            // loadingProgressSpeed *= 0.8;

            for(int i=0; i<7; i++){
              // Draw the next segment in the progress bar
              tft.fillRect(startX + 1 + currentProgress * (barSegmentWidth + 1), startY + 1, barSegmentWidth, barHeight, ST77XX_GREEN);  
              Serial.print("Printing a load bar : ");
              Serial.println(currentProgress);
              currentProgress += 1;  // Move to the next segment
            }
            
        }
    }


}  
bool isLoadingFinished() {
    return loadingFinished;
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

  int startY = Row * textHeight("A", tft);
  int endY = startY + textHeight("A", tft);

  // CLEAR
  tft.fillRect(0, startY, WIDTH, endY, ST77XX_BLACK);

  // WRITE
  drawText(0, startY, textColor, text);

} // MAX 4 ROWS



//------Helpers-------
                          // ALL TFT_BLACK etc need to be updated as ST77XX_BLACK (same for all colors).
int textWidth(String text, Adafruit_GFX &tft) {
  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(text.c_str(), 0, 0, &x1, &y1, &w, &h);
  return w;
}
int textHeight(String text, Adafruit_GFX &tft) {
    int16_t x1, y1;
    uint16_t w, h;
    tft.getTextBounds(text.c_str(), 0, 0, &x1, &y1, &w, &h);
    return h;  // Return the computed text height
}




