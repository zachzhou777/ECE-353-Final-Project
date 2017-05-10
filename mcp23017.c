#include "mcp23017.h"

//*****************************************************************************
// Writes a single byte of data out to the MCP23017
//
// Paramters
//    i2c_base:   a valid base address of an I2C peripheral
//
//    address:    8-bit address of the byte being written
//
//    data:       Data written to the MCP23017.
//
// Returns
// I2C_OK if the byte was written to the MCP23017.
//*****************************************************************************
static i2c_status_t mcp23017_byte_write
(
  uint32_t  i2c_base,
  uint8_t  address,
  uint8_t   data
)
{
  i2c_status_t status;
  
  // Before doing anything, make sure the I2C device is idle
  while (I2CMasterBusy(i2c_base)) {};
	
  // Set the I2C address 
	status = i2cSetSlaveAddr(i2c_base, MCP23017_DEV_ID, I2C_WRITE);
  if (status != I2C_OK) return status;
	
  // Send the address
	status = i2cSendByte(i2c_base, address, I2C_MCS_START | I2C_MCS_RUN);
	if (status != I2C_OK) return status;
	
	 // Send the byte of data to write
	status = i2cSendByte(i2c_base, data, I2C_MCS_RUN | I2C_MCS_STOP);
	
  return status;
}


//*****************************************************************************
// Configure the MCP23017 to take input from the four directional buttons.
//*****************************************************************************
i2c_status_t configure_buttons(void)
{
	i2c_status_t status;
	
	// Configure GPIOB port as input
	status = mcp23017_byte_write(MCP23017_I2C_BASE, MCP23017_IODIRB, 0xFF);
	if (status != I2C_OK) return status;
	
	// Enable pull-up resistors
	status = mcp23017_byte_write(MCP23017_I2C_BASE, MCP23017_GPPUB, 0x0F);
	return status;
}


//*****************************************************************************
// Get input from the four directional buttons.
//*****************************************************************************
i2c_status_t get_button_data(uint8_t *data)
{
  i2c_status_t status;
  
  // Before doing anything, make sure the I2C device is idle
  while (I2CMasterBusy(MCP23017_I2C_BASE)) {};

  // Set the I2C slave address to be the MCP23017 and in Write Mode
	status = i2cSetSlaveAddr(MCP23017_I2C_BASE, MCP23017_DEV_ID, I2C_WRITE);
  if (status != I2C_OK) return status;
	
	// Send the address of the GPIOB registers
	status = i2cSendByte(MCP23017_I2C_BASE, MCP23017_GPIOB, I2C_MCS_START | I2C_MCS_RUN);
  if (status != I2C_OK) return status;
	
	// Set the I2C slave address to be the MCP23017 and in Read Mode
	status = i2cSetSlaveAddr(MCP23017_I2C_BASE, MCP23017_DEV_ID, I2C_READ);
  if (status != I2C_OK) return status;
	
  // Get the data returned by the MCP23017 GPIOB port (button data)
	status = i2cGetByte(MCP23017_I2C_BASE, data, I2C_MCS_START | I2C_MCS_RUN | I2C_MCS_STOP);

  return status;
}


//*****************************************************************************
// Initialize the I2C peripheral.
//*****************************************************************************
bool mcp23017_init(void)
{
  // Configure I2C GPIO pins
  if (!gpio_enable_port(MCP23017_GPIO_BASE)) return false;
  
  // Configure SCL 
  if (!gpio_config_digital_enable(MCP23017_GPIO_BASE, MCP23017_I2C_SCL_PIN)) return false;
  if (!gpio_config_alternate_function(MCP23017_GPIO_BASE, MCP23017_I2C_SCL_PIN)) return false;
  if (!gpio_config_port_control(MCP23017_GPIO_BASE, MCP23017_I2C_SCL_PCTL_M, MCP23017_I2C_SCL_PIN_PCTL)) return false;
  
  // Configure SDA 
  if (!gpio_config_digital_enable(MCP23017_GPIO_BASE, MCP23017_I2C_SDA_PIN)) return false;
  if (!gpio_config_open_drain(MCP23017_GPIO_BASE, MCP23017_I2C_SDA_PIN)) return false;
  if (!gpio_config_alternate_function(MCP23017_GPIO_BASE, MCP23017_I2C_SDA_PIN)) return false;
  if (!gpio_config_port_control(MCP23017_GPIO_BASE, MCP23017_I2C_SDA_PCTL_M, MCP23017_I2C_SDA_PIN_PCTL)) return false;
    
  // Initialize the I2C peripheral
  if (initializeI2CMaster(MCP23017_I2C_BASE)!= I2C_OK) return false;
  return true;
}
