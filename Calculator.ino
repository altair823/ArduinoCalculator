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

uint _columns[4];
uint _rows[4];
char _matrix_values[16];

uint all_columns_mask = 0x0;
uint column_mask[4];

uint columns[4] = { 5, 4, 3, 2 };
uint rows[4] = { 9, 8, 7, 6 };
char matrix[16] = {
  '1', '2', '3', 'A',
  '4', '5', '6', 'B',
  '7', '8', '9', 'C',
  '*', '0', '#', 'D'
};

#define MAX_BUFFER_SIZE 14
#define MAX_INTEGER_DIGIT 15
#define CURSOR_BLINK_INTERVAL_MILLI_SEC 1000
enum class Direction {
  NONE,
  UP,
  DOWN,
  LEFT,
  RIGHT,
};
void pico_keypad_init(uint columns[4], uint rows[4], char matrix_values[16]) {

  for (int i = 0; i < 16; i++) {
    _matrix_values[i] = matrix_values[i];
  }

  for (int i = 0; i < 4; i++) {

    _columns[i] = columns[i];
    _rows[i] = rows[i];

    gpio_init(_columns[i]);
    gpio_init(_rows[i]);

    gpio_set_dir(_columns[i], GPIO_INPUT);
    gpio_set_dir(_rows[i], GPIO_OUTPUT);

    gpio_put(_rows[i], 1);

    all_columns_mask = all_columns_mask + (1 << _columns[i]);
    column_mask[i] = 1 << _columns[i];
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
  bool pressed = false;

  cols = gpio_get_all();
  cols = cols & all_columns_mask;

  if (cols == 0x0) {
    return 0;
  }

  for (int j = 0; j < 4; j++) {
    gpio_put(_rows[j], 0);
  }

  for (row = 0; row < 4; row++) {

    gpio_put(_rows[row], 1);

    busy_wait_us(10000);

    cols = gpio_get_all();
    gpio_put(_rows[row], 0);
    cols = cols & all_columns_mask;
    if (cols != 0x0) {
      break;
    }
  }

  for (int i = 0; i < 4; i++) {
    gpio_put(_rows[i], 1);
  }

  if (cols == column_mask[0]) {
    return (char)_matrix_values[row * 4 + 0];
  } else if (cols == column_mask[1]) {
    return (char)_matrix_values[row * 4 + 1];
  }
  if (cols == column_mask[2]) {
    return (char)_matrix_values[row * 4 + 2];
  } else if (cols == column_mask[3]) {
    return (char)_matrix_values[row * 4 + 3];
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
    gpio_set_irq_enabled_with_callback(_columns[i], 0x8, enable, callback);
  }
}

//U8G2_ST7920_128X64_1_SW_SPI u8g2(U8G2_R0, /* clock=*/ 13, /* data=*/ 11, /* CS=*/ 10, /* reset=*/ 8); // for arduino uno

U8G2_ST7920_128X64_1_SW_SPI u8g2(U8G2_R0, /* clock=*/18, /* data=*/19, /* CS=*/17, /* reset=*/8);  // for pi pico

char str[8][25];
int cur_x = 0;
int cur_y = 0;
char buffer[MAX_BUFFER_SIZE + 1];
char temp_char = ' ';
int cur = 0;
unsigned long previous_time = 0;
boolean cursor_state = false;
boolean button_lock = false;

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
    button_lock = false;
    return Direction::NONE;
  } else if (pin < 200 && button_lock == false) {
    Serial.println("left");
    button_lock = true;
    return Direction::LEFT;
  } else if (pin < 300 && button_lock == false) {
    Serial.println("down");
    return Direction::DOWN;
    button_lock = true;
  } else if (pin < 800 && button_lock == false) {
    Serial.println("right");
    button_lock = true;
    return Direction::RIGHT;
  } else if (button_lock == false) {
    Serial.println("up");
    button_lock = true;
    return Direction::UP;
  } else {
    return Direction::NONE;
  }
}

char get_key_3x4(int analog_input) {
  if (analog_input > 1000 && button_lock == false) {
    button_lock = true;
    return '1';
  } else if (analog_input < 980 && analog_input > 900 && button_lock == false) {
    button_lock = true;
    return '2';
  } else if (analog_input < 760 && analog_input > 700 && button_lock == false) {
    button_lock = true;
    return '3';
  } else if (analog_input < 600 && analog_input > 560 && button_lock == false) {
    button_lock = true;
    return '4';
  } else if (analog_input < 500 && analog_input > 460 && button_lock == false) {
    button_lock = true;
    return '5';
  } else if (analog_input < 440 && analog_input > 390 && button_lock == false) {
    button_lock = true;
    return '6';
  } else if (analog_input < 380 && analog_input > 340 && button_lock == false) {
    button_lock = true;
    return '7';
  } else if (analog_input < 340 && analog_input > 310 && button_lock == false) {
    button_lock = true;
    return '8';
  } else if (analog_input < 310 && analog_input > 280 && button_lock == false) {
    button_lock = true;
    return '9';
  } else if (analog_input < 280 && analog_input > 260 && button_lock == false) {
    button_lock = true;
    return '.';
  } else if (analog_input < 260 && analog_input > 240 && button_lock == false) {
    button_lock = true;
    return '0';
  } else if (analog_input < 240 && analog_input > 200 && button_lock == false) {
    button_lock = true;
    return 'E';
  } else if (analog_input <= 200) {
    button_lock = false;
    return '\0';
  } else {
    return '\0';
  }
}
void setup() {
  Serial.begin(9600);
  u8g2.begin();
  pico_keypad_init(columns, rows, matrix);
}
void loop() {
  char key = pico_keypad_get_key();
  Serial.print(analogRead(26));
  Serial.print(" - ");
  Serial.println(pico_keypad_get_key());
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