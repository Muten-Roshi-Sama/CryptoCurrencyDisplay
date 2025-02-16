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
  {"", ""},
  
}; 

// =============== TIME SETTINGS ===============
const char* ntpServer = "pool.ntp.org";   
const long utcOffsetInSeconds = 0;
WiFiUDP udp;
NTPClient timeClient(udp, ntpServer, utcOffsetInSeconds, 30000);





// =============== UI VARIABLES ===============
#define mainView_bannerColor ST77XX_RED
#define cursorColor ST77XX_BLUE   //Dodger blue
#define highlightColor ST77XX_GOLD
#define bootLogColor ST77XX_DARKGREEN

int bannerHeight;                               //MAIN VIEW
int coinsLeftPadding = 4;
int singleCrypto_Title_Height;;                      //SingleCryptoView


    // From ILI9341 - ALL <TFT_BLACK> etc need to be updated as ST77XX_BLACK (same for all colors).
    #define ST77XX_NAVY 0x000F         ///<   0,   0, 123
    #define ST77XX_DARKGREEN 0x03E0    ///<   0, 125,   0
    #define ST77XX_DARKCYAN 0x03EF     ///<   0, 125, 123
    #define ST77XX_MAROON 0x7800       ///< 123,   0,   0
    #define ST77XX_PURPLE 0x780F       ///< 123,   0, 123
    #define ST77XX_OLIVE 0x7BE0        ///< 123, 125,   0
    #define ST77XX_LIGHTGREY 0xC618    ///< 198, 195, 198
    #define ST77XX_DARKGREY 0x7BEF     ///< 123, 125, 123
    #define ST77XX_GREENYELLOW 0xAFE5  ///< 173, 255,  41
    #define ST77XX_PINK 0xFC18         ///< 255, 130, 198

    #define ST77XX_GREY      0x7BEF
    #define ST77XX_GOLD     0xFFD700 //0xFD20   // Gold  
    #define ST77XX_SKYBLUE  0x87CEEB // Sky Blue
    #define ST77XX_VIOLET   0x7F00   // Violet




//============= FUNCTION DECLARATION =============
int textWidth(String text, Adafruit_GFX &tft);
int textHeight(String text, Adafruit_GFX &tft);
void drawText(int x, int y, int textColor, String text);











#endif