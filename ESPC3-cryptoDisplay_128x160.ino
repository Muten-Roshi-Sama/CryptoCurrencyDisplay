//V.1.1-128x160- ESPC3

// Save before Interrupt Driven Buttons Implementation
//
// PINOUT:
// Vcc - 3v3,  GND-GND,  CS-10,  RST-9,  AO/DC-8,  SDA-6,  SCK-4,  LED/BLK-Vcc


//  TODO :
//        - Fix lagging after button press
//        - Fix wifi display
//        - V.Fix time display
//        - Fix CryptoCoin alignement
//        - Upgrade Cursor
//        - 
//        - Fix Price of SingleCryptoView
//        - Fix Candles
//        - 
//        - Find how to upload the wifi to the Microcontroller (wifi, bluetooth, ...)
//        - Find battery level sensor or create battery estimation function.
//        - 
//        - 




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

#include "img/bootLogo.h"  // Include the bootLogo image
#include "DigitalRainAnim.h"

// =============== DISPLAY CONFIG ===============
#define TFT_CS        10
#define TFT_RST       9  // Or set to -1 and connect to Arduino RESET pin
#define TFT_DC        8
#define ST77XX_GREY      0x7BEF
#define ST77XX_GOLD     0xFFD700 //0xFD20   // Gold  
#define ST77XX_SKYBLUE  0x87CEEB // Sky Blue
#define ST77XX_PINK     0xF81F   // Pink
#define ST77XX_GREENYELLOW 0xA8E4   // Green Yellow
#define ST77XX_DARKGREEN 0x03E0
#define ST77XX_VIOLET   0x7F00   // Violet
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// LCD Dimensions
const int WIDTH = 160;  
const int HEIGHT = 128;  
#define STARTX 0
#define STARTY 0


// =============== WIFI SETTINGS ===============
struct WiFiCredential {
    const char* ssid;
    const char* password;
};
bool isConnected = false;
bool networkScanFlag = false;
int numNetworks;
unsigned long lastWifiCheck = 0;
WiFiCredential wifiList[] = {
  
}; 


// =============== CRYPTO DATA ===============
const char* coins[] = {"BTCUSDT", "ETHUSDT", "SOLUSDT", "XRPUSDT", "ADAUSDT", "DOGEUSDT", "USDCUSDT", "BUSDUSDT"};
const int numCoins = sizeof(coins) / sizeof(coins[0]);

float prices[numCoins] = {0.0};
float changePercentages[numCoins] = {0.0};

uint16_t coinColors[] = {
    ST77XX_GOLD, ST77XX_SKYBLUE, ST77XX_RED, ST77XX_GREEN, 
    ST77XX_CYAN, ST77XX_PINK, ST77XX_MAGENTA, ST77XX_GREENYELLOW
};

// =============== BUTTON SETTINGS ===============
#define BUTTON1_PIN  0
#define BUTTON2_PIN  1

EasyButton button1(BUTTON1_PIN);    
EasyButton button2(BUTTON2_PIN);    

bool cursorButtonFlag = false;

// =============== STATE VARIABLES ===============
int screenIndex = 0;  
int lastScreenIndex = 0;
int selectedCoinIndex = 0;    
int lastSelectedCoinIndex = 0;
int BWT_CyclingIndex = 0;
int InfoCyclingMillis = 0;
const unsigned long InfoCyclingInterval = 4000;



// =============== TIMING VARIABLES ===============
unsigned long previousMillis = 0;  
const unsigned long interval = 5000;

unsigned long previousDataMillis = 0;  
const unsigned long dataUpdateInterval = 9000;  

unsigned long previousCandleMillis = 0;
const unsigned long candleUpdateInterval = 10000;


// =============== TIME SETTINGS ===============
const char* ntpServer = "pool.ntp.org";   
const long utcOffsetInSeconds = 0;
WiFiUDP udp;
NTPClient timeClient(udp, ntpServer, utcOffsetInSeconds, 30000);


// =============== CANDLE CHART ===============
const unsigned long dataHistoricalUpdateInterval = 11000;
struct Candle {
    float open;
    float high;
    float low;
    float close;
    float volume;
};
Candle candles[100];  
const char* chart_interval = "1h";     
const int limit = 24;  
float hourlyPrices[24];  


// =============== BOOT VARIABLES ===============
static unsigned long lastUpdateTime = 0;
unsigned long intervalProgress = 200;  
int curr = 0;  
bool loadingFinished = false;  

//Secret
// DigitalRainAnim digitalRainAnim = DigitalRainAnim(); 


// =============== UI VARIABLES ===============
#define mainView_bannerColor ST77XX_RED
#define cursorColor 0x1E90FF   //Dodger blue
#define highlightColor ST77XX_GOLD
#define bootLogColor ST77XX_DARKGREEN

int bannerHeight;                               //MAIN VIEW
int coinsLeftPadding = 4;
int singleCrypto_Title_Height;;                      //SingleCryptoView





// ==================================================================================================
// ==================================================================================================                
// ===================SETUP FUNCTION==============================
void setup() {
  Serial.begin(115200);
  
  // TFT
  tft.initR(INITR_BLACKTAB);  
  tft.setRotation(1);
  tft.fillScreen(ST77XX_BLACK);

  // Boot()
  screenIndex = 99;
  myFSM();   // prints the boot
  

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

             
// ====================LOOP_======================================
int start=0;
void loop() {
    button1.read();
    button2.read();


    start = millis();
     updateDisplay();
    Serial.println(millis()-start);

    // if (millis() - lastWifiCheck > 1000) {   //TODO: fuse behavior into updateDisplay()
    //   Serial.println("I'm Alive !");
    //   lastWifiCheck = millis();
        
    // }


    if (!isConnected) connectToWiFi();
    // LOOPS
   
    // if(isLoadingFinished()) ;
    // else loadingBar();


    // digitalRainAnim.loop();
      
    // delay(50);


}
// ==================================================================================================
// ==================================================================================================





// ====================BUTTON FUNCTIONS=============================
void buttonsInit() {
    button1.begin();
    button2.begin();

    button1.onPressed(B1_Callback_OnPressed);
    button2.onPressed(B2_Callback_OnPressed);
    button1.onPressedFor(2000, B1_Callback_OnLongPress);

    Serial.println("Buttons initialized!");
}
void B1_Callback_OnPressed() {
    Serial.println("Button 1 pressed! Toggling screen index.");
    screenIndex = (screenIndex + 1) % 2;
}
void B1_Callback_OnLongPress() {
    Serial.println("Button1 Long Press!");
}
void B2_Callback_OnPressed() {
    Serial.print("Button 2 pressed!");
    if (screenIndex == 0) {
        Serial.println(" Cycling.");
        selectedCoinIndex = (selectedCoinIndex + 1) % numCoins;
        cursorButtonFlag = true;
    }
}




              
// =================GENERAL_FUNCTIONS================================
int textLength;
int rowNum;
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
}



//===================================_F_S_M_=================================================
void updateDisplay() {
  // ! To be non-blocking to the button reads, only execute ONE task when calling updateDisplay
  //    this ensures that between each tasks, the buttons are read.

  // BOOT
  if(!isLoadingFinished()){
    myFSM();
    return;
  } 



  int toUpdate = -1;
  unsigned long currentMillis = millis();
  
  // Assign the highest priority task first
  if (lastScreenIndex != screenIndex) toUpdate = 0;   //"ScreenToggleUpdate"
  else if (cursorButtonFlag) toUpdate = 1;           //"CursorUpdate"
  else if (currentMillis - previousMillis >= interval) toUpdate = 2; //"timeOffUpdate"
  else if (currentMillis - previousDataMillis > dataUpdateInterval && isConnected) toUpdate = 3; //"updateCoins"
  else if (currentMillis - previousCandleMillis >= candleUpdateInterval && isConnected) toUpdate = 4; //"updateCandleChart"
  else if (currentMillis - InfoCyclingMillis >= InfoCyclingInterval && screenIndex == 0) toUpdate = 5; //"screen1_InfoCycling"


  switch (toUpdate) {
  // BUTTONS UPDATE
    case 0://"ScreenToggleUpdate":  // B1 : change screens
      myFSM();
        lastScreenIndex = screenIndex;  // Update Index
        
      break;

    case 1://"CursorUpdate":  // B2 : Cursor button pressed
      myFSM();
        cursorButtonFlag = false;
        Serial.println("Cursor moved.");
      break;
  // TIME-OFF UPDATE
    case 2://"timeOffUpdate":
      myFSM();
        previousMillis = currentMillis;
        Serial.println("Screen updated via timer.");
      break;

  


  // DATA UPDATE
    case 3://"updateCoins":
      updateALLCoins();
        previousDataMillis = currentMillis;
        Serial.println("Updated all coins.");
      break;

    case 4://"updateCandleChart":
      fetchHistoricalData(selectedCoinIndex);
        previousCandleMillis = currentMillis;
        Serial.println("Fetched historical data.");
      break;

  // SCREEN INFO Cycling : Battery/Wifi/Time Cycling
    case 5://"screenInfoCycling":
      BWT_CyclingIndex = (BWT_CyclingIndex + 1) % 2;
      InfoCyclingMillis = currentMillis;
      displayTitle();
      Serial.println("Battery/WiFi/Time cycled.");
      break;


    default:
      Serial.println("PASS");
      break;
  }
}
void myFSM(){
  // Serial.print("SreenIndex-FSM: ");
  // Serial.println(screenIndex);
  switch (screenIndex) {
      case 0: 
        cryptoView();  // Coin info screen
        break;
      case 1: 
        singleCryptoView();  // Single crypto info screen
        break;


      case 97:
        // digitalRainAnim.loop();
        break;
        
      case 98:
        loadingBar();
        // Serial.println("OK2");
        if(isLoadingFinished())screenIndex = 0;
        break;
      case 99:  
        boot();
        screenIndex = 98;
        break;
      default: 
        break;
    }
}


//===================================BOOT()=================================================
void boot() {
  tft.fillScreen(ST77XX_BLACK);
    int xPos = (160 - 128) / 2;//(tft.width() - IMG_WIDTH) / 2;
    int yPos = 0;

    tft.drawRGBBitmap(xPos, yPos, bootLogo, img_width, img_height);
    loadingBar();
}


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
    int barWidth = (endX - startX) * 3 / 4;  // Width of the progress bar (75% of the rectangle width)
    
    if(curr > barNum)loadingFinished = true;
     if(isLoadingFinished()) screenIndex = 0;
    // Draw progress bar
    else {
        tft.drawRect(startX, startY, endX - startX + 1, endY - startY, ST77XX_WHITE);  // Draw outer rectangle of the bar
        if (millis() - lastUpdateTime >= intervalProgress){
            lastUpdateTime = millis();  // Update the time when the progress is updated
            int barSegmentWidth = barWidth / barNum;  // Width of each bar segment

            // Draw the next segment in the progress bar
            tft.fillRect(startX + 1 + curr * (barSegmentWidth + 1), startY + 1, barSegmentWidth, barHeight, ST77XX_GREEN);  
            Serial.print("Printing a load bar : ");
            Serial.println(curr);
            curr += 1;  // Move to the next segment
        }
    }


}  // Non-blocking loading bar function (15 segments)
bool isLoadingFinished() {
    return loadingFinished;
}



//================================CRYPTO_VIEW=========================================================
void cryptoView() {
  // updateCoinsArrays();
  tft.fillScreen(ST77XX_BLACK);  // Clear the screen
  displayTitle();
  displayCoin(prices, changePercentages); // MUST be after displayTitle...
}

// DISPLAY
void displayTitle() {
  String title = "CryptoTracker";
  bannerHeight = textHeight(title, tft)+5; // 13
  int bannerColor = mainView_bannerColor;
  int titleColor = ST77XX_WHITE;
  tft.fillRect(STARTX, STARTY, WIDTH, bannerHeight, bannerColor);

  int titleWidth = textWidth(title, tft);
  int titleX = (WIDTH - titleWidth) / 2 - 4;
  int titleY = (bannerHeight - 4) / 2 -2;
  tft.setTextColor(titleColor, bannerColor);
  tft.setCursor(titleX, titleY);
  tft.setTextSize(1);
  tft.println(title);


  // Clear the status area before cycling through the data
  // tft.fillRect(titleX + titleWidth + 1, STARTY, WIDTH, bannerHeight, bannerColor);


  displayWifiStatus(bannerHeight); // at left of banner

  // CYCLE
  switch (BWT_CyclingIndex){
      case 0:
        displayBatteryStatus(titleWidth, titleX, bannerHeight, bannerColor);
        break;
      case 1:
        displayTime(titleX, titleY, titleWidth);
        break;
    }  
}
void displayBatteryStatus(int titleWidth, int titleX, int bannerHeight, int bannerColor){
  int batteryLevel = 81;  // Simulate battery level
  int bckgdColor = ST77XX_WHITE;
  int batteryColor = (batteryLevel >= 70) ? ST77XX_GREEN : (batteryLevel >= 40) ? ST77XX_ORANGE : (batteryLevel >= 20) ? ST77XX_RED : ST77XX_MAGENTA;
  int blocksToFill = (batteryLevel >= 70) ? 3 : (batteryLevel >= 40) ? 2 : (batteryLevel >= 20) ? 1 : 0;
  // String batteryText = String(batteryLevel) + "%";
  // tft.setTextColor(ST77XX_WHITE);
  // tft.setCursor(start_X, STARTY + tft.fontHeight() + 5);
  // tft.setTextSize(1);
  // tft.println(batteryText);


  // Dimensions
  int endTitle = titleX + titleWidth;   // 91
  int available_Width = WIDTH - endTitle - 5; // 34
  int available_Height = 12;//bannerHeight;

  

  // Indicator
  // int margin = 1;
  // int batteryBlockHeight = 6; // size of one of the three blocks representing battery state.
  // int batteryBlockLength = 4;
  // int capLength = 2;
  // int capHeight = 3;

  // Battery dimensions
  int margin = 1;  // Margin between blocks and edges
  int totalLength = available_Width *3/5; // 1/5 each side for padding      (margin + batteryBlockLength)*3 + margin + capLength;
  int totalHeight = available_Height - 2*2*margin;  //margin + batteryBlockHeight + margin;


  // Dynamically scale dimensions
  int capLength = margin * 2 ;//batteryBlockLength / 2;                // Cap length
  int batteryBlockLength = round((totalLength - capLength - 4*margin)/3); //max(6, available_Width / 25); // Scale block length dynamically

  int batteryBlockHeight = totalHeight- 2*margin;    //max(4, available_Height / 5 +1);  // Scale height based on banner height
  int capHeight = batteryBlockHeight / 2 + 3;       // Cap height

  
  
  
  // Clear the area
  tft.fillRect(endTitle + 1, STARTY, available_Width, bannerHeight, bannerColor);
  

  // battery background
  int start_X = endTitle + (available_Width - totalLength)/2 + 2*margin; //   (WIDTH - endTitle)/2; //endTitle + 2* margin;  //WIDTH - availableSpace + 2*margin;
  int start_Y = STARTY + (available_Height - totalHeight) / 2; //+margin;
  int width_X = totalLength - capLength; //3*batteryBlockLength + 4*margin;
  int height_Y = totalHeight; //batteryBlockHeight +2*margin;
  // Serial.println(endTitle);         // 91
  // Serial.println(start_X);         // 99
  // Serial.println(available_Width);    // 34
  // Serial.println(available_Height);  // 11
  // Serial.println(totalLength);      // 20
  tft.fillRect(start_X, start_Y, width_X, height_Y, bckgdColor);       

  // CAP
  int capStart_X = start_X + totalLength - capLength;
  int capStart_Y = start_Y + (totalHeight - capHeight)/2; //start_Y+(batteryBlockHeight+2*margin)/2 - 1;
  tft.fillRect(capStart_X, capStart_Y, capLength, capHeight, bckgdColor); 

  // BatteryBlocks
  
  
  for (int i = 0; i < blocksToFill; i++) {
    int block_X = start_X + margin + i * (margin + batteryBlockLength);
    int block_Y = start_Y + margin;
    tft.fillRect(block_X, block_Y, batteryBlockLength, batteryBlockHeight, batteryColor);
  }
}
void displayTime(int titleX, int titleY, int titleWidth){
  int timeColor = ST77XX_WHITE;
  timeClient.update(); // Update the time from NTP server
  String timeString = String(timeClient.getHours() + 1) + ":" + String(timeClient.getMinutes());

  tft.setTextSize(1);
  int start_X = WIDTH - textWidth(timeString, tft) - 5;//(titleX + titleWidth);
  int start_Y = titleY+1;

  tft.setTextColor(timeColor);
  tft.setCursor(start_X, start_Y);
  tft.setTextSize(1);
  tft.println(timeString);
}
void displayWifiStatus(int bannerHeight) {
  int dot_size = 2;              //Diameter = 4pixels
  int wifiStatusX = STARTX + 5;
  int wifiStatusY = bannerHeight/2; //7
  if (WiFi.status() == WL_CONNECTED) {
    tft.fillCircle(wifiStatusX, wifiStatusY, dot_size, ST77XX_GREEN);
  // } else if (WiFi.status() == WL_IDLE_STATUS || WiFi.status() == WL_CONNECT_FAILED || WiFi.status() == WL_DISCONNECTED) {
  //   tft.fillCircle(wifiStatusX, wifiStatusY, dot_size, ST77XX_YELLOW);
  } else {
    tft.fillCircle(wifiStatusX, wifiStatusY, dot_size, ST77XX_YELLOW);
  }
}
void displayCursor(int selectedCoinIndex, int rowHeight, int rowMargin, int yPosition, int leftPadding) {
  
  int cursorWidth = 3;  // Cursor thickness
  // int cursorAreaHeight = yPosition + selectedCoinIndex * (rowHeight + rowMargin);
  int cursorHeight = rowHeight + 1;  //
  int cursorX = STARTX + coinsLeftPadding;
  int cursorY = yPosition - 1;
  
  // CLEAR
  tft.fillRect(cursorX, cursorY, cursorWidth, HEIGHT, ST77XX_BLACK);
  // CURSOR
  tft.fillRect(cursorX, cursorY, cursorWidth, cursorHeight, cursorColor);
}
void displayCoin(float prices[], float changePercentages[]) {
  int selectedColor = highlightColor;
  int leftPadding = STARTX + coinsLeftPadding + 6; //displayCursor:6=
  int rightMargin = 2;
  int rowHeight = textHeight("A", tft);      // Height of the text in a single row : 5
  int rowMargin = rowHeight - 1;             // Margin between rows
  int yPosition = STARTY + bannerHeight + 3;

  for (int i = 0; i < numCoins; i++) {
    float price = prices[i];
    String coinName = getFormattedCoinName(coins[i]);
    String priceText = "Error.";

    if (isnan(price) || price <= 0) {
      price = 0.0;
    }
    // Format price to fit into the available width
    if (price > 1000 || textWidth(String(price, 2), tft) > WIDTH / 2) {
      // Serial.println(coinName + " 's decimals have been formatted (>$1000).");
      priceText = String((int)price); // remove decimals if price is too large
      priceText = "$" + priceText; // Add the dollar sign
    } else {
      priceText = "$" + String(price, 2); // Format price with two decimals for smaller values
    }

    // Variables
    float change = changePercentages[i];
    String changeText = (change >= 0 ? "+" : "") + String(change, 2) + "%";
    int coinNameWidth = textWidth("DOGE", tft);
    int priceTextWidth = textWidth(priceText, tft);
    int changeTextWidth = textWidth(changeText, tft);
    int coinNameX = leftPadding;
    int changeX = WIDTH - changeTextWidth - rightMargin;
    int priceX = coinNameX + coinNameWidth + 25;//changeX - priceTextWidth - 25;
    

    // Set text color 
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);  // Default text color

      // Set text color based on selection
    if (i == selectedCoinIndex) {
      // Call the cursor display function
      displayCursor(selectedCoinIndex, rowHeight, rowMargin, yPosition ,leftPadding);
      tft.setTextColor(selectedColor);  // Highlighted coin text color ST77XX_GREENYELLOW ST77XX_BLUE
    } else {
      tft.setTextColor(ST77XX_WHITE);  // Default text color
    }


    // Print coin details
    tft.setCursor(coinNameX, yPosition);
    tft.print(coinName);
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    tft.setCursor(priceX, yPosition);
    tft.print(priceText);
    tft.setCursor(changeX, yPosition);
    tft.setTextColor(change >= 0 ? ST77XX_GREEN : ST77XX_RED, ST77XX_BLACK);
    tft.print(changeText);

    // Move to the next row
    yPosition += rowHeight + rowMargin;
  }
}


// HTTP's
void updateCoinArray(int coinIndex) {
  HTTPClient http;
  String url = "https://api.binance.com/api/v3/ticker/24hr?symbol=" + String(coins[coinIndex]);
  http.begin(url);
  int httpResponseCode = http.GET();
  if (httpResponseCode == HTTP_CODE_OK) {
    DynamicJsonDocument jsonBuffer(2048);
    DeserializationError error = deserializeJson(jsonBuffer, http.getString());
    if (error) {
      Serial.print("Failed to parse JSON: ");
      Serial.println(error.c_str());
    } else {
      prices[coinIndex] = jsonBuffer["lastPrice"].as<float>();
      changePercentages[coinIndex] = jsonBuffer["priceChangePercent"].as<float>();
    }
  } else {
    Serial.print("HTTP request failed with error code: ");
    Serial.println(httpResponseCode);
  }
  http.end();  // End the HTTP request
} // single coin http update
void updateALLCoins(){
  for (int i = 0; i < numCoins; i++) {
      updateCoinArray(i);
    }
}
String getFormattedCoinName(const char* coinSymbol) {
  String coinName = String(coinSymbol);
  if (coinName.endsWith("USDT")) {
    coinName = coinName.substring(0, coinName.length() - 4);
  }
  return coinName;
}



//==================================Single_Crypto_View=========================================================
void singleCryptoView() {
  // AREA :
  //      - displaySingleCryptoInfo  :  (0,0) to (xmax, 20)
  //      - displayCryptoChart  :  (0,20) to (xmax, 100)
  //
  tft.fillScreen(ST77XX_BLACK);  // Clear
  displaySingleCryptoInfo(selectedCoinIndex, prices, changePercentages);

  // UPDATE crypto candles array
  if(selectedCoinIndex != lastSelectedCoinIndex) {
      fetchHistoricalData(selectedCoinIndex);
      lastSelectedCoinIndex = selectedCoinIndex;
    }
  //chart
  displayCryptoChart();
  // debugPrintColorfulPixels();
}

// DISPLAY
void displaySingleCryptoInfo(int coinIndex, float prices[], float changePercentages[]) {
  int sizeTwoTextHeight = 12;
  // Define drawing area
  int leftPadding = 4;
  int rightMargin = 1;
  int yPosition = STARTY + 2; // Start drawing within the area
  int verticalMargin = 5;  // Between name and price.
  singleCrypto_Title_Height = yPosition + 2*sizeTwoTextHeight + verticalMargin + 4;

  // GET COIN INFO
  String coinName = getFormattedCoinName(coins[coinIndex]);
  float change = changePercentages[coinIndex];
  String changeText = (change >= 0 ? "+" : "") + String(change, 2) + "%";
  float price = prices[coinIndex];
  String priceText = "$" + String(price, 2);

  // Calculate positions for coin name, change percentage, and price
  int coinNameWidth = textWidth(coinName, tft);
  int changeTextWidth = textWidth(changeText, tft);
  int priceTextWidth = textWidth(priceText, tft);
  int coinNameX = STARTX + leftPadding;
  int changeX = WIDTH - changeTextWidth;
  int priceX = STARTX + leftPadding; // ENDX - priceTextWidth - rightMargin;

  
  // CoinName and percentage change (size 2)
  tft.setTextSize(2);
  tft.setCursor(coinNameX, yPosition);
  tft.setTextColor(ST77XX_WHITE);
  tft.print(coinName);

  // % % %
  tft.setCursor(changeX, yPosition);
  tft.setTextColor(change >= 0 ? ST77XX_GREEN : ST77XX_RED);
  tft.print(changeText);

  // PRICE
  tft.setCursor(priceX, yPosition + sizeTwoTextHeight + verticalMargin);
  tft.setTextColor(ST77XX_WHITE);
  tft.print(priceText);
}
void drawCandle(int x, const Candle &candle, int yStart, int yEnd, float minPrice, float maxPrice) {
  int availableHeight = yEnd - yStart; // Total height for the chart
  int candleWidth = 6;  // Width of the candlestick body
  int wickWidth = 2;    // Width of the wick
  
  // Scale prices to pixel positions
  int yHigh = yStart + (maxPrice - candle.high) * availableHeight / (maxPrice - minPrice);
  int yLow = yStart + (maxPrice - candle.low) * availableHeight / (maxPrice - minPrice);
  int yOpen = yStart + (maxPrice - candle.open) * availableHeight / (maxPrice - minPrice);
  int yClose = yStart + (maxPrice - candle.close) * availableHeight / (maxPrice - minPrice);
  // Ensure the body is always drawn top to bottom
  int yBodyTop = min(yOpen, yClose);
  int yBodyBottom = max(yOpen, yClose);
  // Color for up/down candles
  uint32_t bodyColor = (candle.close >= candle.open) ? ST77XX_GREEN : ST77XX_RED;
  // Draw the wick
  tft.fillRect(x + (candleWidth - wickWidth) / 2, yHigh, wickWidth, yLow - yHigh, bodyColor);
  // Draw the candle body
  tft.fillRect(x, yBodyTop, candleWidth, yBodyBottom - yBodyTop, bodyColor);
}
void displayCryptoChart() {
  int x = STARTX;
  int yStart = singleCrypto_Title_Height; //STARTY + verticalMargin + 2*tft.fontHeight() + 1;   //see displaySingleCryptoInfo()
  int yEnd = HEIGHT -2;  // padding
  
  // Find the min and max prices from the candles
  float minPrice = candles[0].low;
  float maxPrice = candles[0].high;
  for (int i = 1; i < limit; i++) {
    if (candles[i].low < minPrice) minPrice = candles[i].low;
    if (candles[i].high > maxPrice) maxPrice = candles[i].high;
  }

  //Scale - work in progress...
  // displayCryptoScale(minPrice, maxPrice, yStart, yEnd);
  
  // Draw each candle
  for (int i = 0; i < limit; i++) {
    drawCandle(x, candles[i], yStart, yEnd, minPrice, maxPrice);
    x += 8; // Increment x position for the next candle
  }
}
void displayCryptoScale(float minPrice, float maxPrice, int yStart, int yEnd) {
  int scaleSteps = 5; // Number of labels to display on the scale
  float stepValue = (maxPrice - minPrice) / (scaleSteps - 1);
  int stepHeight = (yEnd - yStart) / (scaleSteps - 1);
  // Draw the scale
  for (int i = 0; i < scaleSteps; i++) {
    float price = minPrice + stepValue * i;
    int yPosition = yEnd - i * stepHeight; // Invert Y since the screen's origin is at the top-left
    // Print price label
    tft.setCursor(0, yPosition - 8); // Adjust y-position to center the text
    tft.setTextSize(1);
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    tft.print(String(price, 2)); // Print the price with 2 decimal points
    // Optional: Draw horizontal grid line
    tft.drawLine(30, yPosition, tft.width(), yPosition, ST77XX_GREY);
  }
}


// HTTP
void fetchHistoricalData(int coinIndex) {
  const char* binanceEndpoint = "https://api.binance.com/api/v3/klines";
  // Build the URL for Binance API request
  String url = String(binanceEndpoint) + "?symbol=" + coins[coinIndex] + "&interval=" + chart_interval + "&limit=" + String(limit);
  HTTPClient http;
  http.begin(url);
  int httpResponseCode = http.GET();
  if (httpResponseCode == HTTP_CODE_OK) {
    String payload = http.getString();
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, payload);
    if (!error) {
      // Extract kline data and store in candles array
      Serial.println("Fetched Crypto data from Binance.");
      for (int i = 0; i < limit; i++) {
        candles[i].open = doc[i][1].as<float>();  // Open price
        candles[i].high = doc[i][2].as<float>();  // High price
        candles[i].low = doc[i][3].as<float>();  // Low price
        candles[i].close = doc[i][4].as<float>();  // Close price
        candles[i].volume = doc[i][5].as<float>();  // Volume
      }
    } else {
      Serial.print("Failed to parse JSON: ");
      Serial.println(error.c_str());
    }
  } else {
    Serial.print("HTTP GET request failed, error code: ");
    Serial.println(httpResponseCode);
  }
  http.end();
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

  // CLEAR
  tft.fillRect(0, 0, WIDTH, 20, ST77XX_BLACK);

  // WRITE
  drawText(0, Row*textHeight("A", tft), textColor, text);

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




