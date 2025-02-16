#ifndef buttons_h
#define buttons_h



// =============== BUTTON SETTINGS ===============
#define BUTTON1_PIN  0
#define BUTTON2_PIN  2

EasyButton button1(BUTTON1_PIN);    // Change between screens
EasyButton button2(BUTTON2_PIN);    // move cursor
bool button1Update = false;     
bool button2Update = false;


// ====================BUTTON FUNCTIONS=============================

void B1_Callback_OnPressed() {
    Serial.println("Button 1 pressed! Toggling screen index.");
    button1Update = true;
    // screenIndex = (screenIndex + 1) % 2;
    // updateDisplay();

    // yield();
}
void B1_Callback_OnLongPress() {
    Serial.println("Button1 Long Press!");
    button1Update = true;
    // updateDisplay();
    // yield();
}
void B2_Callback_OnPressed() {
    Serial.print("Button 2 pressed!");
    button2Update = true;
    // if (screenIndex == 0) {
    //     Serial.println(" Cycling.");
    //     selectedCoinIndex = (selectedCoinIndex + 1) % numCoins;
    //     // cursorButtonFlag = true;
    // }
    
    // updateDisplay();
    // yield();
}
void button1ISR(){
  button1.read();
}
void button2ISR(){
  button2.read();
}

void buttonsInit() {
    button1.begin();
    button2.begin();

    button1.onPressed(B1_Callback_OnPressed);
    button2.onPressed(B2_Callback_OnPressed);
    button1.onPressedFor(2000, B1_Callback_OnLongPress);

    if (button1.supportsInterrupt()){
        button1.enableInterrupt(button1ISR);
        Serial.println("Button 1 will be used through interrupts");
      }
    if (button2.supportsInterrupt()){
        button2.enableInterrupt(button2ISR);
        Serial.println("Button 2 will be used through interrupts");
      }

    Serial.println("Buttons initialized!");
}




#endif