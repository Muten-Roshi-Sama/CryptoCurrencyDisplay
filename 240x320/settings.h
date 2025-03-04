// v1.0
#ifndef settings_h
#define settings_h


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
  {"Proximus-Home-577423", "yz643yhhx34reys3"},
  {"SamsungPrintWireless", "tftgtrrr"},
  {"Proximus-Home-729311", "afunj3ze4bmzax32"},  //Papou
  {"VOO-8RLHU0U", "3fAprazdtKzGRs36Cq"}
}; 

// =============== TIME SETTINGS ===============
const char* ntpServer = "pool.ntp.org";   
const long utcOffsetInSeconds = 0;
WiFiUDP udp;
NTPClient timeClient(udp, ntpServer, utcOffsetInSeconds, 30000);


// =============== Colors ===============
    // Additionnal colors
    #define ILI9341_GREY      0x7BEF
    #define ILI9341_GOLD     0xFFD700 //0xFD20   // Gold  
    #define ILI9341_SKYBLUE  0x87CEEB // Sky Blue
    #define ILI9341_VIOLET   0x7F00   // Violet

// =============== UI VARIABLES ===============
#define mainView_bannerColor ILI9341_RED
#define cursorColor ILI9341_DARKCYAN
#define highlightColor ILI9341_GOLD
#define bootLogColor ILI9341_GREENYELLOW

int bannerHeight;                               //MAIN VIEW
int coinsLeftPadding = 4;
int singleCrypto_Title_Height;;                      //SingleCryptoView







//============= FUNCTION DECLARATION =============
int textWidth(String text, Adafruit_GFX &tft, int textSize);
int textHeight(String text, Adafruit_GFX &tft, int textSize);
void drawText(int x, int y, int textColor, String text);











#endif