#include "button.h"
#include "mcu_timer.h"

// --- GLOBAL VARIABLES ---

volatile DisplayState unit_measure = CELSIUS; // FSM state

// ---- ISR ----

ISR (INT0_vect)
{
    static uint32_t last_interrupt_time = 0; // Timestamp of the last valid button press (for debouncing) 
    uint32_t current_time = system_millis;   // Current time

    // Debounce management (200 millis)
    if (current_time - last_interrupt_time > 200)
    {
        unit_measure = (unit_measure + 1) % (TOTAL_STATES); // Change FSM state
        last_interrupt_time = current_time;                 // Update last button pressure time
    }
}