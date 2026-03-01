#ifndef TEMP_SENSOR_H
#define TEMP_SENSOR_H

#include <stdint.h>

// --- ERROR VALUE ---

#define TEMP_SENSOR_ERROR INT16_MIN



// --- DS18B20 PROTOCOL COMMANDS ---

#define TEMP_SKIP_ROM        0xCC
#define TEMP_CONVERT_T       0X44
#define TEMP_READ_SCRATCHPAD 0xBE



// --- PUBLIC FUNCTIONS ---

/*
   Sends a reset pulse and checks for the sensor presence pulse.
   return 1 if sensor is detected, 0 otherwise.
*/
uint8_t sensor_reset (void);



/*
   Executes a full measurement cycle: Reset -> Skip ROM -> Convert T -> Delay ->
   Reset -> Skip ROM -> Read Scratchpad.
   return The 16-bit raw signed temperature value.
*/
int16_t get_raw_temperature (void);



/*
   Converts the raw 16-bit data into a Celsius float.
   The input is a 12-bit fixed-point value in two's complement:
   - Bits 15-11: Sign (5 bits)
   - Bits 10-4:  Integer part (7 bits)
   - Bits 3-0:   Fractional part (4 bits)
   Multiplying by 0.0625 (1/16) handles both parts and the sign.
*/
float convert_to_celsius (int16_t raw_temperature);



// Converts from Celsius to Fahrenheit
float convert_to_fahrenheit (float temp_celsius);




// Converts from Celsius to Kelvin
float convert_to_kelvin (float temp_celsius);

#endif