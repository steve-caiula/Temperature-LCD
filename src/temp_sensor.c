#include <avr/io.h>
#include <stdint.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "temp_sensor.h"
#include "pin_definitions.h"
#include "mcu_timer.h"




uint8_t sensor_reset (void)
{
    uint8_t detected = 0;

    cli (); // Critical Section Start: Disable interrupts to protect 1-Wire timing

    // --- PHASE 1: MASTER TRANSMITS RESET PULSE ---

    DDRD |= (1 << TEMP_SENSOR);   // Set pin as output
    PORTD &= ~(1 << TEMP_SENSOR); // Pull bus LOW
    _delay_us(480);           // Wait time for reset 



    // --- PHASE 2: MASTER REALESES BUS AND WAITS FOR PRESENCE PULSE ---

    DDRD &= ~(1 << TEMP_SENSOR);  // Release bus (set as input)
    
    /* 
       Wait 70us to sample the bus.
       The datasheet states that the sensor waits 15-60us before pulling the bus LOW
       for 60-240us. Sampling at 70us ensures we catch both fast and slow sensors
       during their guaranteed presence pulse window.
    */
    _delay_us (70);



    // --- PHASE 3: SAMPLE THE BUS TO DETECT THE SENSOR ---

    if (!(PIND & (1 << TEMP_SENSOR))) 
    {
        detected = 1; // Sensor pulled the bus LOW (Presence Pulse found)
    }

    else 
    {
        detected = 0;
    }

    /* 
       Recovery time: Wait for the remainder of the 480us time slot.
       This ensures the sensor has finished its presence pulse (max 240us)
       and the bus has stabilized before the next communication starts.
    */
    _delay_us (410);  
    
    sei ();          // Critical Section End: Re-enable interrupts

    return detected; // Return 1 if the sensor is detected, 0 otherwise
}




static void sensor_write_bit (uint8_t bit_value)
{
    cli (); // Atomic Block Start: Prevent interrupt interference during write slot
    
    // Every write slot starts by pulling the bus LOW
    DDRD |= (1 << TEMP_SENSOR);    // Set pin as output
    PORTD &= ~(1 << TEMP_SENSOR);  // Pull bus LOW


    
    /* 
       Write 1 slot: The master pulls the bus LOW and then releases it 
       within 15us. The bus is pulled HIGH by the resistor for the 
       remainder of the slot. Holding for 10us ensures the sensor 
       detects the start but samples a HIGH state.
    */
    if (bit_value) 
    {
        _delay_us (10);           // Keep bus LOW for short amount of time
        DDRD &= ~(1 << TEMP_SENSOR); // Release bus (set as input)
        _delay_us (50);           // Recovery time
    }



    /* 
       Write 0 slot: The master pulls the bus LOW and holds it for 
       the duration of the slot (minimum 60us). Holding for 55-60us 
       ensures the sensor samples a LOW state during its 15-60us window.
    */
    else 
    {
        _delay_us (55);           // Keep bus LOW for almost the whole slot time
        DDRD &= ~(1 << TEMP_SENSOR);  // Release bus (set as input)
        _delay_us (5);            // Recovery time
    }

    sei (); // Atomic Block End

    /* 
       Global recovery time: The dasheet states that the sensor needs a
       minimum of a 1us recovery time between individual write slots.
    */
    _delay_us (2);               // This ensures at least 2us of HIGH bus between any two bits
}




static void sensor_write_byte (uint8_t data)
{
    // Loop 8 times to read each bit of the byte
    for (uint8_t i = 0; i < 8; i++) 
    {
        sensor_write_bit(data & 0x01); // Isolate bit
        data >>= 1;                              // Shift bit
    }
}




static uint8_t sensor_read_bit (void)
{
    uint8_t bit_value = 0;

    cli (); // Atomic Block Start: Protect the timing-sensitive sampling window

    /* 
       Every read slot is initiated by the master pulling the bus LOW 
       for a minimum of 1us. We hold it for 2us to ensure the sensor 
       detects the start of the slot (TINIT).
    */
    DDRD |= (1 << TEMP_SENSOR);    // Set pin as output
    PORTD &= ~(1 << TEMP_SENSOR);  // Pull bus LOW
    _delay_us(2);
    


    /* 
       After the initial pulse, the master must release the bus so the 
       sensor can take control. We then wait for the signal to stabilize.
       The datasheet states data is valid for 15us from the start of the slot.
    */
    DDRD &= ~(1 << TEMP_SENSOR);   // Release bus (set as input)
    _delay_us (10);

    
    
    /* 
       The master must sample within 15us from the falling edge. 
       Having already waited 2us, an additional 10us delay brings us 
       to 12us total. This provides a safety margin, sampling just 
       before the 15us limit (TSAMPLE).
    */
    if (PIND & (1 << TEMP_SENSOR))
    {
        bit_value = 1;
    }

    // else: do nothing as the bit is already 0 by default

    
    
    /* 
       The total time slot must be at least 60us. 
       We wait the remaining time (approx 50us) to complete the slot 
       and allow the mandatory recovery time before the next operation.
    */
    _delay_us (50);



    sei (); // Atomic Block End

    /* 
       Global recovery time: The datasheet states a minimum of 1us 
       is required between individual slots to let the bus stabilize.
    */
    _delay_us (2);

    return bit_value;
}




static uint8_t sensor_read_byte (void)
{
    uint8_t scratchpad_byte = 0; // Temporary container for the received byte

    // Loop 8 times to read each bit of the byte
    for (uint8_t i = 0; i < 8; i++) 
    {
        uint8_t bit = sensor_read_bit();
        
        if (bit == 1) 
        {
            // The DS18B20 transmits the Least Significant Bit (LSB) first

            scratchpad_byte |= (1 << i); // Set the i-th bit of scratchpad_byte
        }

        // else: do nothing as the bit is already 0 by default
    }

    return scratchpad_byte;
}



int16_t get_raw_temperature (void)
{
    // Persistent variables to track state across function calls
    static uint32_t last_conversion_start = 0;
    static int16_t raw_temperature = 0;
    static uint8_t conversion_in_progress = 0;
    
    uint32_t current_time = get_millis ();

    /* STATE 1: Start conversion.
       If no conversion is currently running, we trigger the DS18B20 to start
       calculating the temperature. This is a non-blocking request.
    */
    if (!conversion_in_progress) 
    {
        // If sensor not detected return TEMP_SENSOR_ERROR
        if (sensor_reset() == 0)
        {
            return TEMP_SENSOR_ERROR;
        }

        sensor_write_byte(TEMP_SKIP_ROM);
        sensor_write_byte(TEMP_CONVERT_T);
        
        last_conversion_start = current_time;
        conversion_in_progress = 1;
    }

    /* STATE 2: Wait for conversion and read data.
       The DS18B20 needs up to 750ms for a 12-bit conversion. We check if 
       enough time has passed without blocking the execution of the main loop.
    */
    if (conversion_in_progress && (current_time - last_conversion_start >= 750)) 
    {
        /* Once 750ms have passed, the data is ready in the Scratchpad.
           If sensor not detected return TEMP_SENSOR_ERROR
        */
        if (sensor_reset() == 0)
        {
        conversion_in_progress = 0;
        return TEMP_SENSOR_ERROR;
        }

        sensor_write_byte(TEMP_SKIP_ROM);
        sensor_write_byte(TEMP_READ_SCRATCHPAD);
        
        uint8_t lsb = sensor_read_byte();
        uint8_t msb = sensor_read_byte();
        
        // Combine bytes into the static raw value
        raw_temperature = (msb << 8) | lsb;
        
        // Reset flag to allow a new conversion cycle in the next call
        conversion_in_progress = 0; 
    }

    /* Return the last valid temperature reading.
       While a conversion is in progress, this function returns the 
       previous value, ensuring the display remains stable and the 
       system stays responsive
    */
    return raw_temperature;
}




/* 
   The DS18B20's default resolution is 12-bit, where each digital 
   increment represents 0.0625°C (1/16th of a degree). 
   Multiplying the signed raw value by this resolution converts 
   the binary fixed-point data into a standard float Celsius value.
*/
float convert_to_celsius (int16_t raw_temperature)
{
    return (float)raw_temperature * 0.0625;
}




float convert_to_fahrenheit (float temp_celsius)
{
    return (temp_celsius * 1.8) + 32.0;
}




float convert_to_kelvin (float temp_celsius)
{
    return temp_celsius + 273.15;
}