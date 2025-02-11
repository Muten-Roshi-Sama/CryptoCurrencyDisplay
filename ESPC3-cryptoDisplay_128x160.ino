//V.1.0-128x160- ESPC3

//This code has been (painfully) adapted to work with an Esp32-C3. The original TFT library I used (TFT_eSPI, by Bodmer) isn't yet fully working with the C3, so this code uses the more popular AdaFruit ST7735 library.
// this new lib isn't as powerfull as Bodmer's so Helper functions have been introduced to make the transition easier.
//
// PINOUT:
// Vcc - 3v3,  GND-GND,  CS-10,  RST-9,  AO/DC-8,  SDA-6,  SCK-4,  LED/BLK-Vcc

#include <Arduino.h>
#include <EasyButton.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <HTTPClient.h>
#include <esp_http_client.h>

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <SPI.h>

#define TFT_CS        10
  #define TFT_RST        9 // Or set to -1 and connect to Arduino RESET pin
  #define TFT_DC         8
  #define TFT_GREY 0x7BEF
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// LCD Dimensions : 128x128
#define STARTX 0
#define STARTY 0
#define ENDX 160
#define ENDY 128
int WIDTH = ENDX - STARTX;    //160
int HEIGHT = ENDY - STARTY;  //`128

// WIFI
struct WiFiCredential {
  const char* ssid;
  const char* pswd;
};
bool isConnected = false;
bool networkScanFlag = false;
int numNetworks;
unsigned long lastWifiCheck = 0;
WiFiCredential wifiList[] = {
  {"YouReally", "WannaKnow?"}
}; 

// Coins to display (MAX. 7)
const char* coins[] = {"BTCUSDT", "ETHUSDT", "SOLUSDT", "XRPUSDT", "ADAUSDT", "DOGEUSDT", "USDCUSDT", "BUSDUSDT"};
const int numCoins = sizeof(coins) / sizeof(coins[0]);
float prices[numCoins] = {0.0};
float changePercentages[numCoins] = {0.0};
uint16_t coinColors[] = {ST77XX_GOLD, ST77XX_SKYBLUE, ST77XX_RED, ST77XX_GREEN, ST77XX_CYAN, ST77XX_PINK, ST77XX_MAGENTA, ST77XX_GREENYELLOW, ST77XX_VIOLET};


// Button pin setup
EasyButton button1(0);    //TOGGLE between screens
EasyButton button2(1);   //CYCLE cursor downward
bool cursorButtonFlag = false; //for cursor button.

// Screen cycle state
int screenIndex = 0;  // 0: Coin Info Screen, 1: Wi-Fi Info Screen, 2: Title Screen, 99: boot
int lastScreenIndex = 0;
int selectedCoinIndex = 0;    // Cursor
int lastSelectedCoinIndex = 0;
int BWT_CyclingIndex = 0;       // Index to cycle between showing the battery or time
int InfoCyclingMillis = 0; // 0 = Wi-Fi, 1 = Battery, 2 = Time
const unsigned long InfoCyclingInterval = 4000;

// Millis Screen Update
unsigned long previousMillis = 0;  // Global timer to track screen update intervals
const unsigned long interval = 5000;  // .. 

// Millis Data Update
unsigned long previousDataMillis = 0;  // Timer for data update
const unsigned long dataUpdateInterval = 9000;  // Update coin data every 30 seconds
unsigned long previousCandleMillis = 0;
const unsigned long candleUpdateInterval = 10000;  // Update coin data every 30 seconds

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
// DigitalRainAnim digitalRainAnim = DigitalRainAnim(); 




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















void setup(void) {
  Serial.begin(115200);
  // delay(500);
  

  // Use this initializer if using a 1.8" TFT screen:
  tft.initR(INITR_BLACKTAB);  
  tft.setRotation(1);  // horizontal, pins to the right
  
  // start = millis();
  connectToWiFi();
  // Serial.print(millis() - start);

  // BUTTONS
  buttonsInit();

  tft.fillScreen(ST77XX_BLACK);

  screenIndex = 99;  //boot INDEX
  // myFSM();
  
  
  
  // Time
  timeClient.begin();    // Start NTPClient to get the time
  timeClient.update();  // Set initial time (Optional, but ensures time starts from a known point)
}




void loop() {
  if (millis()-lastWifiCheck >1000)Serial.println("I'm Alive !");
    
  button1.read();
  button2.read();

  
  
  
  if (millis()-lastWifiCheck >2000) connectToWiFi();

  
    
  
  // LOOPS
  // if(isLoadingFinished()) updateDisplay();
  // else loadingBar();


  // digitalRainAnim.loop();
    
  // delay(500);
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
            drawText(0,0,ST77XX_GREEN,String("Found " + String(numNetworks) + " networks."));
        }
        networkScanFlag = true; 
    }

    // Find and connect to wifi from given list
    if (!connecting) {  
        for (int i = 0; i < sizeof(wifiList) / sizeof(wifiList[0]); i++) {
            for (int j = 0; j < numNetworks; j++) {
                if (WiFi.SSID(j) == wifiList[i].ssid) {
                    String message = String("Connecting to: " + String(wifiList[i].ssid) + ".");
                    textLength = textWidth(message, tft);
                    rowNum = (j+1);
                    Serial.printf("Connecting to SSID: %s\n", wifiList[i].ssid);
                    drawText(0,rowNum*6,ST77XX_GREEN ,message);
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
                drawText(textLength ,rowNum*6,ST77XX_GREEN,"  Connected!");
                Serial.print("IP Address: ");
                Serial.println(WiFi.localIP());
                isConnected = true;
                connecting = false;
                
            } else {
                Serial.print(".");
                attempts++;
                if (attempts >= 10) {  // Try for 10 seconds max (10 * 1000ms)
                    Serial.println("\nFailed to connect.");
                    drawText(textLength ,rowNum*6,ST77XX_RED,"  Failed to connect.");
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

















//------Helpers-------
                          // ALL TFT_BLACK etc need to be updated as ST77XX_BLACK (same for all colors).
int textWidth(String text, Adafruit_GFX &tft) {
  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(text.c_str(), 0, 0, &x1, &y1, &w, &h);
  return w;
}













