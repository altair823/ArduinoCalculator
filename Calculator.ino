#include <Arduino.h>
#include <U8g2lib.h>
#include "pico/stdlib.h"
#include "hardware/timer.h"
#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

#define GPIO_INPUT false
#define GPIO_OUTPUT true
#define MAX_BUFFER_SIZE 14
#define MAX_INTEGER_DIGIT 15
#define CURSOR_BLINK_INTERVAL_MILLI_SEC 1000

//U8G2_ST7920_128X64_1_SW_SPI u8g2(U8G2_R0, /* clock=*/ 13, /* data=*/ 11, /* CS=*/ 10, /* reset=*/ 8); // for arduino uno

U8G2_ST7920_128X64_1_SW_SPI u8g2(U8G2_R0, /* clock=*/18, /* data=*/19, /* CS=*/17, /* reset=*/8);  // for pi pico


uint all_columns_mask = 0x0;
uint column_mask[4];

uint columns[4] = { 5, 4, 3, 2 };
uint rows[4] = { 9, 8, 7, 6 };
char matrix[16] = {
  '1', '2', '3', 'A',
  '4', '5', '6', 'B',
  '7', '8', '9', 'C',
  '.', '0', '#', 'D'
};

char str[8][25];
int cur_x = 0;
int cur_y = 0;
char buffer[MAX_BUFFER_SIZE + 1];
char temp_char = ' ';
int cur = 0;
unsigned long previous_time = 0;
bool cursor_state = false;
bool num_lock = false;
bool op_lock = false;
bool dir_lock = false;

enum class Direction {
  NONE,
  UP,
  DOWN,
  LEFT,
  RIGHT,
};
void pico_keypad_init() {
  for (int i = 0; i < 4; i++) {

    gpio_init(columns[i]);
    gpio_init(rows[i]);

    gpio_set_dir(columns[i], GPIO_INPUT);
    gpio_set_dir(rows[i], GPIO_OUTPUT);

    digitalWrite(rows[i], HIGH);

    all_columns_mask = all_columns_mask + (1 << columns[i]);
    column_mask[i] = 1 << columns[i];
  }
}

/**
 * @brief Scan and get the pressed key.
 *
 * This routine returns the first key found to be pressed
 * during the scan.
 */
char pico_keypad_get_key(void) {
  int row;
  uint32_t cols;

  cols = gpio_get_all();
  cols = cols & all_columns_mask;

  if (cols == 0x0) {
    num_lock = false;
    return 0;
  }

  for (int j = 0; j < 4; j++) {
    digitalWrite(rows[j], LOW);
  }

  for (row = 0; row < 4; row++) {

    digitalWrite(rows[row], HIGH);

    cols = gpio_get_all();
    digitalWrite(rows[row], LOW);
    cols = cols & all_columns_mask;
    if (cols != 0x0) {
      break;
    }
  }

  for (int i = 0; i < 4; i++) {
    digitalWrite(rows[i], HIGH);
  }

  if (cols == column_mask[0] && num_lock == false) {
    num_lock = true;
    return matrix[row * 4 + 0];
  } else if (cols == column_mask[1] && num_lock == false) {
    num_lock = true;
    return matrix[row * 4 + 1];
  } else if (cols == column_mask[2] && num_lock == false) {
    num_lock = true;
    return matrix[row * 4 + 2];
  } else if (cols == column_mask[3] && num_lock == false) {
    num_lock = true;
    return matrix[row * 4 + 3];
  } else {
    return 0;
  }
}

/**
 * @brief Setup keypad to work with IRQ.
 *
 * Make keypad pins work as processor interruption
 *
 * @param enable Enable or disable interruption pins
 * @param callback Function of what will happen if the key is pressed
 */
void pico_keypad_irq_enable(bool enable, gpio_irq_callback_t callback) {
  for (int i = 0; i < 4; i++) {
    gpio_set_irq_enabled_with_callback(columns[i], 0x8, enable, callback);
  }
}


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

void blink_state(long interval_milli_sec) {
  auto current_time = millis();
  if (current_time - previous_time >= interval_milli_sec) {
    previous_time = current_time;
    cursor_state = !cursor_state;
  }
}

Direction get_direction() {
  int pin = analogRead(A0);
  if (pin < 100) {
    Serial.println(0);
    dir_lock = false;
    return Direction::NONE;
  } else if (pin < 200 && dir_lock == false) {
    Serial.println("left");
    dir_lock = true;
    return Direction::LEFT;
  } else if (pin < 300 && dir_lock == false) {
    Serial.println("down");
    return Direction::DOWN;
    dir_lock = true;
  } else if (pin < 800 && dir_lock == false) {
    Serial.println("right");
    dir_lock = true;
    return Direction::RIGHT;
  } else if (dir_lock == false) {
    Serial.println("up");
    dir_lock = true;
    return Direction::UP;
  } else {
    return Direction::NONE;
  }
}

char get_op_key_3x4(int analog_input) {
  if (analog_input > 1000 && op_lock == false) {
    op_lock = true;
    return 'a';
  } else if (analog_input > 700 && op_lock == false) {
    op_lock = true;
    return 'b';
  } else if (analog_input > 540 && op_lock == false) {
    op_lock = true;
    return 'C';
  } else if (analog_input > 400 && op_lock == false) {
    op_lock = true;
    return 'd';
  } else if (analog_input > 340 && op_lock == false) {
    op_lock = true;
    return 'e';
  } else if (analog_input > 300 && op_lock == false) {
    op_lock = true;
    return 'f';
  } else if (analog_input > 250 && op_lock == false) {
    op_lock = true;
    return 'g';
  } else if (analog_input > 220 && op_lock == false) {
    op_lock = true;
    return 'h';
  } else if (analog_input > 200 && op_lock == false) {
    op_lock = true;
    return 'i';
  } else if (analog_input > 185 && op_lock == false) {
    op_lock = true;
    return 'j';
  } else if (analog_input > 170 && op_lock == false) {
    op_lock = true;
    return 'k';
  } else if (analog_input > 160 && op_lock == false) {
    op_lock = true;
    return 'l';
  } else {
    op_lock = false;
    return '\0';
  }
}
void setup() {
  Serial.begin(9600);
  u8g2.begin();
  pico_keypad_init();
}

void clear_buffer(){
  for (int i = 0; i < MAX_BUFFER_SIZE; i++){
    buffer[i] = '\0';    
  }  
  cur = 0;
  temp_char = ' ';
}

void loop() {
  char key = pico_keypad_get_key();
  char op = get_op_key_3x4(analogRead(26));
  Serial.print(op);
  Serial.print(" - ");
  Serial.println(key);
  if (op == 'C'){
    clear_buffer();
  }
  if (key == '+' || key == '-' || key == '*' || key == '/') {

  } else if (key && cur < MAX_BUFFER_SIZE) {
    buffer[cur] = key;
    cur++;
  }
  //Serial.println(analogRead(27));
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
    if (cursor_state) {
      if (buffer[cur] == '\0' || buffer[cur] == '_') {
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
  } while (u8g2.nextPage());
}
