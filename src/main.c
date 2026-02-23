#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdlib.h>
#include "lcd.h"
#include "pin_definitions.h"
#include "fsm_states.h"
#include "temp_sensor.h"
#include "lcd.h"
#include "button.h"



void setup (void)
{
    // --- GPIO CONFIGURATION ---

    DDRD &= ~(1 << TEMP_SENSOR); // TEMP_SENSOR (input). Set as input initially, OneWire protocol will toggle this

    DDRD &= ~(1 << BUTTON); // BUTTON (input)

    DDRB |= (1 << LCD_DB4) | (1 << LCD_DB5) | (1 << LCD_DB6) | (1 << LCD_DB7); // LCD Data Pins (output)

    DDRB |= (1 << LCD_RS) | (1 << LCD_E); // LCD Control Pins (output)

    
    
    // --- EXTERNAL INTERRUPT CONFIGURATION  (BUTTON) ---

    EIMSK |= (1 << INT0);                 // Enable external interrupt INT0
    EICRA |= (1 << ISC01) | (1 << ISC00); // Trigger INT0 on rising edge
    
    

    // --- TIMER0 CONFIGURATION (millis) ---
    
    TCCR0A |= (1 << WGM01);              // CTC mode
    TCCR0B |= (1 << CS01) | (1 << CS00); // Prescaler 64
    OCR0A = 249;                         // Comparison value (Formula: 16 MHz / 64 / 1000 Hz - 1)
    TIMSK0 |= (1 << OCIE0A);             // Enable interrupt for A comparison



    // --- ENABLE GLOBAL INTERRUPTS ---
    
    sei ();
}



int main (void)
{
    setup ();
    lcd_initialization();

    while (1)
    {
        int16_t raw_temperature = get_raw_temperature ();
        float temp_celsius = convert_to_celsius (raw_temperature);
        float temp_fahrenheit = convert_to_fahrenheit (temp_celsius);
        float temp_kelvin = convert_to_kelvin (temp_celsius);
        char temp_string[10];

        lcd_set_cursor (0, 0);
        lcd_print ("Temp: ");

        switch (unit_measure) 
        {
            case CELSIUS:
            dtostrf (temp_celsius, 4, 2, temp_string); 
            lcd_print (temp_string);
            lcd_send_byte (LCD_DEGREE_SYMBOL, LCD_DATA); // Print degree symbol '°'
            lcd_print ("C   ");
            break;

            case FAHRENHEIT:
            dtostrf (temp_fahrenheit, 4, 2, temp_string); 
            lcd_print (temp_string);
            lcd_send_byte (LCD_DEGREE_SYMBOL, LCD_DATA); // Print degree symbol '°'
            lcd_print ("F   ");
            break;

            case KELVIN:
            dtostrf (temp_kelvin, 4, 2, temp_string); 
            lcd_print (temp_string);
            lcd_print (" K  ");
            break;

            default:
            lcd_print ("ERROR");
            break;
        }
    }
}