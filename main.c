#include <avr/io.h>
#include <util/delay.h>

// Pin definitions
#define LCD_DB7     PD2
#define LCD_DB6     PD3
#define LCD_DB5     PD4
#define LCD_DB4     PD5
#define LCD_E       PB3
#define LCD_RS      PB4
#define BUTTON      PB6
#define TEMP_SENSOR PB7

typedef enum {
    CELSIUS,
    FAHRENHEIT,
    KELVIN
} TempScale;

int main (void)
{
    // --- GPIO configuration ---
    
    // Set BUTTON as input
    DDRB &= ~(1 << BUTTON);

    // LCD Data Pins
    DDRD |= (1 << LCD_DB4) | (1 << LCD_DB5) | (1 << LCD_DB6) | (1 << LCD_DB7);

    // LCD Control Pins
    DDRB |= (1 << LCD_E) | (1 << LCD_RS);

    
    while (1)
    {

    }
}