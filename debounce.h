#ifndef __DEBOUNCE_H__
#define __DEBOUNCE_H__

#include <stdbool.h>
#include <stdint.h>
#include "launchpad_io.h"
#include "mcp23017.h"

// Four directional buttons correspond to these GPIOB pins on the IO expander
#define RIGHT_BUTTON_PIN			3
#define LEFT_BUTTON_PIN				2
#define DOWN_BUTTON_PIN				1
#define UP_BUTTON_PIN					0

// Debounce states
typedef enum 
{
  DEBOUNCE_ONE,
  DEBOUNCE_1ST_ZERO,
  DEBOUNCE_2ND_ZERO,
  DEBOUNCE_PRESSED
} DEBOUNCE_STATES;

//*****************************************************************************
// Because every debounce function needs the same FSM, we put it in a function.
//*****************************************************************************
extern bool debounce_fsm(DEBOUNCE_STATES *state, bool pin_logic_level);

//*****************************************************************************
// Detect a single button press regardless of how long the user presses SW1.
//*****************************************************************************
extern bool sw1_pressed(void);

//*****************************************************************************
// Detect a single button press regardless of how long the user presses SW2.
//*****************************************************************************
extern bool sw2_pressed(void);

//*****************************************************************************
// Detect a single button press regardless of how long the user presses the 
// up button.
//*****************************************************************************
extern bool up_pressed(void);

//*****************************************************************************
// Detect a single button press regardless of how long the user presses the 
// down button.
//*****************************************************************************
extern bool down_pressed(void);

//*****************************************************************************
// Detect a single button press regardless of how long the user presses the 
// left button.
//*****************************************************************************
extern bool left_pressed(void);

//*****************************************************************************
// Detect a single button press regardless of how long the user presses the 
// right button.
//*****************************************************************************
extern bool right_pressed(void);

#endif
