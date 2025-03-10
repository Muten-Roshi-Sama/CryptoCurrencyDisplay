// v1.0
#ifndef candleChartView_h
#define candleChartView_h

// =============== CANDLE CHART ===============
const unsigned long dataHistoricalUpdateInterval = 11000;
struct Candle {
    float open;
    float high;
    float low;
    float close;
    float volume;
};
Candle candles[100];  
const char* chart_interval = "1h";     
const int limit = 24;  
float hourlyPrices[24];  



//==================================Single_Crypto_View=========================================================

// HTTP
void fetchHistoricalData(int coinIndex) {
  const char* binanceEndpoint = "https://api.binance.com/api/v3/klines";
  // Build the URL for Binance API request
  String url = String(binanceEndpoint) + "?symbol=" + coins[coinIndex] + "&interval=" + chart_interval + "&limit=" + String(limit);
  HTTPClient http;
  http.begin(url);
  yield();
  int httpResponseCode = http.GET();
  if (httpResponseCode == HTTP_CODE_OK) {
    String payload = http.getString();
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, payload);
    if (!error) {
      // Extract kline data and store in candles array
      Serial.println("Fetched Crypto data from Binance.");
      for (int i = 0; i < limit; i++) {
        candles[i].open = doc[i][1].as<float>();  // Open price
        candles[i].high = doc[i][2].as<float>();  // High price
        candles[i].low = doc[i][3].as<float>();  // Low price
        candles[i].close = doc[i][4].as<float>();  // Close price
        candles[i].volume = doc[i][5].as<float>();  // Volume
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

// DISPLAY
void displaySingleCryptoInfo(int coinIndex, float prices[], float changePercentages[]) {
  int textSize = 3;
  int coinNameTS = textSize+1;
  // int sizeTwoTextHeight = 12;
  // Define drawing area
  int leftPadding = 4;
  int rightMargin = 2;
  int yPosition = STARTY + 2; // Start drawing within the area
  int verticalMargin = 5;  // Between name and price.
  singleCrypto_Title_Height = yPosition + 2*textWidth("A", tft, textSize) + verticalMargin ;

  // GET COIN INFO
  String coinName = getFormattedCoinName(coins[coinIndex]);
  float change = changePercentages[coinIndex];
  String changeText = (change >= 0 ? "+" : "") + String(change, 2) + "%";
  float price = prices[coinIndex];
  String priceText = "$" + String(price, 2);

  // Calculate positions for coin name, change percentage, and price
  int coinNameWidth = textWidth(coinName, tft, textSize);
  int changeTextWidth = textWidth(changeText, tft, textSize);
  int priceTextWidth = textWidth(priceText, tft, textSize);
  int coinNameX = STARTX + leftPadding;
  int coinNameY = yPosition + textHeight("A", tft, coinNameTS)*1/3;
  int changeX = WIDTH - (changeTextWidth + 2*rightMargin) ;
  int priceX = WIDTH - (priceTextWidth + 2*rightMargin);//STARTX + leftPadding; // ENDX - priceTextWidth - rightMargin;

  
  // CoinName and percentage change (size 2)
  tft.setTextSize(coinNameTS);
  tft.setCursor(coinNameX, coinNameY);
  tft.setTextColor(ILI9341_WHITE);
  tft.print(coinName);
  tft.setTextSize(textSize);

  // % % %
  tft.setCursor(changeX, yPosition  + textHeight("A", tft, textSize) + verticalMargin);
  tft.setTextColor(change >= 0 ? ILI9341_GREEN : ILI9341_RED);
  tft.print(changeText);

  // PRICE
  tft.setCursor(priceX, yPosition);
  tft.setTextColor(ILI9341_WHITE);
  tft.print(priceText);

  int yLine = singleCrypto_Title_Height + verticalMargin;
  // tft.drawLine(0, yLine, WIDTH, yLine, ILI9341_WHITE);


}
void drawCandle(int x, const Candle &candle, int yStart, int yEnd, float minPrice, float maxPrice) {
  int availableHeight = yEnd - yStart; // Total height for the chart
  int candleWidth = 8;  // Width of the candlestick body
  int wickWidth = candleWidth/4; //2;    // Width of the wick
  
  // Scale prices to pixel positions
  int yHigh = yStart + (maxPrice - candle.high) * availableHeight / (maxPrice - minPrice);
  int yLow = yStart + (maxPrice - candle.low) * availableHeight / (maxPrice - minPrice);
  int yOpen = yStart + (maxPrice - candle.open) * availableHeight / (maxPrice - minPrice);
  int yClose = yStart + (maxPrice - candle.close) * availableHeight / (maxPrice - minPrice);
  // Ensure the body is always drawn top to bottom
  int yBodyTop = min(yOpen, yClose);
  int yBodyBottom = max(yOpen, yClose);
  // Color for up/down candles
  uint32_t bodyColor = (candle.close >= candle.open) ? ILI9341_GREEN : ILI9341_RED;
  // Draw the wick
  tft.fillRect(x + (candleWidth - wickWidth) / 2, yHigh, wickWidth, yLow - yHigh, bodyColor);
  // Draw the candle body
  tft.fillRect(x, yBodyTop, candleWidth, yBodyBottom - yBodyTop, bodyColor);
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
    tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    tft.print(String(price, 2)); // Print the price with 2 decimal points
    // Optional: Draw horizontal grid line
    tft.drawLine(30, yPosition, tft.width(), yPosition, ILI9341_GREY);
  }
}
void displayCryptoChart() {
  int x = STARTX;
  int yStart = singleCrypto_Title_Height + 20; //STARTY + verticalMargin + 2*tft.fontHeight() + 1;   //see displaySingleCryptoInfo()
  int yEnd = HEIGHT -2;  // padding
  
  // Find the min and max prices from the candles
  float minPrice = candles[0].low;
  float maxPrice = candles[0].high;
  for (int i = 1; i < limit; i++) {
    if (candles[i].low < minPrice) minPrice = candles[i].low;
    if (candles[i].high > maxPrice) maxPrice = candles[i].high;
  }

  //Scale - work in progress...
  // displayCryptoScale(minPrice, maxPrice, yStart, yEnd);
  
  // Draw each candle
  for (int i = 0; i < limit; i++) {
    drawCandle(x, candles[i], yStart, yEnd, minPrice, maxPrice);
    x += 12; // Increment x position for the next candle
  }
}




// full call
void singleCryptoView() {
  // AREA :
  //      - displaySingleCryptoInfo  :  (0,0) to (xmax, 20)
  //      - displayCryptoChart  :  (0,20) to (xmax, 100)
  //
  tft.fillScreen(ILI9341_BLACK);  // Clear
  displaySingleCryptoInfo(selectedCoinIndex, prices, changePercentages);
  yield();

  // UPDATE crypto candles array
  if(selectedCoinIndex != lastSelectedCoinIndex) {
      fetchHistoricalData(selectedCoinIndex);
      lastSelectedCoinIndex = selectedCoinIndex;
    }
  //chart
  displayCryptoChart();
  // debugPrintColorfulPixels();
}

#endif