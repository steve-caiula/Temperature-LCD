#include <avr/io.h>
#include <stdint.h>
#include <util/delay.h>
#include "pin_definitions.h"
#include "lcd.h"




static void lcd_enable_pulse (void)
{
   /* The LCD controller samples the data/RS lines 
      on the falling edge of the Enable (E) signal.
   */
   PORTB |= (1 << LCD_E);  // Set Enable HIGH to initiate the pulse
    
   /* Enable Pulse Width (tPW): The datasheet requires a minimum HIGH 
   time of 150ns. A 1us delay ensures compatibility even with 
   slower controllers or long breadboard wires.
   */
   _delay_us(1); 

   PORTB &= ~(1 << LCD_E); // Set Enable LOW (Falling edge triggers the internal latch)

   /* Enable Cycle Time (tcycE) & Execution Time: 
      The controller needs time to process the received data. 
      While most commands take ~37-40us, a 50us delay ensures the 
      next nibble or command won't be sent before the chip is ready.
   */
   _delay_us(50);
}




static void lcd_send_nibble (uint8_t nibble)
{
   /* Step 1: Clear the data interface pins.
      Although PORTB on the ATmega328P has only 6 physical pins (PB0-PB5) 
      connected to the headers, the MCU handles all I/O operations as 
      8-bit bytes. We mask bits 2, 3, 4, and 5 (from LCD_DB4 to LCD_DB7) to '0' 
      while strictly preserving the state of RS (PB0) and E (PB1).
   */
   PORTB &= ~((1 << LCD_DB4) | (1 << LCD_DB5) | (1 << LCD_DB6) | (1 << LCD_DB7));

   /* Step 2: Align and write the 4-bit nibble.
      The input 'nibble' contains data in its lowest 4 bits (0-3). 
      To map these to the physical pins PB2, PB3, PB4, and PB5, we shift 
      the value 2 positions to the left. This "re-aligns" the 4-bit 
      data packet within the 8-bit register space of PORTB.
   */
   PORTB |= ((nibble & 0x0F) << 2);
    
   /* Step 3: Data Latch.
      With the data bits now stable on the PORTB pins, we trigger the 
      Enable pulse. The LCD controller latches the state of the data 
      bus on the falling edge of the E signal.
   */
   lcd_enable_pulse ();
}




void lcd_send_byte (uint8_t data, uint8_t rs_mode)
{
   /* Register Selection (RS)
      We manipulate PB0 to select the register:
      RS = 0: Instruction Register (For commands like Clear Display)
      RS = 1: Data Register (For writing characters to DDRAM)
   */
   if (rs_mode == LCD_COMMAND) 
   {
      PORTB &= ~(1 << LCD_RS); // LCD will interpret the byte as a command
   }

   else 
   {
      PORTB |= (1 << LCD_RS); // LCD will interpret the byte as a character to display
   }

   /* 4-Bit Transfer Protocol
      The TC1602A-01T in 4-bit mode requires the 8-bit byte to be split.
      The High Nibble (DB7-DB4) must be sent first, then the Low Nibble (DB3-DB0).
   */
   lcd_send_nibble (data >> 4); // Send High Nibble (Bits 7-4)
   lcd_send_nibble (data);      // Send Low Nibble (Bits 3-0)
}




void lcd_initialization (void)
{
   /* Step 1: Power-On Delay.
      Wait for the power supply (VCC) to stabilize. The datasheet requires 
      at least 15ms after VCC rises to 4.5V. A 20ms delay provides a robust 
      safety margin for the hardware to settle.
   */
   _delay_ms (20);

   /* Step 2: Signal Initialization.
      Force both RS and E pins LOW. This guarantees that the controller 
      interprets the upcoming wake-up nibbles as commands (RS=0) and 
      prevents any accidental data latching before the sequence begins.
   */
   PORTB &= ~((1 << LCD_RS) | (1 << LCD_E));

   lcd_send_nibble (LCD_WAKE_UP);
   _delay_ms (5); // Required wait > 4.1ms

   lcd_send_nibble (LCD_WAKE_UP);
   _delay_us (100); // Required wait > 100us

   lcd_send_nibble (LCD_WAKE_UP);

   /* Step 4: 4-Bit Mode Activation.
      Send the command (0x02) to switch the data interface from 8-bit 
      to 4-bit mode. From this exact point onward, the controller expects 
      all instructions and data to be sent as two consecutive nibbles.
   */
   lcd_send_nibble (LCD_MODE_4_BIT);
   _delay_us (50);

   /* Step 5: Final Display Configuration.
      Now using the full 8-bit split transmission (lcd_send_byte), we configure 
      the display parameters: 2 lines with a 5x8 font, turn the display ON, 
      clear the memory, and set the cursor to automatically move right.
   */
   lcd_send_byte (LCD_FUNCTION_SET, LCD_COMMAND);

   lcd_send_byte (LCD_DISPLAY_ON, LCD_COMMAND);

   /* The Clear Display command takes significantly longer to execute 
      (~1.52ms) than standard commands. A 2ms delay ensures it completes 
      before the Entry Mode command is sent.
   */
   lcd_send_byte (LCD_CLEAR_DISPLAY, LCD_COMMAND);
   _delay_ms (2);

   lcd_send_byte (LCD_ENTRY_MODE, LCD_COMMAND);
}




void lcd_set_cursor (uint8_t row, uint8_t column)
{
   uint8_t destination_address;
   
   /* Step 1: Calculate the DDRAM Address.
      According to the datasheet, the internal Display Data RAM (DDRAM) 
      address for the first line starts at 0x00, 
      while the second line starts at 0x40. 
      To instruct the controller to move the cursor, the "Set DDRAM Address" 
      command requires the most significant bit (DB7) to be set to 1.
      Therefore, the base command to set the cursor on Line 1 is 0x80, 
      and for Line 2 is 0xC0. The final destination address is calculated 
      by adding the horizontal column offset to this base value (e.g., 0x80 + column).
   */
   if (row == 0)
   {
      destination_address = LCD_LINE_1 + column;
   }

   else 
   {
      destination_address = LCD_LINE_2 + column;
   }

   /* Step 2: Send the Position Command.
      The combined base address and horizontal column offset are sent 
      as a single instruction byte in LCD_COMMAND mode (RS = 0) to place 
      the cursor at the exact memory location.
   */
   lcd_send_byte(destination_address, LCD_COMMAND);
}




void lcd_print (const char *string)
{
   uint8_t i = 0;

   /* Step 1: String Traversal.
      In C, strings are arrays of characters ending with a null 
      terminator ('\0'). This loop iterates through the memory 
      until the terminator is reached, ensuring compatibility 
      with strings of any length (up to the display's memory limit).
   */
   while (string[i] != '\0') 
   {
      /* Step 2: Data Transmission.
         Each character is sent as a DATA byte (RS = 1). 
         The TC1602A-01T controller (Page 15) uses a built-in 
         Character Generator ROM (CGROM) to map ASCII-like 
         values to 5x8 pixel matrices.
      */
      lcd_send_byte(string[i], LCD_DATA);

      /* Step 3: Auto-Increment Logic.
         The controller is configured (via LCD_ENTRY_MODE) to 
         automatically increment the DDRAM address after each 
         write operation, moving the cursor to the next 
         position without manual intervention.
      */
      i++;
   }
}