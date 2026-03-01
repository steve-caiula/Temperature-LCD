// --- FSM STATES DEFINITION ---

#ifndef FSM_STATES
#define FSM_STATES

#define DISPLAY_STATES 3

typedef enum 
{
    CELSIUS,
    FAHRENHEIT,
    KELVIN,
    SENSOR_ERROR
} DisplayState;

#endif