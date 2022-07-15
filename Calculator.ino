
#include <Arduino.h>
#include <U8g2lib.h>
#include <Keypad.h>
#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

U8G2_ST7920_128X64_1_SW_SPI u8g2(U8G2_R0, /* clock=*/ 13, /* data=*/ 11, /* CS=*/ 10, /* reset=*/ 8);
const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
//define the cymbols on the buttons of the keypads
char hexaKeys[ROWS][COLS] = {
  {'1','2','3','x'},
  {'4','5','6','/'},
  {'7','8','9','+'},
  {'0','.','e','-'}
};
byte rowPins[ROWS] = {2, 3, 4, 5}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {6, 7, 8, 9}; //connect to the column pinouts of the keypad

Keypad customKeypad = Keypad( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS); 

void u8g2_prepare(void) {
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setFontRefHeightExtendedText();
  u8g2.setDrawColor(1);
  u8g2.setFontPosTop();
  u8g2.setFontDirection(0);
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  u8g2.begin();
}

String str;
void loop() {
  // put your main code here, to run repeatedly:
  

  char customKey = customKeypad.getKey();
  if (customKey){
    Serial.println(customKey);
    str += customKey;
  }
  u8g2.firstPage();
  do {
    u8g2_prepare();
    u8g2.drawStr(0,0, str.c_str());
  } while ( u8g2.nextPage() );
}
