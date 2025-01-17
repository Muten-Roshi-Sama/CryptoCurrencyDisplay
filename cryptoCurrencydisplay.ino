
#include <Arduino.h>
#include <Button.h>
#include <WiFi.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <esp_http_client.h>

TFT_eSPI tft = TFT_eSPI();  // Initialize TFT screen

// WiFi Credentials
const char* ssid = "REDACTED";
const char* pswd = "REDACTED";

// Coins to display
const char* coins[] = {"BTCUSDT", "ETHUSDT", "SOLUSDT", "XRPUSDT", "ADAUSDT", "DOGEUSDT", "USDCUSDT", "BUSDUSDT"};
const int numCoins = sizeof(coins) / sizeof(coins[0]);
float prices[numCoins] = {0.0};
float changePercentages[numCoins] = {0.0};
uint16_t coinColors[] = {TFT_GOLD, TFT_SKYBLUE, TFT_RED, TFT_GREEN, TFT_CYAN, TFT_PINK, TFT_MAGENTA, TFT_GREENYELLOW, TFT_VIOLET};
int selectedCoinIndex = 0; // with button2, change which coin to display in single view

// Button pin setup
Button button1(19);
Button button2(21);
Button button3(22);
bool buttonFlag = false;

// Screen cycle state
int screenIndex = 0;  // 0: Coin Info Screen, 1: Wi-Fi Info Screen, 2: Title Screen
int lastScreenIndex = 0;

// Global Millis Variables for Screen Update
unsigned long previousMillis = 0;  // Global timer to track screen update intervals
const unsigned long interval = 5000;  

// Global Millis Variables for Data Update
unsigned long previousDataMillis = 0;  // Timer for data update
const unsigned long dataUpdateInterval = 9000;  // Update coin data every 30 seconds

// CHART
struct Candle {
  float o;  // Open price
  float h;  // High price
  float l;  // Low price
  float c;  // Close price
  float v;  // Volume
};
Candle candles[100];  // Array to store candle data (modify size as needed)
const unsigned long dataHistoricalUpdateInterval = 11000;

// Binance API endpoint and parameters
const char* chart_interval = "1h";     // 1-hour interval
const int limit = 24;            // Number of data points (last 24 hours)
// Array to store hourly closing prices
float hourlyPrices[24];

//=======================================================================================
void setup() {
  Serial.begin(115200);
  button1.begin();
  button2.begin();
  button3.begin();

  // TFT
  tft.init();
  tft.setRotation(4);
  connectToWiFi();
  // Data
  fetchHistoricalData(selectedCoinIndex);
  updateALLCoinsArray();
}
void loop() {
  if (button1.pressed()){
    Serial.println("Button pressed! Toggling screen index.");
    screenIndex = (screenIndex + 1) % 2;  // Toggle between 0 and 1
  }
  if (button2.pressed() && screenIndex==0) {
    selectedCoinIndex = (selectedCoinIndex + 1) % numCoins;  // Cycle forward
    // fetchHistoricalData(selectedCoinIndex);
    buttonFlag = true;
  }

  updateDisplay();
  // updateCoinArray_BackgroundLoop();

  // Debug: Direct button state
  // Serial.print("Current button state: ");
  // Serial.println(digitalRead(buttonPin));
  // unsigned long currentMillis = millis();
  // Serial.print("Millis diff: ");
  // Serial.println(currentMillis-previousMillis);
  // Serial.print("Current screenIndex: ");
  // Serial.println(screenIndex);
}
//=======================================================================================
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
void updateALLCoinsArray(){
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

//=======Updates_MILLIS()=======
void updateDisplay() {
  unsigned long currentMillis = millis();
  // UPDATE via Timer
  if (currentMillis - previousMillis >= interval) {
    // tft.fillScreen(TFT_NAVY);
    myFSM();
    previousMillis = currentMillis;  // Update the timer
  }
  // CHANGE SCREEN via button
  if (lastScreenIndex != screenIndex) {
    // tft.fillScreen(TFT_GOLD);
    myFSM();
    lastScreenIndex = screenIndex;  // Update Index
  }
  if (buttonFlag){
    myFSM();
    buttonFlag = false;
  }
  if (currentMillis - previousDataMillis >= dataHistoricalUpdateInterval) {
    // fetchHistoricalData(selectedCoinIndex);
    
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


//====CRYPTO_VIEW===============
void cryptoView() {
  // updateCoinsArrays();
  tft.fillScreen(TFT_BLACK);  // Clear the screen
  displayTitle();
  displayWifiStatus();
  displayCoinInfo(prices, changePercentages);
}
// Display Title and Wi-Fi status
void displayTitle() {
  String title = "Crypto Tracker";
  int bannerHeight = 13;
  int bannerColor = TFT_RED;
  int titleColor = TFT_WHITE;
  tft.fillRect(0, 0, tft.width(), bannerHeight, bannerColor);
  int titleWidth = tft.textWidth(title);
  int titleX = (tft.width() - titleWidth) / 2;
  int titleY = (bannerHeight - 4) / 2;
  tft.setTextColor(titleColor, bannerColor);
  tft.setCursor(titleX, titleY);
  tft.setTextSize(1);
  tft.println(title);
}
void displayWifiStatus() {
  int wifiStatusX = tft.width() - 8;
  int wifiStatusY = 7;
  int dot_size = 3;
  if (WiFi.status() == WL_CONNECTED) {
    tft.fillCircle(wifiStatusX, wifiStatusY, dot_size, TFT_GREEN);
  } else if (WiFi.status() == WL_IDLE_STATUS || WiFi.status() == WL_CONNECT_FAILED || WiFi.status() == WL_DISCONNECTED) {
    tft.fillCircle(wifiStatusX, wifiStatusY, dot_size, TFT_YELLOW);
  } else {
    tft.fillCircle(wifiStatusX, wifiStatusY, dot_size, TFT_RED);
  }
}

void displayCursor(int selectedCoinIndex, int rowHeight, int rowMargin, int yPosition, int leftPadding) {
  int cursorWidth = 2;  // Cursor thickness
  int cursorHeight = rowHeight + 1;  //
  
  // Clear previous cursor (full column width)
  tft.fillRect(0, yPosition + selectedCoinIndex * (rowHeight + rowMargin), cursorWidth, tft.height(), TFT_GREEN);

  // Draw new cursor for the selected coin
  tft.fillRect(0, yPosition -1 , cursorWidth, cursorHeight, TFT_YELLOW);
}

// Main function to display coin information
void displayCoinInfo(float prices[], float changePercentages[]) {
  int maxWidth = tft.width();
  int leftPadding = 3;
  int rightMargin = 2;
  int rowHeight = 5;         // Height of the text in a single row
  int rowMargin = 10;         // Margin between rows
  int yPosition = 16;        // Initial Y position

  for (int i = 0; i < numCoins; i++) {
    String coinName = getFormattedCoinName(coins[i]);
    float price = prices[i];
    if (isnan(price) || price <= 0) {
      price = 0.0;
    }
    String priceText = "$" + String(price, 2);
    while (tft.textWidth(priceText) > maxWidth / 2) {
      priceText = "$" + String(price, priceText.length() - 1);
    }
    float change = changePercentages[i];
    String changeText = (change >= 0 ? "+" : "") + String(change, 2) + "%";

    int coinNameWidth = tft.textWidth(coinName);
    int priceTextWidth = tft.textWidth(priceText);
    int changeTextWidth = tft.textWidth(changeText);
    int changeX = maxWidth - changeTextWidth - rightMargin;
    int priceX = changeX - priceTextWidth - 5;
    int coinNameX = leftPadding;

    // Set text color based on selection
    if (i == selectedCoinIndex) {
      tft.setTextColor(TFT_BLACK);  // Highlighted coin text color
    } else {
      tft.setTextColor(TFT_WHITE);  // Default text color
    }

    // Print coin details
    tft.setCursor(coinNameX, yPosition);
    tft.print(coinName);
    tft.setCursor(priceX, yPosition);
    tft.print(priceText);
    tft.setCursor(changeX, yPosition);
    tft.setTextColor(change >= 0 ? TFT_GREEN : TFT_RED);
    tft.print(changeText);

    // Move to the next row
    yPosition += rowHeight + rowMargin;
  }

  // Call the cursor display function
  displayCursor(selectedCoinIndex, rowHeight, rowMargin, yPosition ,leftPadding);
}




//====Single_Crypto_View======
void singleCryptoView() {
  // AREA :
  //      - displaySingleCryptoInfo  :  (0,0) to (xmax, 20)
  //      - displayCandlestickChart  :  (0,20) to (xmax, 100)
  //
  tft.fillScreen(TFT_BLACK);  // Clear
  displaySingleCryptoInfo(selectedCoinIndex, prices, changePercentages);
  displayCandlestickChart();
  // debugPrintColorfulPixels();
}

void displaySingleCryptoInfo(int coinIndex, float prices[], float changePercentages[]) {
  // Define drawing area
  int startX = 0;
  int startY = 4;
  int endX = tft.width();
  int endY = 100; // Set specific height for this display section
  int maxWidth = endX - startX; // Maximum width within the defined area
  int leftPadding = 4;
  int rightMargin = 2;
  int yPosition = startY; // Start drawing within the area
  // Clear only the specified area
  tft.fillRect(startX, startY, maxWidth, endY - startY, TFT_BLACK);
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
  int changeX = endX - changeTextWidth - rightMargin;
  int coinNameX = startX + leftPadding;
  int priceX = startX + leftPadding; // endX - priceTextWidth - rightMargin;
  // FORMATTING
  if (changeTextWidth > maxWidth - leftPadding - rightMargin) {
    // Truncate change text if necessary
    changeText = changeText.substring(0, maxWidth / 8);  // Adjust length to fit
    changeTextWidth = tft.textWidth(changeText);  // Recalculate width
  }
  if (priceTextWidth > maxWidth - leftPadding - rightMargin) {
    // Truncate price text if necessary
    priceText = priceText.substring(0, maxWidth / 8);  // Adjust length to fit
    priceTextWidth = tft.textWidth(priceText);  // Recalculate width
  }
  // Set text size for the coin name and percentage change (size 2)
  tft.setTextSize(2);
  // Print the coin name on the left (adjusted for overflow)
  tft.setCursor(coinNameX, yPosition);
  tft.setTextColor(TFT_WHITE);
  tft.print(coinName);
  // Print the percentage change on the right
  tft.setCursor(changeX, yPosition);
  tft.setTextColor(change >= 0 ? TFT_GREEN : TFT_RED);
  tft.print(changeText);
  // Move to the next line for the price
  yPosition += 20; // Move down for the next line (adjust based on text size)
  // Print the price on the next line
  tft.setCursor(priceX, yPosition);
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
void displayCandlestickChart() {
  int x = 10;
  int yStart = 50;   
  int yEnd = tft.height();
  
  // Find the min and max prices from the candles
  float minPrice = candles[0].l;
  float maxPrice = candles[0].h;
  for (int i = 1; i < limit; i++) {
    if (candles[i].l < minPrice) minPrice = candles[i].l;
    if (candles[i].h > maxPrice) maxPrice = candles[i].h;
  }
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




//===DEBUG====
void debugPrintColorfulPixels() {
  for (int y = 0; y < tft.height(); y++) {
    // Choose a color gradient based on the y position
    uint16_t color = tft.color565(y % 32 * 8, y % 64 * 4, y % 128 * 2); // RGB gradient
    for (int x = 0; x < tft.width(); x++) {
      tft.drawPixel(x, y, color);
    }
  }
}
