#include "pwm.h"

//*****************************************************************************
// Initialize the PWM hardware so that when PWM output is enabled, GPIOF pin 2 
// is driven with a PWM signal.
//*****************************************************************************
void pwm_init(void)
{
	PWM0_Type *pwm = (PWM0_Type *)PWM1_BASE;
	
	// Enable PWM clock
	SYSCTL->RCGCPWM |= SYSCTL_RCGCPWM_R1;
	while (!(SYSCTL->PRPWM & SYSCTL_PRPWM_R1)) {};
	
	// Enable GPIOF pin 2 for its alternate function. Then configure port control
	gpio_config_alternate_function(GPIOF_BASE, PF2);
	gpio_config_port_control(GPIOF_BASE, GPIO_PCTL_PF2_M, PCTL_M1PWM6);
	
	// Configure RCC register to enable PWM divide. By default, the PWM clock will be the system clock 
	// divided by 64
	SYSCTL->RCC |= SYSCTL_RCC_USEPWMDIV;
	
	// Configure PWM generator for countdown mode with immediate updates to parameters
	pwm->_3_GENA = GENA_COUNTDOWN;
	pwm->_3_GENB = GENB_COUNTDOWN;
	
	// Start timers in PWM generator 3
	pwm->_3_CTL |= PWM_CTL_ENABLE;
}


//*****************************************************************************
// Drive GPIOF pin 2 with a PWM signal with frequency given by the argument 
// and duty cycle of 50%.
//*****************************************************************************
void enable_pwm(uint16_t frequency)
{
	PWM0_Type *pwm = (PWM0_Type *)PWM1_BASE;
	
	// Write period into LOAD register. Keep in mind the PWM's clock source is the system clock divided by 2
	pwm->_3_LOAD = CLOCK_FREQUENCY / (PWM_DIVISOR*frequency);
	
	// Write half the period (remember, 50% duty cycle) into CMPA and CMPB registers
	pwm->_3_CMPA = CLOCK_FREQUENCY / (2*PWM_DIVISOR*frequency);
	pwm->_3_CMPB = CLOCK_FREQUENCY / (2*PWM_DIVISOR*frequency);
	
	// Enable PWM output
	pwm->ENABLE |= PWM_ENABLE_GEN_THREE;
}


//*****************************************************************************
// Disables the PWM output.
//*****************************************************************************
void disable_pwm(void)
{
	PWM0_Type *pwm = (PWM0_Type *)PWM1_BASE;
	
	// Disable PWM output
	pwm->ENABLE &= ~PWM_ENABLE_GEN_THREE;
}
