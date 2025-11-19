#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

#include <stdint.h>

// Display constants
#define OLED_I2C_ADDRESS 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Control bytes
#define CONTROL_BYTE_COMMAND 0x00
#define CONTROL_BYTE_DATA_STREAM 0x40

// SSD1306 commands
#define DISPLAY_OFF 0xAE
#define DISPLAY_ON 0xAF
#define SET_DISPLAY_CLOCK_DIV 0xD5
#define SET_MULTIPLEX_RATIO 0xA8
#define SET_DISPLAY_OFFSET 0xD3
#define SET_START_LINE 0x40
#define CHARGE_PUMP_SETTING 0x8D
#define SET_MEMORY_MODE 0x20
#define SEGMENT_REMAP 0xA0
#define COM_SCAN_DEC 0xC8
#define SET_COM_PINS 0xDA
#define SET_CONTRAST 0x81
#define SET_PRECHARGE_PERIOD 0xD9
#define SET_VCOM_DESELECT 0xDB
#define DISPLAY_ALL_ON_RESUME 0xA4
#define NORMAL_DISPLAY 0xA6
#define SET_COLUMN_ADDRESS 0x21
#define SET_PAGE_ADDRESS 0x22

// Function prototypes
void oled_initialize(void);
void oled_reset(void);
void oled_send_command(uint8_t cmd);
void oled_clear_screen(void);
void oled_set_cursor_position(uint8_t column, uint8_t page);
void oled_print_text(uint8_t x_pos, uint8_t y_pos, const char *text_string);
void oled_print_text_wrapped(uint8_t x_pos, uint8_t y_pos, const char *text_string);
void oled_print_number(uint8_t x_pos, uint8_t y_pos, uint32_t value, uint8_t center_flag);
uint8_t oled_count_digits(uint32_t number);
void oled_convert_uint_to_string(uint32_t value, char *output_string);
void oled_reverse_string(char *string);

#endif // OLED_DISPLAY_H