//*****************************************************************************
// ECE 353 Spring 2017 Final Project - Space Invaders
// By Brent Drazner & Zachary Zhou
//*****************************************************************************

#ifndef __MAIN_H__
#define __MAIN_H__

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "TM4C123.h"
#include "adc.h"
#include "gpio_port.h"
#include "timers.h"
#include "i2c.h"
#include "pc_buffer.h"
#include "uart.h"
#include "launchpad_io.h"
#include "lcd.h"
#include "ps2.h"
#include "ft6x06.h"
#include "serial_debug.h"
#include "fonts.h"
#include "space_invaders_images.h"
#include "pwm.h"
#include "debounce.h"

#define THIRTY_MS_INTERVALS				15000					// #_MS_INTERVALS macros are loaded into a timer's TAILR register
#define TEN_MS_INTERVALS					5000
#define HUNDRED_MS_INTERVALS			50000
#define PRESCALER									100						// Prescaler for all timers

#define FIRING_SOUND_CNT_INIT			5							// When a tank/alien fires, a firing sound should persist for 5 Timer0B interrupts (~50 ms)

#define MAX_ADC_VALUE							0xFFF					// Maximum 12-bit value
#define FAST_LEFT									0.97					// Thresholds for what fraction of the max ADC value the joystick should be held in order 
#define SLOW_LEFT									0.53					// to move at a given speed and direction. We got these numbers experimentally, i.e., we 
#define SLOW_RIGHT								0.47					// kept tweaking these macros until we got the game to "feel right"
#define FAST_RIGHT								0.03

#define SCREEN_WIDTH							240						// Dimensions of LCD screen in pixels
#define SCREEN_HEIGHT							320

#define NUM_COLUMN_ALIENS					6							// Denote the number of aliens in a column/row
#define NUM_ROW_ALIENS						5

#define MAX_DEPTH									6							// Maximum depth of aliens

#define ELITE_POINTS							40						// Point values of each of the aliens
#define NORMAL_POINTS							20
#define GRUNT_POINTS							10

#define MAX_SCORE									9999999				// Maximum 7-digit decimal number (can only fit 7 digits of score)

#define INIT_UPDATE_PERIOD				100						// Initial update period of all the aliens; update period is number of Timer1A interrupts we 
																								// should wait before moving aliens

#define KILL_DELAY_SECONDS				0.2						// Constants related to delaying the game for various events
#define WIN_DELAY_SECONDS					2
#define LOSE_DELAY_SECONDS				2
#define DEATH_DELAY_SECONDS				1
#define ONE_SECOND								50000000			// Number of core clock cycles in one second

#define INIT_ALIEN_FIRING_PERIOD	400						// Refers to initial average number of Timer0B interrupts between any two aliens firing
#define MIN_ALIEN_FIRING_PERIOD		100						// After each round, alien firing period decreases. This is the minimum alien firing period
#define DELTA_ALIEN_FIRING_PERIOD	50						// Alien firing period decreases by this amount after each round

#define MAIN_MENU									1							// Application runs in three distinct modes: main menu, help menu, and game mode
#define HELP_MENU									2
#define GAME_MODE									3

#define PLAY_LEFT									153						// Pixel boundaries of "PLAY GAME"
#define PLAY_RIGHT								85
#define PLAY_TOP									112
#define PLAY_BOTTOM								80

#define HELP_LEFT									153						// Pixel boundaries of "HELP"
#define HELP_RIGHT								85
#define HELP_TOP									48
#define HELP_BOTTOM								32

#define MAIN_LEFT									239						// Pixel boundaries of "<MAIN MENU"
#define MAIN_RIGHT								69
#define MAIN_TOP									319
#define MAIN_BOTTOM								303

#define NUM_RAND_COLORS						8							// Number of random colors we can choose from for coloring the aliens

#define MAX_ALIEN_MISSILES				10						// Maximum number of alien missiles allowed onscreen
#define MISSILE_SPEED_DIVISOR			5							// Tank's missile speed / this value = alien missile speeds

#define MINI_NUKE_KILLS						2							// How many aliens are killed off by the various nukes
#define NUKE_KILLS								4
#define MEGATON_KILLS							6

#define INIT_MINI_NUKES						2							// How many of each nuke type the player starts with
#define INIT_NUKES								1
#define INIT_MEGATONS							0

#define MINI_NUKE_ADD_RATE				2							// When round is a multiple of these numbers, the corresponding nuke is added
#define NUKE_ADD_RATE							3
#define MEGATON_ADD_RATE					5

#define MAX_MINI_NUKES						25						// How many of each nuke type the player can hold at most
#define MAX_NUKES									15
#define MAX_MEGATONS							5

#define EXPLOSION_FREQUENCY				50						// Frequency in Hz of the sounds made by various ingame events
#define TANK_FIRING_FREQUENCY			75
#define ALIEN_FIRING_FREQUENCY		100


// Each struct has coordinate data and an is_alive field that indicates whether 
// the item is alive, i.e., on the screen and contributing to gameplay
typedef struct {
  uint16_t x_position;
  uint16_t y_position;
	uint16_t color;				// Alien's color
	bool is_alive;
} Alien;

typedef struct {
  uint16_t position;
	bool is_alive;
	bool can_fire;				// Tank can only have one missile onscreen at a time
} Tank;

typedef struct {
	uint16_t x_position;
	uint16_t y_position;
	bool is_alive;
} Missile;


#endif
