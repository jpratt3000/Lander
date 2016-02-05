/* rocket.h - Rocket Lander Game */

/*
 * Copyright (c) 2016, Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


// General Enablement

#define IO_BUTTON_BRINGUP	true	// init state to display I/O button values

#define IO_BUTTONS_ENABLE	true	// enable the buttons to change state
#define IO_JOYSTICK_ENABLE	true	// enable the joystick for X-Y
#define IO_LCD_ENABLE 		true	// enable the LCD display
#define IO_TRACKER_ENABLE	false	// enable the Pan&Tilt 'antenae' device
#define IO_LEDRGB_ENABLE	false	// enable the LED_RGB 'antenae' device
#define IO_XYZ_ENABLE		false	// enable the XYZ motors
#define IO_LEDS_ENABLE		false	// enable the LED space lighting
#define IO_SOUND_ENABLE		false	// enable the sound effects device
#define IO_BLUETOOTH_ENABLE	false	// enable the Arduino 101 rocket piggyback device


// Specific installed hardware
#define IO_GROVE_JOYSTICK_ENABLE true
#define IO_GROVE_JOYSTICK_TOGGLE true
#define IO_ADAFRUIT_JOYSTICK_ENABLE false

// Time Controls

/* specify main loop delay (in ms); compute equivalent in ticks */
/* Assume loop of 1/5 seconds */
#define SLEEPTIME  200
#define SLEEPTICKS (SLEEPTIME * sys_clock_ticks_per_sec / 1000)
#define TIME_LOOPS_PER_SECOND	5

/* Specific loop count timeouts per action */
#define XYZ_CONTROL_COUNT		1
#define TRACK_CONTROL_COUNT		4
#define POSITION_CONTROL_COUNT	1

// Game Options

#define GAME_Z_LAND			1
#define GAME_XYZ_LAND		2
#define GAME_XYZ_FLIGHT		3
#define GAME_XYZ_AUTO		4
#define GAME_XYZ_MOVE		5
#define GAME_COME_HITHER	9

#define GAME_FUEL_NORMAL	1
#define GAME_FUEL_LOW		2
#define GAME_FUEL_NOLIMIT	3

#define GAME_GRAVITY_NORMAL		1
#define GAME_GRAVITY_HIGH		2
#define GAME_GRAVITY_NONE		3
#define GAME_GRAVITY_NEGATIVE	4

#define GAME_START_CENTER	1
#define GAME_START_RANDOM	2

#define GAME_DISPLAY_RAW    1
#define GAME_DISPLAY_NORMAL	2

/* LCD Data */
#define LCD_DISPLAY_POS_MAX 16

/* Joystick Data */
#ifdef IO_GROVE_JOYSTICK_ENABLE

#define JOYSTICK_X_PORT 0
#define JOYSTICK_Y_PORT 1
#define JOYSTICK_Z_PORT 2

#define JOYSTICK_X_MIN 200
#define JOYSTICK_X_MID 413
#define JOYSTICK_X_MAX 650
#define JOYSTICK_Y_MIN 200
#define JOYSTICK_Y_MID 413
#define JOYSTICK_Y_MAX 650
#define JOYSTICK_Z_MIN 0
#define JOYSTICK_Z_MID 422
#define JOYSTICK_Z_MAX 1023

#define JOYSTICK_DELTA_XY_MIN 40	// no move at center value zone
#define JOYSTICK_DELTA_Z_MIN  40	// no move at center value zone
#endif

/* ADC Setup */
#define ADC_MAX JOYSTICK_Z_PORT+1

// Exported Structures and Funtions

struct ROCKET_GAME_S {
	int32_t	game;				// selected game
	int32_t	fuel_option;		// selected fuel option
	int32_t	gravity_option;		// selected gravity option
	int32_t	start_option;		// selected start option
	int32_t	play_display_mode;	// selected play display format
};

struct ROCKET_CONTROL_S {
	int32_t	button_a;		// button inputs
	int32_t	button_b;

	bool button_a_prev;		// Previous value for toggle detection
    bool button_b_prev;

	int32_t	analog_x;		// analog inputs
	int32_t	analog_y;
	int32_t	analog_z;

	uint8_t	lcd_line0[20];	// LCD display
	uint8_t	lcd_line1[20];

};

#if defined(CONFIG_STDOUT_CONSOLE)
#include <stdio.h>
#define PRINT           printf
#else
#include <misc/printk.h>
#define PRINT           printk
#endif

extern struct ROCKET_GAME_S r_game;
extern struct ROCKET_CONTROL_S r_control;
extern bool verbose;
extern bool self_test;

extern struct device *i2c;

extern void log(char *message);
extern void log_val(char *format, void *val);
extern int32_t abs(int32_t val);

void init_game();
void init_main();
