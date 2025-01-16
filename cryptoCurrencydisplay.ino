#include <Arduino.h>
#include <WiFi.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <esp_http_client.h>

TFT_eSPI tft = TFT_eSPI();  // Initialize TFT screen

// WiFi Credentials
const char* ssid = "";
const char* pswd = "";

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
const unsigned long interval = 5000;  // Update every 5 seconds

void setup() {
  Serial.begin(115200);

  tft.init();
  tft.setRotation(4);

  pinMode(buttonPin, INPUT_PULLUP);  // Set button pin as input with pull-up

  connectToWiFi();
}

void loop() {
  readButton();
  // if(lastButtonState == 1){
  //       screenIndex += 1;
  //       // previousMillis = 0;
  //       tft.fillCircle(10, 10, 3, TFT_GREEN);
  // }

  
  updateDisplay();
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

void readButton() {
  // Read the state of the button
  int reading = digitalRead(buttonPin);

  // If the state of the button has changed, reset the debounce timer
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  // If the state has been stable for the debounce delay, proceed
  if ((millis() - lastDebounceTime) > debounceDelay) {
    // If the button state has changed and it was previously not pressed
    if (reading != buttonState) {
      buttonState = reading;

      // Only toggle the screenIndex if the button is pressed (LOW)
      if (buttonState == LOW) {
        screenIndex = (screenIndex == 0) ? 1 : 0;  // Toggle between 0 and 1
        previousMillis = millis();  // Reset the timer to force immediate update
      }
    }
  }

  lastButtonState = reading;  // Save the current state for the next loop
}


//=======Updates_MILLIS()=======
// This function will update the screen every 10 seconds automatically
void updateDisplay() {
  unsigned long currentMillis = millis();
  // Check if enough time has passed to update the screen
  if (currentMillis - previousMillis >= interval || lastScreenIndex != screenIndex) {
    tft.fillScreen(TFT_PURPLE);
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
    previousMillis = currentMillis;  // Update the timer
    lastScreenIndex = screenIndex;  // Update Index
  }
}



//====CRYPTO_VIEW===============
void cryptoView() {
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
  tft.fillScreen(TFT_GREEN);
  
}
