#ifndef BUTTON_H
#define BUTTON_H

#include "fsm_states.h"
#include "avr/interrupt.h"

extern volatile DisplayState unit_measure;

ISR (INT0_vect);

#endif