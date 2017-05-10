#ifndef __PWM_H__
#define __PWM_H__

#include <stdint.h>
#include "driver_defines.h"
#include "gpio_port.h"

#define PCTL_M1PWM6						0x00000500	// Configure GPIOF pin 2 to get PWM6 output

#define SYSCTL_RCC_USEPWMDIV	0x00100000  // Enable PWM divider
#define PWM_CTL_ENABLE				0x00000001	// Enable PWM generation block
#define PWM_ENABLE_GEN_THREE	0x000000C0	// Enable PWM output from Generator 3

#define GENA_COUNTDOWN				0x0000008C	// Configure PWM generator for countdown mode
#define GENB_COUNTDOWN				0x0000080C

#define CLOCK_FREQUENCY				50000000		// Clock speed is 50 MHz
#define PWM_DIVISOR						64					// PWM signal period is clock period divided by 64

//*****************************************************************************
// Initialize the PWM hardware so that when PWM output is enabled, GPIOF pin 2 
// is driven with a PWM signal.
//*****************************************************************************
extern void pwm_init(void);

//*****************************************************************************
// Drive GPIOF pin 2 with a PWM signal with frequency given by the argument 
// and duty cycle of 50%.
//*****************************************************************************
extern void enable_pwm(uint16_t frequency);

//*****************************************************************************
// Disables the PWM output.
//*****************************************************************************
extern void disable_pwm(void);

#endif
