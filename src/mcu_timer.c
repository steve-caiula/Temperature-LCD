#include <stdint.h>
#include "mcu_timer.h"

volatile uint32_t system_millis = 0;          // Global millisecond counter updated by TIMER0

uint32_t get_millis (void)
{
    /* 
       Returns the current millisecond count with atomic access.
       Since the AVR is an 8-bit architecture, reading a 32-bit variable (4 bytes)
       takes multiple CPU cycles. This function disables interrupts during the read
       to prevent the timer from updating the value mid-access, which would
       result in data corruption.
    */
    uint32_t ms_copy;        // Temporary variable

    cli ();                  // Disable interrupts to protect the 32-bit read
    ms_copy = system_millis; // Copy system_millis to a local variable while interrupts are disabled
    sei ();                  // Re-enable interrupts immediately after
    return ms_copy;          // Return the protected copy
}

ISR (TIMER0_COMPA_vect)
{
    system_millis++; // + 1 millis
}