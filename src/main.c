#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdlib.h>
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
        float temp_celsius = 0.0;
        float temp_fahrenheit = 0.0;
        float temp_kelvin = 0.0;
        char temp_string[10]; // In this string is saved the temperature value to avoid further conversions

        // Show ERROR warning incase of sensor's disconnection or malfunction (sensor not detected)
        if (raw_temperature == TEMP_SENSOR_ERROR) 
        {
            unit_measure = SENSOR_ERROR;
        }

        // Restore CELSIUS state when the sensor is detected and calculate temperature
        else 
        {
            if (unit_measure == SENSOR_ERROR)
            {
                lcd_send_byte (LCD_CLEAR_DISPLAY, LCD_COMMAND);
                _delay_ms(2);
                unit_measure = CELSIUS;
            }

            temp_celsius = convert_to_celsius (raw_temperature);
            temp_fahrenheit = convert_to_fahrenheit (temp_celsius);
            temp_kelvin = convert_to_kelvin (temp_celsius);
        }

        switch (unit_measure) 
        {
            case CELSIUS:
            lcd_set_cursor (0, 0);
            lcd_print ("Temp: ");
            dtostrf (temp_celsius, 4, 2, temp_string); 
            lcd_print (temp_string);
            lcd_send_byte (LCD_DEGREE_SYMBOL, LCD_DATA); // Print degree symbol '°'
            lcd_print ("C   ");
            break;

            case FAHRENHEIT:
            lcd_set_cursor (0, 0);
            lcd_print ("Temp: ");
            dtostrf (temp_fahrenheit, 4, 2, temp_string); 
            lcd_print (temp_string);
            lcd_send_byte (LCD_DEGREE_SYMBOL, LCD_DATA); // Print degree symbol '°'
            lcd_print ("F   ");
            break;

            case KELVIN:
            lcd_set_cursor (0, 0);
            lcd_print ("Temp: ");
            dtostrf (temp_kelvin, 4, 2, temp_string); 
            lcd_print (temp_string);
            lcd_print (" K  ");
            break;

            case SENSOR_ERROR:
            lcd_set_cursor (0, 0);
            lcd_print ("ERROR          ");
            lcd_set_cursor (1, 0);
            lcd_print ("TEMP SENSOR    ");
            break;
        }
    }
}