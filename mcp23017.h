#ifndef __MCP23017_H__
#define __MCP23017_H__

#include <stdint.h>
#include "i2c.h"
#include "gpio_port.h"

#define MCP23017_DEV_ID			0x27

// Addresses of registers associated with GPIO ports (GPIOB specifically)
#define MCP23017_IODIRB			0x01
#define MCP23017_GPPUB			0x0D
#define MCP23017_GPIOB			0x13

#define MCP23017_GPIO_BASE         	GPIOA_BASE
#define MCP23017_I2C_BASE          	I2C1_BASE
#define MCP23017_I2C_SCL_PIN       	PA6
#define MCP23017_I2C_SDA_PIN       	PA7
#define MCP23017_I2C_SCL_PCTL_M     GPIO_PCTL_PA6_M
#define MCP23017_I2C_SCL_PIN_PCTL 	GPIO_PCTL_PA6_I2C1SCL
#define MCP23017_I2C_SDA_PCTL_M     GPIO_PCTL_PA7_M
#define MCP23017_I2C_SDA_PIN_PCTL  	GPIO_PCTL_PA7_I2C1SDA

//*****************************************************************************
// Configure the MCP23017 to take input from the four directional buttons.
//*****************************************************************************
i2c_status_t configure_buttons(void);

//*****************************************************************************
// Get input from the four directional buttons.
//*****************************************************************************
i2c_status_t get_button_data(uint8_t *data);

//*****************************************************************************
// Initialize the I2C peripheral.
//*****************************************************************************
bool mcp23017_init(void);


#endif
