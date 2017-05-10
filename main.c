//*****************************************************************************
// ECE 353 Spring 2017 Final Project - Space Invaders
// By Brent Drazner & Zachary Zhou
//*****************************************************************************

#include "main.h"

// Boolean flags for each interrupt
volatile bool ALERT_TIMER0A_UPDATE;
volatile bool ALERT_TIMER0B_UPDATE;
volatile bool ALERT_TIMER1A_UPDATE;
volatile bool ALERT_ADC0SS2_UPDATE;

// Description of x_firing_sound_counter field: A firing sound must be made for a period of time, but the 
// game must also be active while a firing sound is generated. Therefore, the sound must be disabled 
// after a set period of time, but other code must execute between when the buzzer is enabled and when 
// the buzzer is disabled. To do this, we'll use a counter that is initialized to 5 when sound must be 
// generated and gets decremented every Timer0B interrupt. When it hits 0, sound generation stops. Each 
// firing sound will then last ~50 ms.

// Fields related to the aliens, i.e., their movement, speed of movement, number alive, and missiles
Alien alien_array[NUM_COLUMN_ALIENS][NUM_ROW_ALIENS];
uint16_t update_period;												// How many Timer1A updates happen before aliens move. Gets reduced as aliens die
uint16_t alien_firing_period;									// Average number of Timer0B updates that happen between two consecutive alien fires
uint8_t alien_firing_sound_counter;
uint8_t num_alive;
uint8_t init_depth;														// Start at depth = 1, increments every round
bool moving_right;														// Indicates direction
bool switch_direction;												// Global indicator of when to switch direction, i.e., invert moving_right
bool move_down;																// Global indicator of when to move all aliens down
bool deassert_move_down;											// Knocks down move_down
Missile alien_missiles[MAX_ALIEN_MISSILES];		// Maintain record of missiles. Treat this like a list ADT, i.e., leftmost missiles are 
uint8_t num_active_missiles = 0;							// all alive and num_active_missiles is an index to the next position in the list to 
																							// add a missile to

// Fields related to the tank, including ammo count
Tank tank;
Missile tank_missile;													// Maintain record of tank's missile
uint8_t tank_firing_sound_counter;
uint8_t num_mini_nukes;
uint8_t num_nukes;
uint8_t num_megatons;

// Score variable
uint32_t score;

// Round number
uint16_t round;

// Indicate whether sound is being produced or not. When sound field is true, blue LED should be on to 
// indicate game is unmuted. Else blue LED is off
bool sound = true;

// Messages to display at various moments throughout game
char win_msg[] = "You Win!";
char lose_msg[] = "GAME  OVER";
char welcome_msg1[] = "WELCOME TO";
char welcome_msg2[] = "SPACE INVADERS";
char credit_msg1[] = "By Brent D";
char credit_msg2[] = "& Zach Z";
char play_msg1[] = "PLAY";
char play_msg2[] = "GAME";
char help_main_msg[] = "HELP";
char help_msg1[] = "DIRECTIONS";
char help_msg2[] = "Move:";
char help_msg3[] = "Joystick";
char help_msg4[] = "Fire:";
char help_msg5[] = "A";
char help_msg6[] = "Mini-Nuke:";
char help_msg7[] = "B";
char help_msg8[] = "Nuke:";
char help_msg9[] = "X";
char help_msg10[] = "Megaton:";
char help_msg11[] = "Y";
char help_msg12[] = "Mute:";
char help_msg13[] = "SW1/SW2";
char help_msg14[] = "<MAIN MENU";
char score_msg1[] = "Score: 0      ";			// Pad with spaces so the entire top of the screen is gray. The 
char score_msg2[7];												// '0' gets drawn over once the player scores for the first time
char ammo_cnt_msg[] = "MAKE THIS GRAY";		// Print a gray bar (text and background are gray) underneath score
char mini_cnt_msg[2];											// Then print ammo over it
char nuke_cnt_msg[2];
char mega_cnt_msg[1];

//*****************************************************************************
// Interrupt service routines. Each sets its respective Boolean flag to true 
// and acknowledges the interrupt. With the exception of the Timer0B handler, 
// which is used to debounce SW1/SW2, they are as minimal as can be; all the 
// work is saved for main loop.
//*****************************************************************************
void TIMER0A_Handler(void)
{	
	ALERT_TIMER0A_UPDATE = true;
	TIMER0->ICR |= TIMER_ICR_TATOCINT;
}

void TIMER0B_Handler(void)
{
	// We debounce SW1 and SW2 in the handler rather than in the main application because throughout the game, 
	// we stall the application using gp_timer_wait(). For example, when an alien is killed, we briefly delay 
	// the game to display an explosion and produce a sound effect. When a round is over, we delay the game to 
	// show the win message. We want it so that the player can mute the game at anytime, including these delays
	if (sw1_pressed() || sw2_pressed()) sound = !sound;
	ALERT_TIMER0B_UPDATE = true;
	TIMER0->ICR |= TIMER_ICR_TBTOCINT;
}

void TIMER1A_Handler(void)
{
	ALERT_TIMER1A_UPDATE = true;
	TIMER1->ICR |= TIMER_ICR_TATOCINT;
}

void ADC0SS2_Handler(void)
{
	ALERT_ADC0SS2_UPDATE = true;
	ADC0->ISC |= ADC_ISC_IN2;
}


void DisableInterrupts(void)
{
  __asm {
    CPSID  I
  }
}

void EnableInterrupts(void)
{
  __asm {
    CPSIE  I
  }
}


//*****************************************************************************
// Initialize and configure all the relevant hardware.
//*****************************************************************************
void initialize_hardware(void)
{
  DisableInterrupts();
	
	// Initialize serial debug
	init_serial_debug(true, true);
	
	// Configure timers
	gp_timer_config_16(TIMER0_BASE, TIMER_TAMR_TAMR_PERIOD, true, true, THIRTY_MS_INTERVALS, PRESCALER);
	gp_timer_config_16(TIMER0_BASE, TIMER_TBMR_TBMR_PERIOD, false, true, TEN_MS_INTERVALS, PRESCALER);
	gp_timer_config_16(TIMER1_BASE, TIMER_TAMR_TAMR_PERIOD, true, true, HUNDRED_MS_INTERVALS, PRESCALER);
	gp_timer_config_32(TIMER2_BASE, TIMER_TAMR_TAMR_1_SHOT, false, false);
	
	// Configure GPIO pins
	lcd_config_gpio();
	
	// Configure LCD touchscreen
	lcd_config_screen();
	lcd_clear_screen(LCD_COLOR_GRAY);
	ft6x06_init();
	
	// Configure port expander to read directional button input
	mcp23017_init();
	configure_buttons();
	
	// Configure PS2 joystick
	ps2_initialize_SS2();
	
	// Configure Launchpad GPIO pins connected to LEDs and push buttons
	lp_io_init();
	
	// Configure PWM generator
	pwm_init();
	
  EnableInterrupts();
}


//*****************************************************************************
// Returns a random color. Arguments are colors that can't be chosen.
//*****************************************************************************
uint16_t rand_color(uint16_t color1, uint16_t color2)
{
	uint16_t rand_color;
	uint8_t selection;
	uint8_t i;
	for (i = 0; i < 3; i++)			// Make 3 passes before giving up and returning default colors
	{
		selection = rand()%NUM_RAND_COLORS;
		switch (selection)
		{
			case 0:
			{
				rand_color = LCD_COLOR_GREEN2;
				break;
			}
			case 1:
			{
				rand_color = LCD_COLOR_BLUE;
				break;
			}
			case 2:
			{
				rand_color = LCD_COLOR_BLUE2;
				break;
			}
			case 3:
			{
				rand_color = LCD_COLOR_YELLOW;
				break;
			}
			case 4:
			{
				rand_color = LCD_COLOR_ORANGE;
				break;
			}
			case 5:
			{
				rand_color = LCD_COLOR_CYAN;
				break;
			}
			case 6:
			{
				rand_color = LCD_COLOR_MAGENTA;
				break;
			}
			default:
			{
				rand_color = LCD_COLOR_BROWN;
			}
		}
		
		if ((rand_color != color1) && (rand_color != color2)) return rand_color;
	}
	// At this point (which rarely gets reached by expectation), return default colors
	if ((LCD_COLOR_GREEN2 != color1) && (LCD_COLOR_GREEN2 != color2)) return LCD_COLOR_GREEN2;
	if ((LCD_COLOR_YELLOW != color1) && (LCD_COLOR_YELLOW != color2)) return LCD_COLOR_YELLOW;
	return LCD_COLOR_ORANGE;
}


//*****************************************************************************
// Prints the main menu to the screen. Takes three arguments to color the 
// three aliens on the screen.
//*****************************************************************************
void print_main_menu(uint16_t rand_color1, uint16_t rand_color2, uint16_t rand_color3)
{
	lcd_print_stringXY(welcome_msg1, 2, 1, LCD_COLOR_CYAN, LCD_COLOR_GRAY);
	lcd_print_stringXY(welcome_msg2, 0, 2, LCD_COLOR_CYAN, LCD_COLOR_GRAY);
	lcd_print_stringXY(credit_msg1, 2, 3, LCD_COLOR_GREEN, LCD_COLOR_GRAY);
	lcd_print_stringXY(credit_msg2, 3, 4, LCD_COLOR_GREEN, LCD_COLOR_GRAY);
	lcd_draw_image(165, menu_alienWidthPixels, 170, menu_alienHeightPixels, menu_alienBitmaps, rand_color1, LCD_COLOR_GRAY);
	lcd_draw_image(95, menu_alienWidthPixels, 170, menu_alienHeightPixels, menu_alienBitmaps, rand_color2, LCD_COLOR_GRAY);
	lcd_draw_image(25, menu_alienWidthPixels, 170, menu_alienHeightPixels, menu_alienBitmaps, rand_color3, LCD_COLOR_GRAY);
	lcd_print_stringXY(play_msg1, 5, 13, LCD_COLOR_RED, LCD_COLOR_YELLOW);
	lcd_print_stringXY(play_msg2, 5, 14, LCD_COLOR_RED, LCD_COLOR_YELLOW);
	lcd_print_stringXY(help_main_msg, 5, 17, LCD_COLOR_RED, LCD_COLOR_YELLOW);
}

//*****************************************************************************
// Prints the help menu to the screen.
//*****************************************************************************
void print_help_menu(void)
{
	lcd_print_stringXY(help_msg1, 2, 2, LCD_COLOR_CYAN, LCD_COLOR_ORANGE);
	lcd_print_stringXY(help_msg2, 0, 5, LCD_COLOR_CYAN, LCD_COLOR_ORANGE);
	lcd_print_stringXY(help_msg3, 6, 5, LCD_COLOR_BLACK, LCD_COLOR_ORANGE);
	lcd_print_stringXY(help_msg4, 0, 7, LCD_COLOR_CYAN, LCD_COLOR_ORANGE);
	lcd_print_stringXY(help_msg5, 13, 7, LCD_COLOR_GREEN, LCD_COLOR_ORANGE);
	lcd_print_stringXY(help_msg6, 0, 9, LCD_COLOR_CYAN, LCD_COLOR_ORANGE);
	lcd_print_stringXY(help_msg7, 13, 9, LCD_COLOR_RED, LCD_COLOR_ORANGE);
	lcd_print_stringXY(help_msg8, 0, 11, LCD_COLOR_CYAN, LCD_COLOR_ORANGE);
	lcd_print_stringXY(help_msg9, 13, 11, LCD_COLOR_BLUE, LCD_COLOR_ORANGE);
	lcd_print_stringXY(help_msg10, 0, 13, LCD_COLOR_CYAN, LCD_COLOR_ORANGE);
	lcd_print_stringXY(help_msg11, 13, 13, LCD_COLOR_YELLOW, LCD_COLOR_ORANGE);
	lcd_print_stringXY(help_msg12, 0, 15, LCD_COLOR_CYAN, LCD_COLOR_ORANGE);
	lcd_print_stringXY(help_msg13, 7, 15, LCD_COLOR_BLACK, LCD_COLOR_ORANGE);
	lcd_print_stringXY(help_msg14, 0, 0, LCD_COLOR_RED, LCD_COLOR_YELLOW);
}

//*****************************************************************************
// Prints the ammo count and round number to the screen.
//*****************************************************************************
void print_ammo(uint8_t num_mini_nukes, uint8_t num_nukes, uint8_t num_megatons)
{
	// Draw a gray bar
	lcd_print_stringXY(ammo_cnt_msg, 0, 1, LCD_COLOR_GRAY, LCD_COLOR_GRAY);
	
	// Draw the nuke images in the appropriate color
	lcd_draw_image(222, nukeWidthPixels, 289, nukeHeightPixels, nukeBitmaps, LCD_COLOR_RED, LCD_COLOR_GRAY);
	lcd_draw_image(171, nukeWidthPixels, 289, nukeHeightPixels, nukeBitmaps, LCD_COLOR_BLUE, LCD_COLOR_GRAY);
	lcd_draw_image(120, nukeWidthPixels, 289, nukeHeightPixels, nukeBitmaps, LCD_COLOR_YELLOW, LCD_COLOR_GRAY);
	
	// Convert each of the numbers to a string
	sprintf(mini_cnt_msg, "%d", num_mini_nukes);
	sprintf(nuke_cnt_msg, "%d", num_nukes);
	sprintf(mega_cnt_msg, "%d", num_megatons);
	
	// Print the quantities next to its appropriate nuke image
	lcd_print_stringXY(mini_cnt_msg, 1, 1, LCD_COLOR_WHITE, LCD_COLOR_GRAY);
	lcd_print_stringXY(nuke_cnt_msg, 4, 1, LCD_COLOR_WHITE, LCD_COLOR_GRAY);
	lcd_print_stringXY(mega_cnt_msg, 7, 1, LCD_COLOR_WHITE, LCD_COLOR_GRAY);
}


//*****************************************************************************
// Kills an alien at a the array redraw_positions specified by arguments. 
// Increases score accordingly.
//*****************************************************************************
void kill_alien(uint8_t row_index, uint8_t column_index, bool missile_kill)
{
	// Erase the alien and missile
	lcd_draw_image(alien_array[row_index][column_index].x_position, alienWidthPixels, 
			alien_array[row_index][column_index].y_position, alienHeightPixels, alienBitmaps, 
			LCD_COLOR_BLACK, LCD_COLOR_BLACK);
	
	// Erase the missile if it exists. Also draw the explosion if it's a missile kill
	if (missile_kill)
	{
		lcd_draw_image(tank_missile.x_position, missileWidthPixels, tank_missile.y_position, missileHeightPixels, 
				missileBitmaps, LCD_COLOR_BLACK, LCD_COLOR_BLACK);
		lcd_draw_image(alien_array[row_index][column_index].x_position+1, alien_explosionWidthPixels, 
				alien_array[row_index][column_index].y_position, alien_explosionHeightPixels, alien_explosionBitmaps, 
				LCD_COLOR_RED, LCD_COLOR_BLACK);
	}
	
	// Kill the alien and missile
	alien_array[row_index][column_index].is_alive = false;
	if (missile_kill) tank_missile.is_alive = false;
	
	// Update score
	if (row_index == 0) score += ELITE_POINTS;
	else if (row_index == 1 || row_index == 2) score += NORMAL_POINTS;
	else score += GRUNT_POINTS;
	if (score > MAX_SCORE) score = MAX_SCORE;
	
	// Print the score
	sprintf(score_msg2, "%d", score);
	lcd_print_stringXY(score_msg2, 7, 0, LCD_COLOR_WHITE, LCD_COLOR_GRAY);
	
	// Update number of aliens alive
	num_alive--;
	
	// Make the aliens move faster
	update_period -= INIT_UPDATE_PERIOD / (NUM_ROW_ALIENS*NUM_COLUMN_ALIENS);
	
	// Delay the game whenever an alien is killed by a missile so that the explosion is drawn for a while. Then erase 
	// the explosion. Produce sound accordingly
	if (missile_kill)
	{
		if (sound) enable_pwm(EXPLOSION_FREQUENCY);
		gp_timer_wait(TIMER2_BASE, KILL_DELAY_SECONDS*ONE_SECOND);
		lcd_draw_image(alien_array[row_index][column_index].x_position+1, alien_explosionWidthPixels, 
				alien_array[row_index][column_index].y_position, alien_explosionHeightPixels, alien_explosionBitmaps, 
				LCD_COLOR_BLACK, LCD_COLOR_BLACK);
		disable_pwm();
	}
}

//*****************************************************************************
// Randomly kills x aliens. Aliens killed using one of the nukes will grant 
// the player the same number of points as those killed normally.
//*****************************************************************************
void kill_x_aliens(uint8_t x)
{
	uint8_t i, j, k;
	uint8_t rand_num;
	uint8_t target = 0;
	uint8_t redraw_num = num_alive;
	uint16_t redraw_positions[x][2];		// Keep track of which coordinates to draw black over due to explosions
	
	// If this is true, kill all aliens
	if (x >= num_alive)
	{
		for (i = 0; i < NUM_COLUMN_ALIENS; i++)
		{
			for (j = 0; j < NUM_ROW_ALIENS; j++)
			{
				if (alien_array[i][j].is_alive)
				{
					// Kill alien, record positions of explosions, then draw explosions
					kill_alien(i, j, false);
					redraw_positions[target][0] = alien_array[i][j].x_position;
					redraw_positions[target][1] = alien_array[i][j].y_position;
					target++;
					lcd_draw_image(alien_array[i][j].x_position+1, alien_explosionWidthPixels, alien_array[i][j].y_position, 
							alien_explosionHeightPixels, alien_explosionBitmaps, LCD_COLOR_RED, LCD_COLOR_BLACK);
				}
			}
		}
		
		// Make sound if unmuted and delay game for a bit. Draw black over all alien explosions. Then stop making sound
		if (sound) enable_pwm(EXPLOSION_FREQUENCY);
		gp_timer_wait(TIMER2_BASE, KILL_DELAY_SECONDS*ONE_SECOND);
		for (i = 0; i < redraw_num; i++) lcd_draw_image(redraw_positions[i][0]+1, alien_explosionWidthPixels, redraw_positions[i][1], 
				alien_explosionHeightPixels, alien_explosionBitmaps, LCD_COLOR_BLACK, LCD_COLOR_BLACK);
		disable_pwm();
	}
	
	// Algorithm for determining which aliens to kill: each alien has equal probability of getting smoked; that probability is 
	// x/num_alive. Iterate through all alive aliens and generate a random number from 1 to num_alive each iteration. If the 
	// random number from 1 to x, kill the alien, otherwise let it live. With just this, it's possible for no aliens to be 
	// killed, so we make two passes through all the alive aliens. If the second pass doesn't do the trick (rarely ever happens), 
	// we'll make a third pass and kill the first however many aliens we gotta kill in order to kill x aliens.
	else
	{
		// First two passes
		for (k = 0; k < 2; k++)
		{
			for (i = 0; i < NUM_COLUMN_ALIENS; i++)
			{
				for (j = 0; j < NUM_ROW_ALIENS; j++)
				{
					if (alien_array[i][j].is_alive)
					{
						rand_num = rand()%num_alive + 1;
						if (rand_num <= x)
						{
							// Kill aliens, record positions of explosions, then draw explosions
							kill_alien(i, j, false);
							redraw_positions[target][0] = alien_array[i][j].x_position;
							redraw_positions[target][1] = alien_array[i][j].y_position;
							lcd_draw_image(alien_array[i][j].x_position+1, alien_explosionWidthPixels, alien_array[i][j].y_position, 
									alien_explosionHeightPixels, alien_explosionBitmaps, LCD_COLOR_RED, LCD_COLOR_BLACK);
							// Increment the target with each kill. Once target matches x, stop killing aliens
							target++;
							if (target >= x)	// If this is true, then we've killed all x aliens
							{
								// Delay the game once all x aliens get nuked. Produce sound. Draw black over explosions. Stop 
								// producing sound. Then break out of function by returning
								if (sound) enable_pwm(EXPLOSION_FREQUENCY);
								gp_timer_wait(TIMER2_BASE, KILL_DELAY_SECONDS*ONE_SECOND);
								for (i = 0; i < x; i++) lcd_draw_image(redraw_positions[i][0]+1, alien_explosionWidthPixels, redraw_positions[i][1], 
										alien_explosionHeightPixels, alien_explosionBitmaps, LCD_COLOR_BLACK, LCD_COLOR_BLACK);
								disable_pwm();
								return;
							}
						}
					}
				}
			}
		}
		
		// Third pass; smoke aliens unconditionally
		for (i = 0; i < NUM_COLUMN_ALIENS; i++)
		{
			for (j = 0; j < NUM_ROW_ALIENS; j++)
			{
				if (alien_array[i][j].is_alive)
				{
					// Kill aliens unconditionally, record positions, then draw explosions
					kill_alien(i, j, false);
					redraw_positions[target][0] = alien_array[i][j].x_position;
					redraw_positions[target][1] = alien_array[i][j].y_position;
					lcd_draw_image(alien_array[i][j].x_position+1, alien_explosionWidthPixels, alien_array[i][j].y_position, 
							alien_explosionHeightPixels, alien_explosionBitmaps, LCD_COLOR_RED, LCD_COLOR_BLACK);
					// Once the target is met, create the explosion sound and delay the game. Then redraw black over explosions, stop making sound, 
					// and break out of function by returning
					target++;
					if (target >= x)
					{
						// Delay the game whenever aliens get nuked. Draw black over explosions. Break out of function by returning
						if (sound) enable_pwm(EXPLOSION_FREQUENCY);
						gp_timer_wait(TIMER2_BASE, KILL_DELAY_SECONDS*ONE_SECOND);
						for (i = 0; i < x; i++) lcd_draw_image(redraw_positions[i][0]+1, alien_explosionWidthPixels, redraw_positions[i][1], 
										alien_explosionHeightPixels, alien_explosionBitmaps, LCD_COLOR_BLACK, LCD_COLOR_BLACK);
						disable_pwm();
						return;
					}
				}
			}
		}
	}
}


//*****************************************************************************
// Space Invaders
//*****************************************************************************
int main(void)
{
	// Initialize all relevant hardware
	initialize_hardware();
	
	// Counters within Timer1A. Used for moving the aliens and missiles once every so often when the counter is 0. Overflow using modulus operator
	uint8_t dont_move_aliens = 0;
	uint8_t alien_missiles_dont_move = 0;
	
	// Read ADC0SS2 conversions into this variable
	uint16_t joystick;
	
	// Read FT6x06 readings into these variables
	uint16_t lcd_x = 0;
	uint16_t lcd_y = 0;
	
	// Keep track of whether the application is in:
	// 		1) main menu (default upon reset)
	// 		2) help menu
	// 		3) game mode
	uint8_t mode = 1;
	
	// Store three random colors
	uint16_t rand_color1, rand_color2, rand_color3;
	
	// Loop counters
	int8_t i, j;
	
	// Random colors to generate before making the menu
	rand_color1 = rand_color(LCD_COLOR_BLACK, LCD_COLOR_BLACK);
	rand_color2 = rand_color(rand_color1, LCD_COLOR_BLACK);
	rand_color3 = rand_color(rand_color1, rand_color2);
	
  // Reach infinite loop
  while (1)
	{
		switch (mode)
		{
			case MAIN_MENU:
			{
				// These interrupts won't mean anything in the main menu so just set their global Booleans false whenever they interrupt
				ALERT_TIMER0A_UPDATE = false;
				ALERT_TIMER1A_UPDATE = false;
				ALERT_ADC0SS2_UPDATE = false;
				ALERT_TIMER0B_UPDATE = false;
				
				// Print the main menu				
				print_main_menu(rand_color1, rand_color2, rand_color3);
				
				// "Throw away" a random number to increase randomness of pseudo-random numbers
				rand();
				
				// If the user is touching the screen, read in the coordinates they're touching. Default lcd_x and lcd_y to 0 
				// to prevent stale values from being used (had an issue where we lost the game and immediately jumped back into 
				// another game rather than staying at the main menu)
				lcd_x = 0;
				lcd_y = 0;
				if (ft6x06_read_td_status())
				{
					lcd_x = ft6x06_read_x();
					lcd_y = ft6x06_read_y();
				}
				
				// This really just says "if the user hit "PLAY GAME" on the LCD
				if ((lcd_x > PLAY_RIGHT) && (lcd_x < PLAY_LEFT) && (lcd_y > PLAY_BOTTOM) && (lcd_y < PLAY_TOP))
				{
					// Get screen ready for game mode by initializing all game-related variables
					init_depth = 1;													// Set start depth to 1
					tank.is_alive = true;										// Set tank alive
					num_mini_nukes = INIT_MINI_NUKES;				// Initialize number of nukes
					num_nukes = INIT_NUKES;
					num_megatons = INIT_MEGATONS;
					score = 0;															// Set score to 0
					round = 1;															// Set round to 1
					alien_firing_sound_counter = 0;					// Set sound counters to 0
					tank_firing_sound_counter = 0;
					alien_firing_period = INIT_ALIEN_FIRING_PERIOD;			// Initialize firing period
					lcd_clear_screen(LCD_COLOR_BLACK);									// Clear screen to black and print score message
					lcd_print_stringXY(score_msg1, 0, 0, LCD_COLOR_WHITE, LCD_COLOR_GRAY);
					mode = GAME_MODE;												// Switch to game mode
				}
				
				// This says "else if the user hit "HELP" on the LCD
				else if ((lcd_x > HELP_RIGHT) && (lcd_x < HELP_LEFT) && (lcd_y > HELP_BOTTOM) && (lcd_y < HELP_TOP))
				{
					// Clear the screen orange to get it ready to display the help menu. Then switch to help menu
					lcd_clear_screen(LCD_COLOR_ORANGE);
					mode = HELP_MENU;
				}
				break;
			}
			
			case HELP_MENU:
			{
				// These interrupts won't mean anything in the main menu
				ALERT_TIMER0A_UPDATE = false;
				ALERT_TIMER1A_UPDATE = false;
				ALERT_ADC0SS2_UPDATE = false;
				ALERT_TIMER0B_UPDATE = false;
				
				// Print all the contents of the help menu
				print_help_menu();
				
				// Subsequent code in this case implement touchscreen functionality related to the help menu
				
				// If the user is touching the screen, read in the coordinates they're touching
				if (ft6x06_read_td_status())
				{
					lcd_x = ft6x06_read_x();
					lcd_y = ft6x06_read_y();
				}
				
				// This says "if the user hit "<MAIN MENU" on the LCD
				if ((lcd_x > MAIN_RIGHT) && (lcd_x < MAIN_LEFT) && (lcd_y > MAIN_BOTTOM) && (lcd_y < MAIN_TOP))
				{
					// Clear the screen gray to get it ready to display the main menu. Then switch to main menu
					lcd_clear_screen(LCD_COLOR_GRAY);
					mode = MAIN_MENU;
				}
				break;
			}
			
			default:		// Game mode
			{
				// While tank is alive, play a round
				while (tank.is_alive)
				{
					// Round initialization sequence
					tank.position = SCREEN_WIDTH/2 - tankWidthPixels/2;						// Initialize tank position
					tank.can_fire = true;																					// Tank can fire right from the start
					rand_color1 = rand_color(LCD_COLOR_BLACK, LCD_COLOR_BLACK);		// Set three random colors for the aliens
					rand_color2 = rand_color(rand_color1, LCD_COLOR_BLACK);
					rand_color3 = rand_color(rand_color1, rand_color2);
					// Initialize alien positions
					for (i = 0; i < NUM_COLUMN_ALIENS; i++)
					{
						for (j = 0; j < NUM_ROW_ALIENS; j++)
						{
							alien_array[i][j].x_position = (NUM_ROW_ALIENS-j)*SCREEN_WIDTH/(NUM_ROW_ALIENS+1) - alienWidthPixels/2;
							alien_array[i][j].y_position = SCREEN_HEIGHT - FONT_HEIGHT - (i+init_depth+1)*(alienHeightPixels+5);
							if (i == 0) alien_array[i][j].color = rand_color1;										// Assign colors based on row
							else if (i == 1 || i == 2) alien_array[i][j].color = rand_color2;
							else alien_array[i][j].color = rand_color3;
							alien_array[i][j].is_alive = true;																		// All aliens start alive
						}
					}
					moving_right = false;																		// Aliens move left from the start
					update_period = INIT_UPDATE_PERIOD;											// Aliens move at same speed at start of every round
					num_alive = NUM_ROW_ALIENS * NUM_COLUMN_ALIENS;					// Keep track of number of aliens alive
					print_ammo(num_mini_nukes, num_nukes, num_megatons);		// Print ammo count
					
					while (num_alive)		// While there's aliens alive, play round
					{
						// Timer0A interrupts trigger conversions of PS2 position
						if (ALERT_TIMER0A_UPDATE)
						{							
							// Trigger analog conversions of PS2 position
							trigger_X_conversion(PS2_X_ADC_CHANNEL);
							
							// Knock down ALERT_TIMER0A_UPDATE
							ALERT_TIMER0A_UPDATE = false;
						}
						
						// Timer0B interrupts make aliens fire and debounce buttons. Also involved in continuing sound production
						if (ALERT_TIMER0B_UPDATE)
						{
							// Sound counters are set when sound is enabled. When they expire, disable sound production
							if (alien_firing_sound_counter)
							{
								alien_firing_sound_counter--;
								if (!alien_firing_sound_counter) disable_pwm();
							}
							
							if (tank_firing_sound_counter)
							{
								tank_firing_sound_counter--;
								if (!tank_firing_sound_counter) disable_pwm();
							}
							
							// Alien firing logic: only the lowest alien in a column should be firing. Each lowermost alien should fire roughly 
							// once every five seconds. 100 Timer0B interrupts occur each second. Therefore, the probability a lowermost alien fires 
							// for a given Timer0B interrupt should be 1/500
							for (j = 0; j < NUM_ROW_ALIENS; j++)
							{
								for (i = NUM_COLUMN_ALIENS-1; i >= 0; i--)
								{
									// If it's alive, make it shoot with calculated probability (make a random number from 0 to 499; if it's 0, then 
									// event happens), then set i=0 to break out of innermost loop
									if((num_active_missiles < MAX_ALIEN_MISSILES) && alien_array[i][j].is_alive)
									{
										if (0 == rand()%alien_firing_period)
										{
											// If the tank's not currently making a firing sound, allow the alien to make its firing sound
											if (!tank_firing_sound_counter)
											{
												if (sound) enable_pwm(ALIEN_FIRING_FREQUENCY);
												alien_firing_sound_counter = FIRING_SOUND_CNT_INIT;
											}
											// Create a new missile in the list
											alien_missiles[num_active_missiles].x_position = alien_array[i][j].x_position + alienWidthPixels/2 - 1;
											alien_missiles[num_active_missiles].y_position = alien_array[i][j].y_position - missileHeightPixels;
											alien_missiles[num_active_missiles].is_alive = true;
											num_active_missiles++;
										}
										i = 0;		// Probability check failed, so move to the next column
									}
								}
							}
							
							// If the down button is pressed and there's not currently a tank missile in action, make the tank fire
							if (down_pressed() && !tank_missile.is_alive)
							{
								// Tank firing sound has higher precedence over alien firing sound. Even if an alien is currently making a firing 
								// sound, overpower it with the tank firing sound
								if (sound) enable_pwm(TANK_FIRING_FREQUENCY);
								tank_firing_sound_counter = FIRING_SOUND_CNT_INIT;
								alien_firing_sound_counter = 0;
								// Create a new missile
								tank_missile.x_position = tank.position + tankWidthPixels/2 - 1;
								tank_missile.y_position = tankHeightPixels;
								tank_missile.is_alive = true;
							}
							
							// Nuke logic: if appropriate directional button is pressed and there's enough ammo, decrement count of 
							// whichever nuke is used, print a new ammo count, and randomly kill the appropriate number of aliens
							
							// If the right button is pressed, release a mini-nuke (kill 3 random aliens)
							if (right_pressed() && num_mini_nukes)
							{
								num_mini_nukes--;
								print_ammo(num_mini_nukes, num_nukes, num_megatons);
								kill_x_aliens(MINI_NUKE_KILLS);
							}
							
							// If the left button is pressed, release a normal nuke (kill 5 random aliens)
							if (left_pressed() && num_nukes)
							{
								num_nukes--;
								print_ammo(num_mini_nukes, num_nukes, num_megatons);
								kill_x_aliens(NUKE_KILLS);
							}
							
							// If the up button is pressed, release a megaton (kill 7 random aliens)
							if (up_pressed() && num_megatons)
							{
								num_megatons--;
								print_ammo(num_mini_nukes, num_nukes, num_megatons);
								kill_x_aliens(MEGATON_KILLS);
							}
							
							// Knock down ALERT_TIMER0B_UPDATE
							ALERT_TIMER0B_UPDATE = false;
						}
						
						// Timer1A interrupts move tank and alien missiles, move the 
						if (ALERT_TIMER1A_UPDATE)
						{
							// Move the missiles
							if (tank_missile.is_alive)
							{
								// Draw over the old missile image in black and draw the new one in green. Note the pre-increment in the 
								// second lcd_draw_image() call
								lcd_draw_image(tank_missile.x_position, missileWidthPixels, tank_missile.y_position, missileHeightPixels, 
										missileBitmaps, LCD_COLOR_BLACK, LCD_COLOR_BLACK);
								lcd_draw_image(tank_missile.x_position, missileWidthPixels, ++tank_missile.y_position, missileHeightPixels, 
										missileBitmaps, LCD_COLOR_GREEN, LCD_COLOR_GREEN);
								
								// If the missile's beyond the top edge of the game screen (top of the screen minus 2 times font height since top 
								// two bars are reserved for score and ammo count), draw black over it and kill missile off
								if ((tank_missile.y_position + missileHeightPixels) >= (SCREEN_HEIGHT - 2*FONT_HEIGHT))
								{
									lcd_draw_image(tank_missile.x_position, missileWidthPixels, tank_missile.y_position, missileHeightPixels, 
											missileBitmaps, LCD_COLOR_BLACK, LCD_COLOR_BLACK);
									tank_missile.is_alive = false;
								}
							}
							// Only make alien missiles move once every MISSILE_SPEED_DIVISOR Timer1A interrupts
							alien_missiles_dont_move = (alien_missiles_dont_move + 1) % MISSILE_SPEED_DIVISOR;
							if (!alien_missiles_dont_move)
							{
								// Iterate through all alive missiles
								for (i = 0; i < num_active_missiles; i++)
								{
									if (alien_missiles[i].is_alive)
									{
										// Draw over the old missile image in black and draw the new one in green. Note the pre-decrement
										lcd_draw_image(alien_missiles[i].x_position, missileWidthPixels, alien_missiles[i].y_position, 
												missileHeightPixels, missileBitmaps, LCD_COLOR_BLACK, LCD_COLOR_BLACK);
										lcd_draw_image(alien_missiles[i].x_position, missileWidthPixels, --alien_missiles[i].y_position, 
												missileHeightPixels, missileBitmaps, LCD_COLOR_RED, LCD_COLOR_RED);
										
										// Detect if an alien missile has collided with the tank
										if ((alien_missiles[i].y_position <= adjusted_tankHeightPixels) && (alien_missiles[i].x_position >= tank.position) && 
												(alien_missiles[i].x_position <= tank.position + tankWidthPixels))
										{
											// Create an explosion sound, draw over the tank and missile in black, draw an explosion, wait 
											// a while, redraw black over explosion, disable sound, kill tank, and break out of inner while loop
											if (sound) enable_pwm(EXPLOSION_FREQUENCY);
											lcd_draw_image(alien_missiles[i].x_position, missileWidthPixels, alien_missiles[i].y_position, 
													missileHeightPixels, missileBitmaps, LCD_COLOR_BLACK, LCD_COLOR_BLACK);
											lcd_draw_image(tank.position, tankWidthPixels, 0, tankHeightPixels, tankBitmaps, 
													LCD_COLOR_BLACK, LCD_COLOR_BLACK);
											// Add 4 to x-coordinate to center the explosion image
											lcd_draw_image(tank.position+4, tank_explosionWidthPixels, 0, tank_explosionHeightPixels, 
													tank_explosionBitmaps, LCD_COLOR_RED, LCD_COLOR_BLACK);
											gp_timer_wait(TIMER2_BASE, DEATH_DELAY_SECONDS*ONE_SECOND);
											lcd_draw_image(tank.position+4, tank_explosionWidthPixels, 0, tank_explosionHeightPixels, 
													tank_explosionBitmaps, LCD_COLOR_BLACK, LCD_COLOR_BLACK);
											disable_pwm();
											num_alive = 0;		// Set this to 0 to break out of inner while loop
											tank.is_alive = false;
										}
										// Detect if an alien missile is off the bottom edge of the screen
										else if (alien_missiles[i].y_position <= 0)
										{
											// Draw over missile in black
											lcd_draw_image(alien_missiles[i].x_position, missileWidthPixels, alien_missiles[i].y_position, 
													missileHeightPixels, missileBitmaps, LCD_COLOR_BLACK, LCD_COLOR_BLACK);
											// If the missile to remove is not the rightmost alive missile, then we should shift all missiles to 
											// the right of it one to the left to fill in the gap
											if (i < num_active_missiles-1)
											{
												for (j = i; j < num_active_missiles-1; j++) alien_missiles[j] = alien_missiles[j+1];
												alien_missiles[j].is_alive = false;			// At this point, j = num_active_missiles - 1
											}
											else alien_missiles[i].is_alive = false;
											
											num_active_missiles--;
										}
									}
								}
							}
							
							// Update all the aliens once per update_period
							dont_move_aliens = (dont_move_aliens + 1) % update_period;
							if (switch_direction)
							{
								// Switch direction, move down, and knock down the switch_direction control signal
								moving_right = !moving_right;
								move_down = true;
								switch_direction = false;
							}
							// Iterate through all alive aliens
							for (i = 0; i < NUM_COLUMN_ALIENS; i++)
							{
								for (j = 0; j < NUM_ROW_ALIENS; j++)
								{
									if (alien_array[i][j].is_alive)
									{
										// Tank missile to alien collision detection
										if ((tank_missile.y_position >= alien_array[i][j].y_position) && 
												(tank_missile.y_position <= alien_array[i][j].y_position + alienHeightPixels) && 
												(tank_missile.x_position >= alien_array[i][j].x_position) && 
												(tank_missile.x_position <= alien_array[i][j].x_position + alienWidthPixels) && 
												tank_missile.is_alive) kill_alien(i, j, true);
										// Tank to alien collision detection. Don't use adjusted height because if an alien hits 
										// the cannon, then the tank is dead
										else if ((tankHeightPixels >= alien_array[i][j].y_position) && 
												(tank.position + tankWidthPixels >= alien_array[i][j].x_position) && 
												(tank.position <= alien_array[i][j].x_position + alienWidthPixels))
										{
											// Create explosion sound, draw black over tank and alien, draw explosion over tank, 
											// wait a while, draw black over explosion, stop making sound, kill tank, and break out 
											// of inner while loop
											if (sound) enable_pwm(EXPLOSION_FREQUENCY);
											lcd_draw_image(alien_array[i][j].x_position, alienWidthPixels, alien_array[i][j].y_position, 
													alienHeightPixels, alienBitmaps, LCD_COLOR_BLACK, LCD_COLOR_BLACK);
											lcd_draw_image(tank.position, tankWidthPixels, 0, tankHeightPixels, tankBitmaps, 
													LCD_COLOR_BLACK, LCD_COLOR_BLACK);
											// Add 4 to center explosion image over dead tank
											lcd_draw_image(tank.position+4, tank_explosionWidthPixels, 0, tank_explosionHeightPixels, 
													tank_explosionBitmaps, LCD_COLOR_RED, LCD_COLOR_BLACK);
											gp_timer_wait(TIMER2_BASE, DEATH_DELAY_SECONDS*ONE_SECOND);
											lcd_draw_image(tank.position+4, tank_explosionWidthPixels, 0, tank_explosionHeightPixels, 
													tank_explosionBitmaps, LCD_COLOR_BLACK, LCD_COLOR_BLACK);
											disable_pwm();
											num_alive = 0;		// Set this to 0 to break out of inner while loop
											tank.is_alive = false;
										}
										// No collision has occurred, so draw alien at next position if it's time to move them
										else if (!dont_move_aliens)
										{
											// Draw black where the alien originally was
											lcd_draw_image(alien_array[i][j].x_position, alienWidthPixels, alien_array[i][j].y_position, 
													alienHeightPixels, alienBitmaps, LCD_COLOR_BLACK, LCD_COLOR_BLACK);
											
											if (move_down)
											{
												alien_array[i][j].y_position -= alienHeightPixels;
												deassert_move_down = true;
											}
											
											// When moving right or left, if an alien appears to be beyond the edge of the screen, assert 
											// switch_direction so that the next time all aliens move, they'll switch direction and move down
											else if (moving_right)
											{
												alien_array[i][j].x_position--;
												if (alien_array[i][j].x_position <= 0) switch_direction = true;
											}
											else
											{
												alien_array[i][j].x_position++;
												if (alien_array[i][j].x_position >= (SCREEN_WIDTH - alienWidthPixels)) switch_direction = true;
											}
											
											// Draw the alien at its new position
											lcd_draw_image(alien_array[i][j].x_position, alienWidthPixels, alien_array[i][j].y_position, 
													alienHeightPixels, alienBitmaps, alien_array[i][j].color, LCD_COLOR_BLACK);
										}
									}
								}
							}
							// Deassert both move_down related variables
							if (deassert_move_down)
							{
								move_down = false;
								deassert_move_down = false;
							}
							
							// Knock down ALERT_TIMER1A_UPDATE
							ALERT_TIMER1A_UPDATE = false;
						}
						
						// ADC0SS2 interrupts indicate that ADC conversion of PS2 joystick position is finished. With the joystick position, 
						// move and draw the tank
						if (ALERT_ADC0SS2_UPDATE)
						{
							// Get coordinates. Bitwise AND with 0xFFF to guarantee we capture only 12 bits
							joystick = ADC0->SSFIFO2 & MAX_ADC_VALUE;
							
							// Move the tank
							if ((joystick  > FAST_LEFT*MAX_ADC_VALUE) && (tank.position < SCREEN_WIDTH-tankWidthPixels-1)) tank.position+= 2;
							else if ((joystick > SLOW_LEFT*MAX_ADC_VALUE) && (tank.position < SCREEN_WIDTH-tankWidthPixels)) tank.position++;
							if ((joystick < FAST_RIGHT*MAX_ADC_VALUE) && (tank.position > 1)) tank.position -= 2;
							else if ((joystick < SLOW_RIGHT*MAX_ADC_VALUE) && (tank.position > 0)) tank.position--;
							
							lcd_draw_image(tank.position, tankWidthPixels, 0, tankHeightPixels, tankBitmaps, LCD_COLOR_GREEN, LCD_COLOR_BLACK);
							
							// Knock down ALERT_ADC0SS2_UPDATE
							ALERT_ADC0SS2_UPDATE = false;
						}
					}
					
					// At this point, num_alive is 0 either because all aliens are dead or the tank has died. Check if 
					// the tank is alive to determine which has occurred
					if (tank.is_alive)
					{
						// Print the win message, wait a while, increase initial depth of aliens (saturate to MAX_DEPTH if 
						// needed), increment round variable, decrease alien firing period, and based on the next round number, 
						// add nuke(s)
						lcd_print_stringXY(win_msg, 3, 5, LCD_COLOR_BLUE, LCD_COLOR_WHITE);
						gp_timer_wait(TIMER2_BASE, WIN_DELAY_SECONDS*ONE_SECOND);
						init_depth++;
						if (init_depth > MAX_DEPTH) init_depth = MAX_DEPTH;
						round++;
						alien_firing_period -= DELTA_ALIEN_FIRING_PERIOD;
						if (alien_firing_period < MIN_ALIEN_FIRING_PERIOD) alien_firing_period = MIN_ALIEN_FIRING_PERIOD;
						if ((round%MINI_NUKE_ADD_RATE == 0) && (num_mini_nukes < MAX_MINI_NUKES)) num_mini_nukes++;
						if ((round%NUKE_ADD_RATE == 0) && (num_nukes < MAX_NUKES)) num_nukes++;
						if ((round%MEGATON_ADD_RATE == 0) && (num_megatons < MAX_MEGATONS)) num_megatons++;
						// Draw over the win message and the tank with black to clear their artifacts from the screen
						lcd_print_stringXY(win_msg, 3, 5, LCD_COLOR_BLACK, LCD_COLOR_BLACK);
						lcd_draw_image(tank.position, tankWidthPixels, 0, tankHeightPixels, tankBitmaps, LCD_COLOR_BLACK, LCD_COLOR_BLACK);
					}
					
					// Remove all active missiles before ending the round
					for (i = 0; i < num_active_missiles; i++)
					{
						lcd_draw_image(alien_missiles[i].x_position, missileWidthPixels, alien_missiles[i].y_position, 
								missileHeightPixels, missileBitmaps, LCD_COLOR_BLACK, LCD_COLOR_BLACK);
						alien_missiles[i].is_alive = false;
					}
					num_active_missiles = 0;
					lcd_draw_image(tank_missile.x_position, missileWidthPixels, tank_missile.y_position, missileHeightPixels, 
							missileBitmaps, LCD_COLOR_BLACK, LCD_COLOR_BLACK);
					tank_missile.is_alive = false;
				}
				
				// At this point, the tank is dead. Draw the lose message and leave it on the screen for a while
				lcd_print_stringXY(lose_msg, 2, 5, LCD_COLOR_RED, LCD_COLOR_WHITE);
				gp_timer_wait(TIMER2_BASE, LOSE_DELAY_SECONDS*ONE_SECOND);
				
				// Get screen ready to display the main menu. Create three random colors, clear screen gray, and switch to main menu
				rand_color1 = rand_color(LCD_COLOR_BLACK, LCD_COLOR_BLACK);
				rand_color2 = rand_color(rand_color1, LCD_COLOR_BLACK);
				rand_color3 = rand_color(rand_color1, rand_color2);
				lcd_clear_screen(LCD_COLOR_GRAY);
				mode = MAIN_MENU;
				break;
			}
		}
  }
}
