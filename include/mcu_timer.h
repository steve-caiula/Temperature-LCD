#ifndef MCU_TIMER_H
#define MCU_TIMER_H

#include <stdint.h>
#include <avr/interrupt.h>

extern volatile uint32_t system_millis;

// --- get_millis FUNCTION ---

uint32_t get_millis (void);

ISR (TIMER0_COMPA_vect);


#endif