// --- NUMBER OF STATES ---

#ifndef TOTAL_STATES
#define TOTAL_STATES 3

#endif

// --- FSM STATES DEFINITION ---

#ifndef FSM_STATES
#define FSM_STATES

typedef enum 
{
    CELSIUS,
    FAHRENHEIT,
    KELVIN
} DisplayState;

#endif