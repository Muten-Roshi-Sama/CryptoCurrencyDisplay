#ifndef cryptoView_h
#define cryptoView_h


// =============== CRYPTO DATA ===============
const char* coins[] = {"BTCUSDT", "ETHUSDT", "SOLUSDT", "XRPUSDT", "ADAUSDT", "DOGEUSDT", "USDCUSDT", "BUSDUSDT"};
const int numCoins = sizeof(coins) / sizeof(coins[0]);

float prices[numCoins] = {0.0};
float changePercentages[numCoins] = {0.0};

uint16_t coinColors[] = {
    ST77XX_GOLD, ST77XX_SKYBLUE, ST77XX_RED, ST77XX_GREEN, 
    ST77XX_CYAN, ST77XX_PINK, ST77XX_MAGENTA, ST77XX_GREENYELLOW
};






//================================CRYPTO_VIEW=========================================================
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


// DISPLAY
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
  switch (infoCyclingIndex){
      case 0:
        displayBatteryStatus(titleWidth, titleX, bannerHeight, bannerColor);
        break;
      case 1:
        displayTime(titleX, titleY, titleWidth);
        break;
    }  
}


// full call
void cryptoView() {
  // updateCoinsArrays();
  tft.fillScreen(ST77XX_BLACK);  // Clear the screen
  displayTitle();
  // yield();
  
  displayCoin(prices, changePercentages); // MUST be after displayTitle...
  
}

#endif