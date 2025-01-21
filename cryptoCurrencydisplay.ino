//V.2
#include <Arduino.h>
#include <Button.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <esp_http_client.h>

TFT_eSPI screen = TFT_eSPI();  // Initialize TFT screen

// LCD Dimensions : 128x128
#define STARTX 2
#define STARTY 2
#define ENDX 127
#define ENDY 129
int WIDTH = ENDX - STARTX;    //125
int HEIGHT = ENDY - STARTY;  //`127

// WiFi Credentials
const char* ssid = "REDACTED";
const char* pswd = "REDACTED";

// Coins to display (MAX. 7)
const char* coins[] = {"BTCUSDT", "ETHUSDT", "SOLUSDT", "XRPUSDT", "ADAUSDT", "DOGEUSDT", "USDCUSDT", "BUSDUSDT"};
const int numCoins = sizeof(coins) / sizeof(coins[0]);
float prices[numCoins] = {0.0};
float changePercentages[numCoins] = {0.0};
uint16_t coinColors[] = {TFT_GOLD, TFT_SKYBLUE, TFT_RED, TFT_GREEN, TFT_CYAN, TFT_PINK, TFT_MAGENTA, TFT_GREENYELLOW, TFT_VIOLET};
int selectedCoinIndex = 0; // Cursor with button2, change which coin to display in single view
int lastSelectedCoinIndex = 0;

// Button pin setup
Button button1(19);
Button button2(21);
Button button3(22);
bool buttonFlag = false; //for cursor button.

// Screen cycle state
int screenIndex = 0;  // 0: Coin Info Screen, 1: Wi-Fi Info Screen, 2: Title Screen
int lastScreenIndex = 0;

// Global Millis Variables for Screen Update
unsigned long previousMillis = 0;  // Global timer to track screen update intervals
const unsigned long interval = 5000;  // .. 

// Global Millis Variables for Data Update
unsigned long previousDataMillis = 0;  // Timer for data update
const unsigned long dataUpdateInterval = 9000;  // Update coin data every 30 seconds

// TIME
const char* ntpServer = "pool.ntp.org";   // NTP configuration
const long utcOffsetInSeconds = 0;     // Adjust for your time zone, UTC = 0 for GMT
WiFiUDP udp;
NTPClient timeClient(udp, ntpServer, utcOffsetInSeconds, 30000);  // Fetch time every 30 seconds
int BWT_CyclingIndex = 0;
int InfoCyclingMillis = 0; // 0 = Wi-Fi, 1 = Battery, 2 = Time

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

//=======================================================================================
void setup() {
  Serial.begin(115200);
  connectToWiFi();
  button1.begin();
  button2.begin();
  button3.begin();

  // TFT
  screen.init();
  screen.setRotation(4);
  // Data
  fetchHistoricalData(selectedCoinIndex);
  updateALLCoins();
  // Time
  timeClient.begin();    // Start NTPClient to get the time
  timeClient.update();  // Set initial time (Optional, but ensures time starts from a known point)
}
void loop() {
  if (button1.pressed()){
    Serial.println("Button pressed! Toggling screen index.");
    screenIndex = (screenIndex + 1) % 2;  // Toggle between 0 and 1
    
  }
  // CURSOR
  if (button2.pressed() && screenIndex==0) {
    selectedCoinIndex = (selectedCoinIndex + 1) % numCoins;  // Cycle forward
    buttonFlag = true; // for cursor
  }
  updateDisplay();
}






//=============================GENERAL==========================================================
void connectToWiFi() {
  Serial.print("Connecting to Wi-Fi...");
  WiFi.begin(ssid, pswd);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected to Wi-Fi");
  Serial.println(WiFi.localIP());
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
  unsigned long currentMillis = millis();
  // UPDATE via Timer
  if (currentMillis - previousMillis >= interval) {
    // screen.fillScreen(TFT_NAVY);
    myFSM();
    previousMillis = currentMillis;  // Update the timer
  }
  // BUTTON 1 : CHANGE SCREEN via button
  if (lastScreenIndex != screenIndex) {
    // screen.fillScreen(TFT_GOLD);
    myFSM();
    lastScreenIndex = screenIndex;  // Update Index
  }
  // BUTTON 2 : Cursor
  if (buttonFlag){
    myFSM();
    buttonFlag = false;
  }
  // DATA
  if (currentMillis - previousDataMillis >= dataHistoricalUpdateInterval) {
    // fetchHistoricalData(selectedCoinIndex);
  }
  // Battery/Wifi/Time Cycling
  if (currentMillis - InfoCyclingMillis >= interval && screenIndex == 0){  //5000ms
    displayTitle();
    BWT_CyclingIndex = (BWT_CyclingIndex + 1) % 3;
    InfoCyclingMillis = currentMillis;
    Serial.println(BWT_CyclingIndex);
  }

  // // DEBUG :
  // Serial.print("Millis diff: ");
  // Serial.println(currentMillis-previousMillis);
  // Serial.print("Current screenIndex: ");
  // Serial.println(screenIndex);
}
void myFSM(){
  switch (screenIndex) {
      case 0: 
        cryptoView();  // Coin info screen
        break;
      case 1: 
        singleCryptoView();  // Single crypto info screen
        break;
      default: 
        break;
    }
}




//================================CRYPTO_VIEW=========================================================
void cryptoView() {
  // updateCoinsArrays();
  screen.fillScreen(TFT_BLACK);  // Clear the screen
  displayTitle();
  displayCoin(prices, changePercentages);
}
// Display Title and Wi-Fi status
void displayTitle() {
  String title = "CryptoTrack";
  int bannerHeight = screen.fontHeight()+3; // 13
  int bannerColor = TFT_RED;
  int titleColor = TFT_WHITE;
  screen.fillRect(STARTX, STARTY, WIDTH + 2, bannerHeight, bannerColor);
  int titleWidth = screen.textWidth(title);
  int titleX = (WIDTH - titleWidth) / 2;
  int titleY = (bannerHeight - 4) / 2;
  screen.setTextColor(titleColor, bannerColor);
  screen.setCursor(titleX, titleY);
  screen.setTextSize(1);
  screen.println(title);

  // CYCLE
  switch (BWT_CyclingIndex){
      case 0:
        displayWifiStatus(bannerHeight);
      case 1:
        displayBatteryStatus();
      case 2:
        displayTime(titleX, titleY, titleWidth);
    }  
}
void displayBatteryStatus(){
  int batteryLevel = 75;  // Simulate battery level
  // String batteryText = String(batteryLevel) + "%";
  // screen.setTextColor(TFT_WHITE);
  // screen.setCursor(start_X, STARTY + screen.fontHeight() + 5);
  // screen.setTextSize(1);
  // screen.println(batteryText);

  // Indicator
  int innerBatteryHeight = 3;
  int innerBatteryLength = 9;
  int margin = 1;
  uint32_t batteryColor = TFT_SKYBLUE;
  uint32_t bckgdColor = TFT_WHITE;

  
  int start_X = WIDTH - innerBatteryLength - 2*margin - 5;
  int start_Y = STARTY;
  screen.fillRect(start_X, start_Y, start_X + 13, start_Y + 9, bckgdColor);       // battery background

  int capLength = 1;
  int capHeight = 3;
  int capStart_X = start_X + 2*margin + innerBatteryLength;
  int capStart_Y = start_Y+(innerBatteryHeight+2*margin)/2 - 1;
  screen.fillRect(capStart_X, capStart_Y, capStart_X+capLength, capStart_Y + capHeight, TFT_GREEN); // CAP

  // Blocks
  if (batteryLevel >80) screen.fillRect(start_X + margin, start_Y + margin, start_X + innerBatteryLength, start_Y + innerBatteryHeight, batteryColor);
  else if (batteryLevel >40) screen.fillRect(start_X + margin, start_Y + margin, start_X + innerBatteryLength *2/3, start_Y + innerBatteryHeight, batteryColor);
  else screen.fillRect(start_X + margin, start_Y + margin, start_X + innerBatteryLength *1/3, start_Y + innerBatteryHeight, batteryColor);

}
void displayTime(int titleX, int titleY, int titleWidth){
  timeClient.update(); // Update the time from NTP server
  String timeString = String(timeClient.getHours()) + ":" + String(timeClient.getMinutes());

  screen.setTextSize(1);
  int start_X = WIDTH - screen.textWidth(timeString) + 2;//(titleX + titleWidth);
  int start_Y = titleY+1;

  screen.setTextColor(TFT_WHITE);
  screen.setCursor(start_X, start_Y);
  screen.setTextSize(1);
  screen.println(timeString);
}
void displayWifiStatus(int bannerHeight) {
  int wifiStatusX = WIDTH - 8;
  int wifiStatusY = bannerHeight/2; //7
  int dot_size = 2;
  if (WiFi.status() == WL_CONNECTED) {
    screen.fillCircle(wifiStatusX, wifiStatusY, dot_size, TFT_GREEN);
  } else if (WiFi.status() == WL_IDLE_STATUS || WiFi.status() == WL_CONNECT_FAILED || WiFi.status() == WL_DISCONNECTED) {
    screen.fillCircle(wifiStatusX, wifiStatusY, dot_size, TFT_YELLOW);
  } else {
    screen.fillCircle(wifiStatusX, wifiStatusY, dot_size, TFT_RED);
  }
}
void displayCursor(int selectedCoinIndex, int rowHeight, int rowMargin, int yPosition, int leftPadding) {
  int cursorWidth = 2;  // Cursor thickness
  // int cursorAreaHeight = yPosition + selectedCoinIndex * (rowHeight + rowMargin);
  int cursorHeight = rowHeight + 1;  //
  int cursorX = STARTX;
  int cursorY = yPosition - 1;
  
  // CLEAR
  screen.fillRect(cursorX, cursorY, cursorWidth, HEIGHT, TFT_BLACK);
  // CURSOR
  screen.fillRect(cursorX, cursorY, cursorWidth, cursorHeight, TFT_YELLOW);
}
// Main function to display coin information
void displayCoin(float prices[], float changePercentages[]) {
  int leftPadding = STARTX + 3; //3
  int rightMargin = 1;
  int rowHeight = screen.fontHeight();      // Height of the text in a single row : 5
  int rowMargin = rowHeight - 1;             // Margin between rows
  int yPosition = STARTY + screen.fontHeight() + 2 + 5; // Initial Y position, 16 (see displayTitle())

  for (int i = 0; i < numCoins; i++) {
    float price = prices[i];
    String coinName = getFormattedCoinName(coins[i]);
    String priceText = "Error.";

    if (isnan(price) || price <= 0) {
      price = 0.0;
    }
    // Format price to fit into the available width
    if (price > 1000 || screen.textWidth(String(price, 2)) > WIDTH / 2) {
      Serial.println(coinName + " 's decimals have been formatted (>$1000).");
      priceText = String((int)price); // remove decimals if price is too large
      priceText = "$" + priceText; // Add the dollar sign
    } else {
      priceText = "$" + String(price, 2); // Format price with two decimals for smaller values
    }

    // Variables
    float change = changePercentages[i];
    String changeText = (change >= 0 ? "+" : "") + String(change, 2) + "%";
    int coinNameWidth = screen.textWidth(coinName);
    int priceTextWidth = screen.textWidth(priceText);
    int changeTextWidth = screen.textWidth(changeText);
    int changeX = WIDTH - changeTextWidth - rightMargin;
    int priceX = changeX - priceTextWidth - 5;
    int coinNameX = leftPadding;

    // Set text color 
    screen.setTextColor(TFT_WHITE, TFT_BLACK);  // Default text color

      // Set text color based on selection
    if (i == selectedCoinIndex) {
      // Call the cursor display function
      displayCursor(selectedCoinIndex, rowHeight, rowMargin, yPosition ,leftPadding);
      screen.setTextColor(TFT_SKYBLUE);  // Highlighted coin text color TFT_GREENYELLOW TFT_BLUE
    } else {
      screen.setTextColor(TFT_WHITE);  // Default text color
    }


    // Print coin details
    screen.setCursor(coinNameX, yPosition);
    screen.print(coinName);
    screen.setTextColor(TFT_WHITE, TFT_BLACK);
    screen.setCursor(priceX, yPosition);
    screen.print(priceText);
    screen.setCursor(changeX, yPosition);
    screen.setTextColor(change >= 0 ? TFT_GREEN : TFT_RED, TFT_BLACK);
    screen.print(changeText);

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
  screen.fillScreen(TFT_BLACK);  // Clear
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
  // Define drawing area
  int leftPadding = 4;
  int rightMargin = 1;
  int yPosition = STARTY + 1; // Start drawing within the area
  int verticalMargin = yPosition + 15;  // Between name and price.

  // GET COIN INFO
  String coinName = getFormattedCoinName(coins[coinIndex]);
  float change = changePercentages[coinIndex];
  String changeText = (change >= 0 ? "+" : "") + String(change, 2) + "%";
  float price = prices[coinIndex];
  String priceText = "$" + String(price, 2);

  // Calculate positions for coin name, change percentage, and price
  int coinNameWidth = screen.textWidth(coinName);
  int changeTextWidth = screen.textWidth(changeText);
  int priceTextWidth = screen.textWidth(priceText);
  int changeX = ENDX - changeTextWidth - rightMargin;
  int coinNameX = STARTX + leftPadding;
  int priceX = STARTX + leftPadding; // ENDX - priceTextWidth - rightMargin;

  // FORMATTING
  if (changeTextWidth > WIDTH - leftPadding - rightMargin) {
    // Truncate change text if necessary
    changeText = changeText.substring(0, WIDTH / 8);  // Adjust length to fit
    changeTextWidth = screen.textWidth(changeText);  // Recalculate width
  }
  if (priceTextWidth > WIDTH - leftPadding - rightMargin) {
    // Truncate price text if necessary
    priceText = priceText.substring(0, WIDTH / 8);  // Adjust length to fit
    priceTextWidth = screen.textWidth(priceText);  // Recalculate width
  }

  // CoinName and percentage change (size 2)
  screen.setTextSize(2);
  screen.setCursor(coinNameX, yPosition);
  screen.setTextColor(TFT_WHITE);
  screen.print(coinName);

  // % % %
  screen.setCursor(changeX, yPosition);
  screen.setTextColor(change >= 0 ? TFT_GREEN : TFT_RED);
  screen.print(changeText);

  // PRICE
  screen.setCursor(priceX, verticalMargin);
  screen.setTextColor(TFT_WHITE);
  screen.print(priceText);
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
  screen.fillRect(x + (candleWidth - wickWidth) / 2, yHigh, wickWidth, yLow - yHigh, bodyColor);
  // Draw the candle body
  screen.fillRect(x, yBodyTop, candleWidth, yBodyBottom - yBodyTop, bodyColor);
}
void displayCryptoChart() {
  int x = STARTX;
  int yStart = STARTY + 1 + 15;   //see displaySingleCryptoInfo()
  int yEnd = HEIGHT;
  
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
    screen.setCursor(0, yPosition - 8); // Adjust y-position to center the text
    screen.setTextSize(1);
    screen.setTextColor(TFT_WHITE, TFT_BLACK);
    screen.print(String(price, 2)); // Print the price with 2 decimal points
    // Optional: Draw horizontal grid line
    screen.drawLine(30, yPosition, screen.width(), yPosition, TFT_DARKGREY);
  }
}




//====================================DEBUG============================================================
void debugPrintColorfulPixels() {
  for (int y = 0; y < screen.height(); y++) {
    // Choose a color gradient based on the y position
    uint16_t color = screen.color565(y % 32 * 8, y % 64 * 4, y % 128 * 2); // RGB gradient
    for (int x = 0; x < screen.width(); x++) {
      screen.drawPixel(x, y, color);
    }
  }
}
