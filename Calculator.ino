
#include <Arduino.h>
#include <U8g2lib.h>
#include <Keypad.h>
#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

#define MAX_BUFFER_SIZE 14
#define MAX_INTEGER_DIGIT 15
#define CURSOR_BLINK_INTERVAL_MILLI_SEC 1000
enum class Direction{
  NONE,
  UP,
  DOWN,
  LEFT,
  RIGHT,
};

//U8G2_ST7920_128X64_1_SW_SPI u8g2(U8G2_R0, /* clock=*/ 13, /* data=*/ 11, /* CS=*/ 10, /* reset=*/ 8); // for arduino uno

U8G2_ST7920_128X64_1_SW_SPI u8g2(U8G2_R0, /* clock=*/ 18, /* data=*/ 19, /* CS=*/ 17, /* reset=*/ 8); // for pi pico
const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
//define the cymbols on the buttons of the keypads
char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3', 'x'},
  {'4', '5', '6', '/'},
  {'7', '8', '9', '+'},
  {'0', '.', 'e', '-'}
};
byte rowPins[ROWS] = {2, 3, 4, 5}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {6, 7, 8, 9}; //connect to the column pinouts of the keypad

Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);
char str[8][25];
int cur_x = 0;
int cur_y = 0;
char buffer[MAX_BUFFER_SIZE + 1];
char temp_char = ' ';
int cur = 0;
unsigned long previous_time = 0;
boolean cursor_state = false;
boolean arrow_lock = false;

void print_error() {
  u8g2.clearDisplay();
  u8g2.drawStr(2, 50, "Error");
}

long long chars_to_int(const char *s, int length) {
  long long result = 0;
  for (int i = 0; i < length; i++) {
    result *= 10;
    result += (s[i] - '0');
  }
  return result;
}

void u8g2_prepare(void) {
  u8g2.setFontRefHeightExtendedText();
  u8g2.setDrawColor(1);
  u8g2.setFontPosTop();
  u8g2.setFontDirection(0);
}

void draw_result(int y, const char *str, int length) {
  u8g2.setFont(u8g2_font_5x7_tf);
  auto display_width = u8g2.getDisplayWidth();
  auto char_width = u8g2.getMaxCharWidth();
  auto char_height = u8g2.getMaxCharHeight();
  auto char_count = display_width / char_width;
  int index = 0;
  u8g2.drawStr(0, y * char_height, str);
}

void draw_input(long long input) {

  int count = 0;
  int n = input;
  while (n != 0) {
    n = n / 10;
    ++count;
  }
  if (count >= MAX_BUFFER_SIZE) {
    print_error();
    return;
  }

  u8g2.setFont(u8g2_font_7x14B_tf);
  u8g2.setCursor(2, 50);

  char buf[MAX_BUFFER_SIZE];
  if (input / 1000000L != 0) {
    sprintf(buf, "%0ld", input / 1000000L);
    u8g2.print(buf);
    Serial.print(buf);
  }
  sprintf(buf, "%0ld", input % 1000000L);
  u8g2.print(buf);
  Serial.println(buf);

}

void blink_state(long interval_milli_sec){
  auto current_time = millis();
  if (current_time - previous_time >= interval_milli_sec){
    previous_time = current_time;
    cursor_state =! cursor_state;
  }
}

Direction get_direction(){
  int pin = analogRead(A0);
  if (pin < 100){
      Serial.println(0);
      arrow_lock = false;
      return Direction::NONE;
  }
  else if (pin < 200 && arrow_lock == false){
    Serial.println("left");
    arrow_lock = true;
    return Direction::LEFT;
  }
  else if (pin < 300 && arrow_lock == false){
    Serial.println("down");
    return Direction::DOWN;
    arrow_lock = true;
  }
  else if (pin < 800 && arrow_lock == false){
    Serial.println("right");
    arrow_lock = true;
    return Direction::RIGHT;
  }
  else if (arrow_lock == false) {
    Serial.println("up");
    arrow_lock = true;
    return Direction::UP;
  }
  else {
    return Direction::NONE;
  }
}

void setup() {
  Serial.begin(9600);
  u8g2.begin();
//  for (int i = 0; i < 4; i++){
//    pinMode(i + 2, OUTPUT);
//    pinMode(i + 6, INPUT);
//  }
}
void loop() {
  Serial.println(analogRead(26));
//  for (int i = 0; i < 4; i++){
//      digitalWrite(i + 2, HIGH);
//  }
//  Serial.print(digitalRead(2));
//  Serial.print(digitalRead(3));
//  Serial.print(digitalRead(4));
//  Serial.print(digitalRead(5));
//  Serial.print(" - ");
//  Serial.print(digitalRead(6));
//  Serial.print(digitalRead(7));
//  Serial.print(digitalRead(8));
//  Serial.println(digitalRead(9));
  //char customKey = customKeypad.getKey();
  //Serial.println(customKey);
  // if (customKey == '+' || customKey == '-' || customKey == '*' || customKey == '/'){
    
  // }
  // else if (customKey && cur < MAX_BUFFER_SIZE) {
  //   buffer[cur] = customKey;
  //   cur++;
  // }
  // switch (get_direction()){
  //   case Direction::LEFT:
  //     buffer[cur] = '\0';
  //     cur--;
  //     break;
  //   case Direction::RIGHT:
  //     cur++;
  //     break;
  //   default:
  //     break;
  // }
  u8g2.firstPage();
  do {
    u8g2_prepare();
    for (int y = 0; y <= cur_y; y++) {
      draw_result(y, str[y], cur_x);
    }
    blink_state(CURSOR_BLINK_INTERVAL_MILLI_SEC);
    if (cursor_state){
      if (buffer[cur] == '\0' || buffer[cur] == '_'){
        temp_char = ' ';
      } else {
        temp_char = buffer[cur];
      }
      buffer[cur] = '_';    
      digitalWrite(LED_BUILTIN, HIGH);
    } else {
      buffer[cur] = temp_char;
      digitalWrite(LED_BUILTIN, LOW);  
    }
    u8g2.setFont(u8g2_font_7x14B_tf);
    u8g2.setCursor(2, 50);
    u8g2.print(buffer);
    //Serial.println(buffer);
    //draw_input(chars_to_int(buffer, cur));
  } while ( u8g2.nextPage() );
}
