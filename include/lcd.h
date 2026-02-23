#ifndef LCD_H
#define LCD_H

#include <stdint.h>

// RS
#define LCD_COMMAND 0
#define LCD_DATA    1

// COMMANDS
#define LCD_CLEAR_DISPLAY 0x01
#define LCD_RETURN_HOME   0x02
#define LCD_MODE_4_BIT    0x02
#define LCD_WAKE_UP       0x03
#define LCD_ENTRY_MODE    0x06
#define LCD_DISPLAY_ON    0x0C
#define LCD_FUNCTION_SET  0x28

// SET CURSOR
#define  LCD_LINE_1   0x80
#define  LCD_LINE_2   0xC0
#define  LCD_COLUMN_1 0x00

// SPECIAL CHARACTERS
#define LCD_DEGREE_SYMBOL 0xDF

// PUBLIC FUNCTIONS
void lcd_initialization (void);
void lcd_send_byte (uint8_t data, uint8_t rs_mode);
void lcd_set_cursor (uint8_t row, uint8_t column);
void lcd_print (const char *string);

#endif