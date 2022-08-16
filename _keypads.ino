#include <Arduino.h>
#include "pico/stdlib.h"
#include "hardware/timer.h"

uint all_columns_mask = 0x0;
uint column_mask[4];

uint columns[4] = { 5, 4, 3, 2 };
uint rows[4] = { 9, 8, 7, 6 };
char num_matrix[16] = {
  '1', '2', '3', ' ',
  '4', '5', '6', ' ',
  '7', '8', '9', ' ',
  '.', '0', 's', ' ' // ., 0, string
};
bool shift = false;
char op_matrix1[12] = {
  '+', 'r', 'C', // +, root, C/CE
  '-', '^', 'S', // -, square, shift
  '*', '(', 'd', // *, (, delete
  '/', ')', 'L'  // /, ), log
};
char op_matrix2[12] = {
  'B', '!', '?', // binary, factorial, wrong
  'O', '%', '?', // octal, modulo, wrong
  'D', 'R', '?', // decimal, random, wrong
  'H', '#', '#'  // hexadecimal, temp, temp
};

bool num_lock = false;
bool op_lock = false;

void pico_keypad_init() {
  for (int i = 0; i < 4; i++) {

    gpio_init(columns[i]);
    gpio_init(rows[i]);

    gpio_set_dir(columns[i], GPIO_IN);
    gpio_set_dir(rows[i], GPIO_OUT);

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
    return num_matrix[row * 4 + 0];
  } else if (cols == column_mask[1] && num_lock == false) {
    num_lock = true;
    return num_matrix[row * 4 + 1];
  } else if (cols == column_mask[2] && num_lock == false) {
    num_lock = true;
    return num_matrix[row * 4 + 2];
  } else if (cols == column_mask[3] && num_lock == false) {
    num_lock = true;
    return num_matrix[row * 4 + 3];
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

char get_op_key_3x4(int analog_input) {
  if (analog_input > 1000 && op_lock == false) {
    op_lock = true;
    return op_matrix1[0];
  } else if (analog_input > 700 && op_lock == false) {
    op_lock = true;
    return op_matrix1[1];
  } else if (analog_input > 520 && op_lock == false) {
    op_lock = true;
    return op_matrix1[2];
  } else if (analog_input > 400 && op_lock == false) {
    op_lock = true;
    return op_matrix1[3];
  } else if (analog_input > 340 && op_lock == false) {
    op_lock = true;
    return op_matrix1[4];
  } else if (analog_input > 300 && op_lock == false) {
    op_lock = true;
    return op_matrix1[5];
  } else if (analog_input > 250 && op_lock == false) {
    op_lock = true;
    return op_matrix1[6];
  } else if (analog_input > 220 && op_lock == false) {
    op_lock = true;
    return op_matrix1[7];
  } else if (analog_input > 200 && op_lock == false) {
    op_lock = true;
    return op_matrix1[8];
  } else if (analog_input > 185 && op_lock == false) {
    op_lock = true;
    return op_matrix1[9];
  } else if (analog_input > 170 && op_lock == false) {
    op_lock = true;
    return op_matrix1[10];
  } else if (analog_input > 160 && op_lock == false) {
    op_lock = true;
    return op_matrix1[11];
  } else if (analog_input < 100){
    op_lock = false;
    return '\0';
  } else {
    return '\0';
  }
}