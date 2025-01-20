//V.1
#include <Arduino.h>
#include <Button.h>
#include <WiFi.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <esp_http_client.h>

TFT_eSPI screen = TFT_eSPI();  // Initialize TFT screen

// LCD Dimensions : 128x128
#define STARTX 2
#define STARTY 2
#define endX 127
#define endY 129
int WIDTH = endX - STARTX;    //125
int HEIGHT = endY - STARTY;  //`127

// WiFi Credentials
const char* ssid = "";
const char* pswd = "";

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
}
void loop() {
  if (button1.pressed()){
    Serial.println("Button pressed! Toggling screen index.");
    screenIndex = (screenIndex + 1) % 2;  // Toggle between 0 and 1
  }
  // CURSOR
  if (button2.pressed() && screenIndex==0) {
    selectedCoinIndex = (selectedCoinIndex + 1) % numCoins;  // Cycle forward
    // fetchHistoricalData(selectedCoinIndex);
    buttonFlag = true; // for cursor
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

//=======Updates_MILLIS()=======
void updateDisplay() {
  unsigned long currentMillis = millis();
  // UPDATE via Timer
  if (currentMillis - previousMillis >= interval) {
    // screen.fillScreen(TFT_NAVY);
    myFSM();
    previousMillis = currentMillis;  // Update the timer
  }
  // CHANGE SCREEN via button
  if (lastScreenIndex != screenIndex) {
    // screen.fillScreen(TFT_GOLD);
    myFSM();
    lastScreenIndex = screenIndex;  // Update Index
  }
  // Cursor
  if (buttonFlag){
    myFSM();
    buttonFlag = false;
  }
  // DATA
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
  screen.fillScreen(TFT_BLACK);  // Clear the screen
  displayTitle();
  displayWifiStatus();
  displayCoin(prices, changePercentages);
}
// Display Title and Wi-Fi status
void displayTitle() {
  String title = "Crypto Tracker";
  int bannerHeight = screen.fontHeight()+3; // 13
  int bannerColor = TFT_RED;
  int titleColor = TFT_WHITE;
  screen.fillRect(STARTX, STARTY, WIDTH, bannerHeight, bannerColor);
  int titleWidth = screen.textWidth(title);
  int titleX = (WIDTH - titleWidth) / 2;
  int titleY = (bannerHeight - 4) / 2;
  screen.setTextColor(titleColor, bannerColor);
  screen.setCursor(titleX, titleY);
  screen.setTextSize(1);
  screen.println(title);
}
void displayWifiStatus() {
  int wifiStatusX = screen.width() - 8;
  int wifiStatusY = 7;
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




//====Single_Crypto_View======
void singleCryptoView() {
  // AREA :
  //      - displaySingleCryptoInfo  :  (0,0) to (xmax, 20)
  //      - displayCandlestickChart  :  (0,20) to (xmax, 100)
  //
  screen.fillScreen(TFT_BLACK);  // Clear
  displaySingleCryptoInfo(selectedCoinIndex, prices, changePercentages);
  displayCandlestickChart();
  // debugPrintColorfulPixels();
}

void displaySingleCryptoInfo(int coinIndex, float prices[], float changePercentages[]) {
  // Define drawing area
  // int startX = 0;
  // int startY = 4;
  // int endX = screen.width();
  // int endY = 100; // Set specific height for this display section
  int WIDTH = endX - STARTX; // Maximum width within the defined area
  int leftPadding = 4;
  int rightMargin = 2;
  int yPosition = STARTY; // Start drawing within the area
  // Clear only the specified area
  // screen.fillRect(startX, startY, WIDTH, endY - startY, TFT_BLACK);
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
  int changeX = endX - changeTextWidth - rightMargin;
  int coinNameX = STARTX + leftPadding;
  int priceX = STARTX + leftPadding; // endX - priceTextWidth - rightMargin;
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
  // Set text size for the coin name and percentage change (size 2)
  screen.setTextSize(2);
  // Print the coin name on the left (adjusted for overflow)
  screen.setCursor(coinNameX, yPosition);
  screen.setTextColor(TFT_WHITE);
  screen.print(coinName);
  // Print the percentage change on the right
  screen.setCursor(changeX, yPosition);
  screen.setTextColor(change >= 0 ? TFT_GREEN : TFT_RED);
  screen.print(changeText);
  // Move to the next line for the price
  yPosition += 20; // Move down for the next line (adjust based on text size)
  // Print the price on the next line
  screen.setCursor(priceX, yPosition);
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
void displayCandlestickChart() {
  int x = 10;
  int yStart = 50;   
  int yEnd = screen.height();
  
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
    screen.setCursor(0, yPosition - 8); // Adjust y-position to center the text
    screen.setTextSize(1);
    screen.setTextColor(TFT_WHITE, TFT_BLACK);
    screen.print(String(price, 2)); // Print the price with 2 decimal points
    // Optional: Draw horizontal grid line
    screen.drawLine(30, yPosition, screen.width(), yPosition, TFT_DARKGREY);
  }
}




//===DEBUG====
void debugPrintColorfulPixels() {
  for (int y = 0; y < screen.height(); y++) {
    // Choose a color gradient based on the y position
    uint16_t color = screen.color565(y % 32 * 8, y % 64 * 4, y % 128 * 2); // RGB gradient
    for (int x = 0; x < screen.width(); x++) {
      screen.drawPixel(x, y, color);
    }
  }
}
