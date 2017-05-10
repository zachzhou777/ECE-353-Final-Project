#include "debounce.h"

//*****************************************************************************
// Because every debounce function needs the same FSM, we put it in a function.
//*****************************************************************************
bool debounce_fsm(DEBOUNCE_STATES *state, bool pin_logic_level)
{
  switch (*state)
  {
    case DEBOUNCE_ONE:
    {
      if (pin_logic_level) *state = DEBOUNCE_ONE;
      else *state = DEBOUNCE_1ST_ZERO;
      break;
    }
    case DEBOUNCE_1ST_ZERO:
    {
      if (pin_logic_level) *state = DEBOUNCE_ONE;
      else *state = DEBOUNCE_2ND_ZERO;
      break;
    }
    case DEBOUNCE_2ND_ZERO:
    {
      if (pin_logic_level) *state = DEBOUNCE_ONE;
      else *state = DEBOUNCE_PRESSED;
      break;
    }
    case DEBOUNCE_PRESSED:
    {
      if (pin_logic_level) *state = DEBOUNCE_ONE;
      else *state = DEBOUNCE_PRESSED;
      break;
    }
    default: while (1) {};
  }
  
  if (*state == DEBOUNCE_2ND_ZERO) return true;
  else return false;
}

//*****************************************************************************
// Detect a single button press regardless of how long the user presses SW1.
//*****************************************************************************
bool sw1_pressed(void)
{
  static DEBOUNCE_STATES state = DEBOUNCE_ONE;
  return debounce_fsm(&state, lp_io_read_pin(SW1_BIT));
}

//*****************************************************************************
// Detect a single button press regardless of how long the user presses SW2.
//*****************************************************************************
bool sw2_pressed(void)
{
  static DEBOUNCE_STATES state = DEBOUNCE_ONE;
  return debounce_fsm(&state, lp_io_read_pin(SW2_BIT));
}

//*****************************************************************************
// Detect a single button press regardless of how long the user presses the 
// up button.
//*****************************************************************************
bool up_pressed(void)
{
  static DEBOUNCE_STATES state = DEBOUNCE_ONE;
  uint8_t pin_logic_level;
  get_button_data(&pin_logic_level);
  pin_logic_level &= 1<<UP_BUTTON_PIN;
  return debounce_fsm(&state, pin_logic_level);
}

//*****************************************************************************
// Detect a single button press regardless of how long the user presses the 
// down button.
//*****************************************************************************
bool down_pressed(void)
{
  static DEBOUNCE_STATES state = DEBOUNCE_ONE;
  uint8_t pin_logic_level;
  get_button_data(&pin_logic_level);
  pin_logic_level &= 1<<DOWN_BUTTON_PIN;
  return debounce_fsm(&state, pin_logic_level);
}

//*****************************************************************************
// Detect a single button press regardless of how long the user presses the 
// left button.
//*****************************************************************************
bool left_pressed(void)
{
  static DEBOUNCE_STATES state = DEBOUNCE_ONE;
  uint8_t pin_logic_level;
  get_button_data(&pin_logic_level);
  pin_logic_level &= 1<<LEFT_BUTTON_PIN;
  return debounce_fsm(&state, pin_logic_level);
}

//*****************************************************************************
// Detect a single button press regardless of how long the user presses the 
// right button.
//*****************************************************************************
bool right_pressed(void)
{
  static DEBOUNCE_STATES state = DEBOUNCE_ONE;
  uint8_t pin_logic_level;
  get_button_data(&pin_logic_level);
  pin_logic_level &= 1<<RIGHT_BUTTON_PIN;
  return debounce_fsm(&state, pin_logic_level);
}
