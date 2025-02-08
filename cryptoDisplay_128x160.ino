//V.2.5.2-128x160


//  TODO :
//        - Fix lagging after button press
//        - Fix battery display
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
//        - Add Loading screen
//        - WIFI connect only to detected wifis


#include <Arduino.h>
#include <EasyButton.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <esp_http_client.h>
// #include "cryptoBoot.h"
#include "img/bootLogo.h"  // Include the bootLogo image
#include "DigitalRainAnim.h"


TFT_eSPI tft = TFT_eSPI();  // Initialize TFT screen
#define TFT_GREY 0x7BEF

// LCD Dimensions : 128x128
#define STARTX 0
#define STARTY 0
#define ENDX 160
#define ENDY 128
int WIDTH = ENDX - STARTX;    //160
int HEIGHT = ENDY - STARTY;  //`128

// WiFi Credentials
struct WiFiCredential {
  const char* ssid;
  const char* pswd;
};
bool isConnected = false;
bool networkScanFlag = false;
int numNetworks;
unsigned long lastWifiCheck = 0;
WiFiCredential wifiList[] = {
  {"", ""}
};

// Coins to display (MAX. 7)
const char* coins[] = {"BTCUSDT", "ETHUSDT", "SOLUSDT", "XRPUSDT", "ADAUSDT", "DOGEUSDT", "USDCUSDT", "BUSDUSDT"};
const int numCoins = sizeof(coins) / sizeof(coins[0]);
float prices[numCoins] = {0.0};
float changePercentages[numCoins] = {0.0};
uint16_t coinColors[] = {TFT_GOLD, TFT_SKYBLUE, TFT_RED, TFT_GREEN, TFT_CYAN, TFT_PINK, TFT_MAGENTA, TFT_GREENYELLOW, TFT_VIOLET};


// Button pin setup
// Button button1(19);   //TOGGLE between screens
// Button button2(21);   //CYCLE cursor downward
// Button button3(22);
EasyButton button1(19);    //TOGGLE between screens
EasyButton button2(21);   //CYCLE cursor downward
bool cursorButtonFlag = false; //for cursor button.

// Screen cycle state
int screenIndex = 0;  // 0: Coin Info Screen, 1: Wi-Fi Info Screen, 2: Title Screen, 99: boot
int lastScreenIndex = 0;
int selectedCoinIndex = 0;    // Cursor
int lastSelectedCoinIndex = 0;
int BWT_CyclingIndex = 0;       // Index to cycle between showing the battery or time
int InfoCyclingMillis = 0; // 0 = Wi-Fi, 1 = Battery, 2 = Time

// Millis Screen Update
unsigned long previousMillis = 0;  // Global timer to track screen update intervals
const unsigned long interval = 5000;  // .. 

// Millis Data Update
unsigned long previousDataMillis = 0;  // Timer for data update
const unsigned long dataUpdateInterval = 9000;  // Update coin data every 30 seconds

// TIME
const char* ntpServer = "pool.ntp.org";   // NTP configuration
const long utcOffsetInSeconds = 0;     // Adjust for your time zone, UTC = 0 for GMT
WiFiUDP udp;
NTPClient timeClient(udp, ntpServer, utcOffsetInSeconds, 30000);  // Fetch time every 30 seconds


// CANDLE CHART
const unsigned long dataHistoricalUpdateInterval = 11000;
struct Candle {
  float o;  // Open price
  float h;  // High price
  float l;  // Low price
  float c;  // Close price
  float v;  // Volume
};
Candle candles[100];  // Array to store candle data (modify size as needed)
const char* chart_interval = "1h";     // 1-hour interval
const int limit = 24;                 // Number of data points (last 24 hours)
float hourlyPrices[24];             // Array to store hourly closing prices



// BOOT
static unsigned long lastUpdateTime = 0;
unsigned long intervalProgress = 200;  // Speed of loading bar progression
int curr = 0;  // Tracks progress of loading bar
bool loadingFinished = false; // Flag to check if loading is finished

//Secret
DigitalRainAnim digitalRainAnim = DigitalRainAnim(); 




//MAIN VIEW
int bannerHeight;
int coinsLeftPadding=4;

//SingleCryptoView
int singleCrypto_Title_Height;




//-----BUTTONS_INIT--------
void buttonsInit(){
  button1.begin();
  button2.begin();

  // Serial.println("Initializing buttons...");

  button1.onPressed(onButton1Pressed);
  button2.onPressed(onButton2Pressed);
  button1.onPressedFor(2000, onButton1LongPress);

  Serial.println("Buttons initialized!");
}

void onButton1Pressed(){
  Serial.println("Button 1 pressed! Toggling screen index.");
    screenIndex = (screenIndex + 1) % 2;  // Toggle between 0 and 1
}
void onButton2Pressed(){
  if(screenIndex==0) {
    Serial.println("Button 2 pressed! Cycling.");
    selectedCoinIndex = (selectedCoinIndex + 1) % numCoins;  // Cycle forward
    cursorButtonFlag = true; // for cursor
  }
}
void onButton1LongPress(){
  Serial.print("Button1 Long Press !");
}
//=======================================================================================



unsigned long start;


void setup() {
  Serial.begin(115200);

  // TFT
  tft.init();
  tft.setRotation(1);  // horizontal, pins to the right
  

  start = millis();
  connectToWiFi();
  Serial.print(millis() - start);
  // measureExecutionTime(connectToWiFi, "connectToWiFi");

  // BUTTONS
  buttonsInit();
  


  // digitalRainAnim.init(&tft);
  //   digitalRainAnim.pause();
  //   digitalRainAnim.stop();
  tft.fillScreen(TFT_BLACK);

  screenIndex = 99;
  myFSM();
  
  
  
  // Time
  timeClient.begin();    // Start NTPClient to get the time
  timeClient.update();  // Set initial time (Optional, but ensures time starts from a known point)

}


void loop() {
    
  button1.read();
  button2.read();
  
  if (millis()-lastWifiCheck >2000) connectToWiFi();


    
  
  // LOOPS
  if(isLoadingFinished()) screenIndex = 0;
  // digitalRainAnim.loop();
    
  updateDisplay();
    
  
  // delay(100);
}


//=============================GENERAL==========================================================



int textLength;
int rowNum;
void connectToWiFi() {
  lastWifiCheck = 0;
  if (isConnected) return;  // Exit if already connected
    static int attempts = 0;  // Track connection attempts
    static bool connecting = false;
    static unsigned long lastAttemptTime = 0;

    

    // Scan for Networks
    if (!networkScanFlag) {  
        Serial.println("Scanning for networks...");
        numNetworks = WiFi.scanNetworks();
        if (numNetworks == 0) {
            Serial.println("No networks found.");
            return;
        } else {
            Serial.printf("Found %d networks.\n", numNetworks);
            drawText(0,0,TFT_GREEN,String("Found " + String(numNetworks) + " networks."));
        }
        networkScanFlag = true; 
    }

    // Find and connect to wifi from given list
    if (!connecting) {  
        for (int i = 0; i < sizeof(wifiList) / sizeof(wifiList[0]); i++) {
            for (int j = 0; j < numNetworks; j++) {
                if (WiFi.SSID(j) == wifiList[i].ssid) {
                    String message = String("Connecting to: " + String(wifiList[i].ssid) + ".");
                    textLength = tft.textWidth(message);
                    rowNum = (j+1);
                    Serial.printf("Connecting to SSID: %s\n", wifiList[i].ssid);
                    drawText(0,rowNum*6,TFT_GREEN ,message);
                    WiFi.begin(wifiList[i].ssid, wifiList[i].pswd);
                    connecting = true;
                    attempts = 0;
                    lastAttemptTime = millis();  // Start connection timer
                    return;
                }
            }
        }
    }

    // Attempt
    if (connecting) {
        // Check connection every 500ms without blocking
        if (millis() - lastAttemptTime >= 2000) {
            lastAttemptTime = millis(); 
            if (WiFi.status() == WL_CONNECTED) {
                Serial.println("\nConnected!");
                drawText(textLength ,rowNum*6,TFT_GREEN,"  Connected!");
                Serial.print("IP Address: ");
                Serial.println(WiFi.localIP());
                isConnected = true;
                connecting = false;
                
            } else {
                Serial.print(".");
                attempts++;
                if (attempts >= 10) {  // Try for 10 seconds max (10 * 1000ms)
                    Serial.println("\nFailed to connect.");
                    drawText(textLength ,rowNum*6,TFT_RED,"  Failed to connect.");
                    connecting = false;
                }
            }
        }
    }
}

void drawText(int x, int y, int textColor, String text){
  tft.setCursor(x,y);
  tft.setTextColor(textColor);
  tft.println(text);

}


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




//===================================Updates_MILLIS()=================================================



void updateDisplay() {
  // ! To be non-blocking to the button reads, only execute ONE task when calling updateDisplay
  //    this ensures that between each tasks, the buttons are read.
  //
  unsigned long currentMillis = millis();
  int toUpdate;

  

  
  // Place in INVERSE order of priority
  if(lastScreenIndex != screenIndex) toUpdate = 5;//"ScreenToggleUpdate";
  if(cursorButtonFlag) toUpdate = 4;//"CursorUpdate";
  if(currentMillis - previousMillis >= interval) toUpdate = 3;//"timeOffUpdate";
  if(currentMillis - previousDataMillis > dataUpdateInterval && isConnected) toUpdate = 2;//"updateCoins";
  if(currentMillis - previousDataMillis >= dataHistoricalUpdateInterval && isConnected) toUpdate = 1;//"updateCandleChart";
  if(currentMillis - InfoCyclingMillis >= interval && screenIndex == 0) toUpdate = 0;//"screenInfoCycling";



  switch (toUpdate) {
  // BUTTONS UPDATE
    case 0://"ScreenToggleUpdate":  // B1 : change screens
      myFSM();
        lastScreenIndex = screenIndex;  // Update Index
        Serial.println("Screen changed by button.");
      break;

    case 1;//"CursorUpdate":  // B2 : Cursor button pressed
      myFSM();
        cursorButtonFlag = false;
        Serial.println("Cursor moved.");
      break;
  // TIME-OFF UPDATE
    case 2;//"timeOffUpdate":
      myFSM();
        previousMillis = currentMillis;
        Serial.println("Screen updated via timer.");
      break;

  


  // DATA UPDATE
    case 3;//"updateCoins":
      updateALLCoins();
        previousDataMillis = currentMillis;
        Serial.println("Updated all coins.");
      break;

    case 4;//"updateCandleChart":
      fetchHistoricalData(selectedCoinIndex);
        dataHistoricalUpdateInterval = currentMillis;
        Serial.println("Fetched historical data.");
      break;

  // SCREEN INFO Cycling : Battery/Wifi/Time Cycling
    case 5;//"screenInfoCycling":
      BWT_CyclingIndex = (BWT_CyclingIndex + 1) % 2;
      InfoCyclingMillis = currentMillis;
      displayTitle();
      Serial.println("Battery/WiFi/Time cycled.");
      break;


    default:
      break;
  }
}
void myFSM(){
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
// Boot function that handles images with glitches (non-blocking)
void boot() {
  tft.fillScreen(TFT_BLACK);
  tft.setSwapBytes(true);
  int xPos = (160 - 128) / 2;

  tft.pushImage(xPos, 0, img_width, img_height, bootLogo);  // Display boot logo
  curr = 0;  // Reset the progress bar
    loadingBar();
}

// Non-blocking loading bar function (15 segments)
void loadingBar() {
    int startY = 105;
    int startX = 40;
    int endY = 115;
    int endX = 120;

    int barNum = 15;  // Total number of bars (segments)
    int barHeight = (endY - startY) - 2;  // Height of the progress bar
    int barWidth = (endX - startX) * 3 / 4;  // Width of the progress bar (75% of the rectangle width)

    if(curr > barNum)loadingFinished = true;
    
    // Draw progress bar
    else {
        tft.drawRect(startX, startY, endX - startX + 1, endY - startY, TFT_WHITE);  // Draw outer rectangle of the bar
        if (millis() - lastUpdateTime >= intervalProgress){
            lastUpdateTime = millis();  // Update the time when the progress is updated
            int barSegmentWidth = barWidth / barNum;  // Width of each bar segment

            // Draw the next segment in the progress bar
            tft.fillRect(startX + 1 + curr * (barSegmentWidth + 1), startY + 1, barSegmentWidth, barHeight, TFT_GREEN);  
            curr += 1;  // Move to the next segment
        }
    }


}


bool isLoadingFinished() {
    return loadingFinished;
}








//================================CRYPTO_VIEW=========================================================
void cryptoView() {
  // updateCoinsArrays();
  tft.fillScreen(TFT_BLACK);  // Clear the screen
  displayTitle();
  displayCoin(prices, changePercentages); // MUST be after displayTitle...
}
// Display Title and Wi-Fi status
void displayTitle() {
  String title = "CryptoTracker";
  bannerHeight = tft.fontHeight()+5; // 13
  int bannerColor = TFT_RED;
  int titleColor = TFT_WHITE;
  tft.fillRect(STARTX, STARTY, WIDTH, bannerHeight, bannerColor);

  int titleWidth = tft.textWidth(title);
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
  int bckgdColor = TFT_WHITE;
  int batteryColor = (batteryLevel >= 70) ? TFT_GREEN : (batteryLevel >= 40) ? TFT_ORANGE : (batteryLevel >= 20) ? TFT_RED : TFT_MAGENTA;
  int blocksToFill = (batteryLevel >= 70) ? 3 : (batteryLevel >= 40) ? 2 : (batteryLevel >= 20) ? 1 : 0;
  // String batteryText = String(batteryLevel) + "%";
  // tft.setTextColor(TFT_WHITE);
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
  int timeColor = TFT_WHITE;
  timeClient.update(); // Update the time from NTP server
  String timeString = String(timeClient.getHours() + 1) + ":" + String(timeClient.getMinutes());

  tft.setTextSize(1);
  int start_X = WIDTH - tft.textWidth(timeString) - 5;//(titleX + titleWidth);
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
    tft.fillCircle(wifiStatusX, wifiStatusY, dot_size, TFT_GREEN);
  // } else if (WiFi.status() == WL_IDLE_STATUS || WiFi.status() == WL_CONNECT_FAILED || WiFi.status() == WL_DISCONNECTED) {
  //   tft.fillCircle(wifiStatusX, wifiStatusY, dot_size, TFT_YELLOW);
  } else {
    tft.fillCircle(wifiStatusX, wifiStatusY, dot_size, TFT_YELLOW);
  }
}




void displayCursor(int selectedCoinIndex, int rowHeight, int rowMargin, int yPosition, int leftPadding) {
  int cursorColor = TFT_MAGENTA;
  int cursorWidth = 3;  // Cursor thickness
  // int cursorAreaHeight = yPosition + selectedCoinIndex * (rowHeight + rowMargin);
  int cursorHeight = rowHeight + 1;  //
  int cursorX = STARTX + coinsLeftPadding;
  int cursorY = yPosition - 1;
  
  // CLEAR
  tft.fillRect(cursorX, cursorY, cursorWidth, HEIGHT, TFT_BLACK);
  // CURSOR
  tft.fillRect(cursorX, cursorY, cursorWidth, cursorHeight, cursorColor);
}
// Main function to display coin information
void displayCoin(float prices[], float changePercentages[]) {
  int selectedColor = TFT_CYAN;
  int leftPadding = STARTX + coinsLeftPadding + 6; //displayCursor:6=
  int rightMargin = 2;
  int rowHeight = tft.fontHeight();      // Height of the text in a single row : 5
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
    if (price > 1000 || tft.textWidth(String(price, 2)) > WIDTH / 2) {
      Serial.println(coinName + " 's decimals have been formatted (>$1000).");
      priceText = String((int)price); // remove decimals if price is too large
      priceText = "$" + priceText; // Add the dollar sign
    } else {
      priceText = "$" + String(price, 2); // Format price with two decimals for smaller values
    }

    // Variables
    float change = changePercentages[i];
    String changeText = (change >= 0 ? "+" : "") + String(change, 2) + "%";
    int coinNameWidth = tft.textWidth("DOGE");
    int priceTextWidth = tft.textWidth(priceText);
    int changeTextWidth = tft.textWidth(changeText);
    int coinNameX = leftPadding;
    int changeX = WIDTH - changeTextWidth - rightMargin;
    int priceX = coinNameX + coinNameWidth + 25;//changeX - priceTextWidth - 25;
    

    // Set text color 
    tft.setTextColor(TFT_WHITE, TFT_BLACK);  // Default text color

      // Set text color based on selection
    if (i == selectedCoinIndex) {
      // Call the cursor display function
      displayCursor(selectedCoinIndex, rowHeight, rowMargin, yPosition ,leftPadding);
      tft.setTextColor(selectedColor);  // Highlighted coin text color TFT_GREENYELLOW TFT_BLUE
    } else {
      tft.setTextColor(TFT_WHITE);  // Default text color
    }


    // Print coin details
    tft.setCursor(coinNameX, yPosition);
    tft.print(coinName);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(priceX, yPosition);
    tft.print(priceText);
    tft.setCursor(changeX, yPosition);
    tft.setTextColor(change >= 0 ? TFT_GREEN : TFT_RED, TFT_BLACK);
    tft.print(changeText);

    // Move to the next row
    yPosition += rowHeight + rowMargin;
  }
}




//==================================Single_Crypto_View=========================================================
void singleCryptoView() {
  // AREA :
  //      - displaySingleCryptoInfo  :  (0,0) to (xmax, 20)
  //      - displayCryptoChart  :  (0,20) to (xmax, 100)
  //
  tft.fillScreen(TFT_BLACK);  // Clear
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
  int coinNameWidth = tft.textWidth(coinName);
  int changeTextWidth = tft.textWidth(changeText);
  int priceTextWidth = tft.textWidth(priceText);
  int coinNameX = STARTX + leftPadding;
  int changeX = WIDTH - changeTextWidth;
  int priceX = STARTX + leftPadding; // ENDX - priceTextWidth - rightMargin;

  
  // CoinName and percentage change (size 2)
  tft.setTextSize(2);
  tft.setCursor(coinNameX, yPosition);
  tft.setTextColor(TFT_WHITE);
  tft.print(coinName);

  // % % %
  tft.setCursor(changeX, yPosition);
  tft.setTextColor(change >= 0 ? TFT_GREEN : TFT_RED);
  tft.print(changeText);

  // PRICE
  tft.setCursor(priceX, yPosition + sizeTwoTextHeight + verticalMargin);
  tft.setTextColor(TFT_WHITE);
  tft.print(priceText);
}
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
        candles[i].o = doc[i][1].as<float>();  // Open price
        candles[i].h = doc[i][2].as<float>();  // High price
        candles[i].l = doc[i][3].as<float>();  // Low price
        candles[i].c = doc[i][4].as<float>();  // Close price
        candles[i].v = doc[i][5].as<float>();  // Volume
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
void drawCandle(int x, const Candle &candle, int yStart, int yEnd, float minPrice, float maxPrice) {
  int availableHeight = yEnd - yStart; // Total height for the chart
  int candleWidth = 6;  // Width of the candlestick body
  int wickWidth = 2;    // Width of the wick
  
  // Scale prices to pixel positions
  int yHigh = yStart + (maxPrice - candle.h) * availableHeight / (maxPrice - minPrice);
  int yLow = yStart + (maxPrice - candle.l) * availableHeight / (maxPrice - minPrice);
  int yOpen = yStart + (maxPrice - candle.o) * availableHeight / (maxPrice - minPrice);
  int yClose = yStart + (maxPrice - candle.c) * availableHeight / (maxPrice - minPrice);
  // Ensure the body is always drawn top to bottom
  int yBodyTop = min(yOpen, yClose);
  int yBodyBottom = max(yOpen, yClose);
  // Color for up/down candles
  uint32_t bodyColor = (candle.c >= candle.o) ? TFT_GREEN : TFT_RED;
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
  float minPrice = candles[0].l;
  float maxPrice = candles[0].h;
  for (int i = 1; i < limit; i++) {
    if (candles[i].l < minPrice) minPrice = candles[i].l;
    if (candles[i].h > maxPrice) maxPrice = candles[i].h;
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
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.print(String(price, 2)); // Print the price with 2 decimal points
    // Optional: Draw horizontal grid line
    tft.drawLine(30, yPosition, tft.width(), yPosition, TFT_DARKGREY);
  }
}




//====================================DEBUG============================================================
void debugPrintColorfulPixels() {
  for (int y = 0; y < tft.height(); y++) {
    // Choose a color gradient based on the y position
    Serial.print("screenHeight:");
    Serial.println(tft.height());
    Serial.print("screenW:");
    Serial.println(tft.width());
    uint16_t color = tft.color565(y % 32 * 8, y % 64 * 4, y % 128 * 2); // RGB gradient
    for (int x = 0; x < tft.width(); x++) {
      tft.drawPixel(x, y, color);
    }
  }
}






















//--------END---------
