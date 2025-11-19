/*
 * oled_display.c
 * SSD1306 OLED Display Driver for MSP430
 */

#include "oled_display.h"
#include <msp430.h>
#include <stdint.h>
#include "simple_font_5x7.h"
#include "i2c_master.h"

// Display buffer for I2C transactions
static uint8_t display_buffer[17] = {0};

/* ====================================================================
 * Horizontal Centering Offsets
 * ==================================================================== */
static const uint8_t horizontal_center_offsets[] = {
    0,    // 0 digits (unused)
    61,   // 1 digit
    58,   // 2 digits
    55,   // 3 digits
    49,   // 4 digits + 1 separator
    46,   // 5 digits + 1 separator
    43,   // 6 digits + 1 separator
    37,   // 7 digits + 2 separators
    34,   // 8 digits + 2 separators
    31,   // 9 digits + 2 separators
    25    // 10 digits + 3 separators
};

// Initialize OLED display
void oled_initialize(void) {
    // Display initialization sequence
    oled_send_command(DISPLAY_OFF);                    // 0xAE
    oled_send_command(SET_DISPLAY_CLOCK_DIV);          // 0xD5
    oled_send_command(0x80);                           // Suggested clock ratio

    oled_send_command(SET_MULTIPLEX_RATIO);            // 0xA8
    oled_send_command(SCREEN_HEIGHT - 1);

    oled_send_command(SET_DISPLAY_OFFSET);             // 0xD3
    oled_send_command(0x00);                           // No offset
    oled_send_command(SET_START_LINE | 0x00);          // Start at line 0
    
    oled_send_command(CHARGE_PUMP_SETTING);            // 0x8D
    oled_send_command(0x14);                           // Internal VCC generation
    
    oled_send_command(SET_MEMORY_MODE);                // 0x20
    oled_send_command(0x00);                           // Horizontal addressing
    
    oled_send_command(SEGMENT_REMAP | 0x01);           // Flip horizontally
    oled_send_command(COM_SCAN_DEC);                   // Reverse scan direction

    oled_send_command(SET_COM_PINS);                   // 0xDA
    oled_send_command(0x12);
    
    oled_send_command(SET_CONTRAST);                   // 0x81
    oled_send_command(0xCF);

    oled_send_command(SET_PRECHARGE_PERIOD);           // 0xD9
    oled_send_command(0xF1);
    
    oled_send_command(SET_VCOM_DESELECT);              // 0xDB
    oled_send_command(0x40);
    
    oled_send_command(DISPLAY_ALL_ON_RESUME);          // 0xA4
    oled_send_command(NORMAL_DISPLAY);                 // 0xA6

    oled_send_command(DISPLAY_ON);                     // Turn on OLED panel
}

// Reset display to initial state
void oled_reset(void) {
    oled_send_command(DISPLAY_OFF);
    oled_clear_screen();
    oled_send_command(DISPLAY_ON);
}

// Send command to display controller
void oled_send_command(uint8_t cmd) {
    display_buffer[0] = CONTROL_BYTE_COMMAND;
    display_buffer[1] = cmd;
    i2c_send_data(OLED_I2C_ADDRESS, display_buffer, 2);
}

// Clear entire display
void oled_clear_screen(void) {
    oled_set_cursor_position(0, 0);
    
    for (uint8_t page = 0; page < 8; page++) {
        for (uint8_t col_group = 0; col_group < 16; col_group++) {
            // Prepare data packet
            for (uint8_t i = 0; i < 16; i++) {
                display_buffer[i] = (i == 0) ? CONTROL_BYTE_DATA_STREAM : 0x00;
            }
            display_buffer[16] = 0x00;
            
            i2c_send_data(OLED_I2C_ADDRESS, display_buffer, 17);
        }
    }
}

// Set cursor position
void oled_set_cursor_position(uint8_t column, uint8_t page) {
    if (column >= SCREEN_WIDTH) column = 0;
    if (page >= 8) page = 0;

    oled_send_command(SET_COLUMN_ADDRESS);
    oled_send_command(column);
    oled_send_command(SCREEN_WIDTH - 1);

    oled_send_command(SET_PAGE_ADDRESS);
    oled_send_command(page);
    oled_send_command(7); // End at page 7
}

// Print text string at specified position
void oled_print_text(uint8_t x_pos, uint8_t y_pos, const char *text_string) {
    oled_set_cursor_position(x_pos, y_pos);
    
    while (*text_string != '\0') {
        // Handle text wrapping
        if ((x_pos + 6) >= SCREEN_WIDTH) {
            x_pos = 0;
            y_pos++;
            oled_set_cursor_position(x_pos, y_pos);
        }

        // Prepare character data
        display_buffer[0] = CONTROL_BYTE_DATA_STREAM;
        
        uint8_t char_index = *text_string - ' ';
        for (uint8_t i = 0; i < 5; i++) {
            display_buffer[i + 1] = simple_font_5x7[char_index][i];
        }
        display_buffer[6] = 0x00;

        // Send character data
        uint8_t start_byte = CONTROL_BYTE_DATA_STREAM;
        i2c_send_data(OLED_I2C_ADDRESS, &start_byte, 1);
        i2c_send_data(OLED_I2C_ADDRESS, display_buffer, 7);
        
        text_string++;
        x_pos += 6;
    }
}

// Print text with word wrapping
void oled_print_text_wrapped(uint8_t x_pos, uint8_t y_pos, const char *text_string) {
    char current_word[16];
    uint8_t end_position = x_pos;

    while (*text_string != '\0') {
        uint8_t char_count = 0;
        
        // Extract next word
        while ((*text_string != ' ') && (*text_string != '\0')) {
            current_word[char_count++] = *text_string++;
        }
        current_word[char_count] = '\0';

        end_position += char_count * 6;

        // Handle line wrapping
        if (end_position >= SCREEN_WIDTH) {
            x_pos = 0;
            y_pos++;
            oled_print_text(x_pos, y_pos, current_word);
            end_position = char_count * 6;
            x_pos = end_position;
        } else {
            oled_print_text(x_pos, y_pos, current_word);
            end_position += 6; // Space after word
            x_pos = end_position;
        }
        
        if (*text_string == ' ') text_string++;
    }
}

// Print unsigned 32-bit integer with optional centering
void oled_print_number(uint8_t x_pos, uint8_t y_pos, uint32_t value, uint8_t center_flag) {
    char number_text[14];
    oled_convert_uint_to_string(value, number_text);
    
    if (center_flag) {
        uint8_t digit_count = oled_count_digits(value);
        oled_print_text(horizontal_center_offsets[digit_count], y_pos, number_text);
    } else {
        oled_print_text(x_pos, y_pos, number_text);
    }
}

// Count digits in number
uint8_t oled_count_digits(uint32_t number) {
    if (number < 10) return 1;
    if (number < 100) return 2;
    if (number < 1000) return 3;
    if (number < 10000) return 4;
    if (number < 100000) return 5;
    if (number < 1000000) return 6;
    if (number < 10000000) return 7;
    if (number < 100000000) return 8;
    if (number < 1000000000) return 9;
    return 10;
}

// Convert unsigned integer to string with thousand separators
void oled_convert_uint_to_string(uint32_t value, char *output_string) {
    uint8_t position = 0;
    uint8_t separator_counter = 0;

    // Build string in reverse
    do {
        if (separator_counter == 3) {
            output_string[position++] = ',';
            separator_counter = 0;
        }
        output_string[position++] = (value % 10) + '0';
        separator_counter++;
    } while ((value /= 10) > 0);

    output_string[position] = '\0';
    oled_reverse_string(output_string);
}

// Reverse string in place
void oled_reverse_string(char *string) {
    uint8_t length = 0;
    while (string[length] != '\0') length++;
    
    for (uint8_t i = 0, j = length - 1; i < j; i++, j--) {
        uint8_t temp = string[i];
        string[i] = string[j];
        string[j] = temp;
    }
}