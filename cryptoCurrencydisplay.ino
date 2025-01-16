#include <Arduino.h>
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

// Button pin setup
const int buttonPin = 21;  // Button connected to pin 2
int buttonState = 0;  // Current state of the button
int lastButtonState = 0;  // Previous state of the button
unsigned long lastDebounceTime = 0;  // Last debounce time
unsigned long debounceDelay = 50;  // Debounce delay

// Screen cycle state
int screenIndex = 0;  // 0: Coin Info Screen, 1: Wi-Fi Info Screen, 2: Title Screen
int lastScreenIndex = 0;


// Global Millis Variables for Screen Update
unsigned long previousMillis = 0;  // Global timer to track screen update intervals
const unsigned long interval = 5000;  

// Global Millis Variables for Data Update
unsigned long previousDataMillis = 0;  // Timer for data update
const unsigned long dataUpdateInterval = 4000;  // Update coin data every 30 seconds
int currentCoinIndex = 0;  // Keep track of which coin to update next


void setup() {
  Serial.begin(115200);

  tft.init();
  tft.setRotation(4);

  pinMode(buttonPin, INPUT_PULLUP);  // Set button pin as input with pull-up

  connectToWiFi();
  updateALLCoinsArrays();
}

void loop() {
  readButton();  
  updateDisplay();
  // updateCoinArray_BackgroundLoop();

  // Debug: Direct button state
  Serial.print("Current button state: ");
  Serial.println(digitalRead(buttonPin));
  unsigned long currentMillis = millis();
  Serial.print("Millis diff: ");
  Serial.println(currentMillis-previousMillis);
  Serial.print("Current screenIndex: ");
  Serial.println(screenIndex);
}


//=======MISC=========================
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
void updateALLCoinsArrays() {
  // Loop through all coins and fetch data
  for (int i = 0; i < numCoins; i++) {
    updateCoinArray(i);
  }
} // at setup ONLY
void updateCoinArray_BackgroundLoop() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousDataMillis >= dataUpdateInterval) {
    updateCoinArray(currentCoinIndex);  // Update one coin at a time
    
    // // Move to next coin
    currentCoinIndex = (currentCoinIndex + 1) % numCoins;
    // updateALLCoinsArrays();
    // Update the data 
    previousDataMillis = currentMillis;
  }
} // Non-blocking function to update coin data one by one
void updateCoinArray() {
  // unsigned long currentMillis = millis();
  // if (currentMillis - previousDataMillis >= dataUpdateInterval) {
    updateCoinArray(currentCoinIndex);  // Update one coin at a time
    // // Move to next coin
    currentCoinIndex = (currentCoinIndex + 1) % numCoins;
    // updateALLCoinsArrays();
    // Update the data 
    // previousDataMillis = currentMillis;
  // }
}

void readButton() {
    int reading = digitalRead(buttonPin);  // Read current state of the button

    // Check for a state change
    if (reading != lastButtonState) {
        lastDebounceTime = millis();  // Reset debounce timer
    }

    // Apply debounce delay
    if ((millis() - lastDebounceTime) > debounceDelay) {
        // Check if state has changed
        if (reading != buttonState) {
            buttonState = reading;

            // Button pressed (LOW)
            if (buttonState == LOW) {
                Serial.println("Button pressed! Toggling screen index.");
                screenIndex = (screenIndex + 1) % 2;  // Toggle between 0 and 1
            }
        }
    }

    lastButtonState = reading;  // Save the state for the next loop
} //changes screenIndex when pressed


//=======Updates_MILLIS()=======
// This function will update the screen every 10 seconds automatically
void updateDisplay() {
  unsigned long currentMillis = millis();
  Serial.print("Millis diff: ");
  Serial.println(currentMillis-previousMillis);
  Serial.print("Current screenIndex: ");
  Serial.println(screenIndex);

  // UPDATE via Timer
  if (currentMillis - previousMillis >= interval) {
    tft.fillScreen(TFT_NAVY);
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
    updateCoinArray();
    previousMillis = currentMillis;  // Update the timer
    
  }
  // CHANGE SCREEN via button
  if (lastScreenIndex != screenIndex) {
    tft.fillScreen(TFT_GOLD);
    // Serial.print("screenIndex :");
    // Serial.println(screenIndex);
    // Serial.print("lastScreenIndex :");
    // Serial.println(lastScreenIndex);
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
    updateCoinArray();
    lastScreenIndex = screenIndex;  // Update Index
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

// Display coin information
void displayCoinInfo(float prices[], float changePercentages[]) {
  int maxWidth = tft.width();
  int leftPadding = 2;
  int rightMargin = 2;
  int yPosition = 16;

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

    tft.setCursor(coinNameX, yPosition);
    tft.setTextColor(TFT_WHITE);
    tft.print(coinName);

    tft.setCursor(priceX, yPosition);
    tft.print(priceText);

    tft.setCursor(changeX, yPosition);
    tft.setTextColor(change >= 0 ? TFT_GREEN : TFT_RED);
    tft.print(changeText);

    yPosition += 15;
  }
}

String getFormattedCoinName(const char* coinSymbol) {
  String coinName = String(coinSymbol);
  if (coinName.endsWith("USDT")) {
    coinName = coinName.substring(0, coinName.length() - 4);
  }
  return coinName;
}




//====Single_Crypto_View======
void singleCryptoView() {
  int coinIndex = 0; // For example, use index 0 for "BTCUSDT"
  tft.fillScreen(TFT_GREEN);  // Clear the screen with a color (can be changed)

  displaySingleCryptoInfo(coinIndex, prices, changePercentages);  // Display the info for the selected coin
}
  




void displaySingleCryptoInfo(int coinIndex, float prices[], float changePercentages[]) {
  tft.fillScreen(TFT_BLACK);
  int maxWidth = tft.width();
  int leftPadding = 4;
  int rightMargin = 2;
  int yPosition = 5;

  // Get the formatted coin name
  String coinName = getFormattedCoinName(coins[coinIndex]);
  
  // Get the percentage change for the coin
  float change = changePercentages[coinIndex];
  String changeText = (change >= 0 ? "+" : "") + String(change, 2) + "%";
  
  // Get the price for the coin
  float price = prices[coinIndex];
  String priceText = "$" + String(price, 2);

  // Calculate positions for coin name, change percentage, and price
  int coinNameWidth = tft.textWidth(coinName);
  int changeTextWidth = tft.textWidth(changeText);
  int priceTextWidth = tft.textWidth(priceText);

  int changeX = maxWidth - changeTextWidth - rightMargin;
  int coinNameX = leftPadding;
  int priceX = maxWidth - priceTextWidth - rightMargin;

  // Set text size for the coin name and percentage change (size 2)
  tft.setTextSize(2);

  // Print the coin name on the left
  tft.setCursor(coinNameX, yPosition);
  tft.setTextColor(TFT_WHITE);
  tft.print(coinName);

  // Print the percentage change on the right
  tft.setCursor(changeX, yPosition);
  tft.setTextColor(change >= 0 ? TFT_GREEN : TFT_RED);
  tft.print(changeText);

  // Move to the next line for the price
  yPosition += 24;  // Move down for the next line (adjust based on text size)

  // Set text size for the price (size 1)
  // tft.setTextSize(1);

  // Print the price on the next line
  tft.setCursor(priceX, yPosition);
  tft.setTextColor(TFT_WHITE);
  tft.print(priceText);
}


