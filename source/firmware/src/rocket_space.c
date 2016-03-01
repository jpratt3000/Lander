/* rocket_space.c - Rocket Lander Game */

/* <legal-notice>
 *
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * This software has been developed and/or maintained under the Wind River 
 * CodeSwap program. The right to copy, distribute, modify, or otherwise 
 * make use of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 * 
 * <credits>
 *   { David Reyna,  david.reyna@windriver.com,  },
 * </credits>
 *
 * </legal-notice>
 */

#include <zephyr.h>

#include <i2c.h>

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "rocket.h"
#include "rocket_space.h"


/* 
 * forward declarations
 *
 */
 
void rocket_position_send();
void rocket_command_send(uint8_t command);
static void move_rocket_position();
static void set_rocket_position();

/*
 * Rocket Control Structures
 *
 */

struct ROCKET_SPACE_S r_space;


struct ROCKET_TOWER_S r_towers[ROCKET_TOWER_MAX] = {
    {
        .pos_x       = X_POS_MIN,
        .pos_y       = Y_POS_MAX,
        .pos_z       = Z_POS_MAX,
        .mount_pos_x = ROCKET_MOUNT_X_POS_MIN,
	    .mount_pos_y = ROCKET_MOUNT_Y_POS_MAX,
	    .mount_pos_z = ROCKET_MOUNT_Z_POS_MAX,
        .i2c_address = ROCKET_TOWER_NW_ADDR,
        .speed = MOTOR_SPEED_AUTO
    },
    {
        .pos_x       = X_POS_MAX,
        .pos_y       = Y_POS_MAX,
        .pos_z       = Z_POS_MAX,
        .mount_pos_x = ROCKET_MOUNT_X_POS_MAX,
	    .mount_pos_y = ROCKET_MOUNT_Y_POS_MAX,
	    .mount_pos_z = ROCKET_MOUNT_Z_POS_MAX,
        .i2c_address = ROCKET_TOWER_NE_ADDR,
        .speed = MOTOR_SPEED_AUTO
    },
    {
        .pos_x       = X_POS_MIN,
        .pos_y       = Y_POS_MIN,
        .pos_z       = Z_POS_MAX,
        .mount_pos_x = ROCKET_MOUNT_X_POS_MIN,
	    .mount_pos_y = ROCKET_MOUNT_Y_POS_MIN,
	    .mount_pos_z = ROCKET_MOUNT_Z_POS_MAX,
        .i2c_address = ROCKET_TOWER_SW_ADDR,
        .speed = MOTOR_SPEED_AUTO
    },
    {
        .pos_x       = X_POS_MAX,
        .pos_y       = Y_POS_MIN,
        .pos_z       = Z_POS_MAX,
        .mount_pos_x = ROCKET_MOUNT_X_POS_MAX,
	    .mount_pos_y = ROCKET_MOUNT_Y_POS_MIN,
	    .mount_pos_z = ROCKET_MOUNT_Z_POS_MAX,
        .i2c_address = ROCKET_TOWER_SE_ADDR,
        .speed = MOTOR_SPEED_AUTO
    },
};



/*
 * Initialize Rocket Hardware
 *
 */

bool init_rocket_hardware () {

	// TODO ################ Calibrate true rocket power-up position
	r_space.rocket_x = 0;			// current game-space rocket position, in uMeters
	r_space.rocket_y = 0;
	r_space.rocket_z = 0;

	// Initialize XYZ motor controls
	if (IO_MOTOR_ENABLE) {
		// TODO ################
	}

	return true;
}


/*
 * Initialize Rocket Settings
 *
 */

void init_rocket_game (int32_t pos_x, int32_t pos_y, int32_t pos_z, int32_t fuel, int32_t gravity, int32_t mode)
 {

	// set initial rocket conditions

	// TODO ################### Adjust
	r_game.fuel_option = fuel;
	if        (r_game.fuel_option == GAME_FUEL_LOW) {
		r_space.rocket_fuel=FUEL_SUPPLY_INIT/2;
	} else if (r_game.fuel_option == GAME_FUEL_NOLIMIT) {
		r_space.rocket_fuel=FUEL_SUPPLY_INIT*10;
	} else {
		r_space.rocket_fuel=FUEL_SUPPLY_INIT;
	}

	// TODO ###################
	r_game.gravity_option=gravity;
	if        (r_game.gravity_option == GAME_GRAVITY_NORMAL) {
	} else if (r_game.gravity_option == GAME_GRAVITY_HIGH) {
	} else if (r_game.gravity_option == GAME_GRAVITY_NONE) {
	} else if (r_game.gravity_option == GAME_GRAVITY_NEGATIVE) {
	}

	r_space.rocket_goal_x = pos_x;		// goal game-space rocket position, in uMeters
	r_space.rocket_goal_y = pos_y;
	r_space.rocket_goal_z = pos_z;

	if (GAME_AT_HOME == mode) {
		r_space.rocket_x = pos_x;		// preset current game-space rocket position, in uMeters
		r_space.rocket_y = pos_y;
		r_space.rocket_z = pos_z;
	}

	r_space.rocket_delta_x = 0;		// current game-space rocket speed, in uMeters
	r_space.rocket_delta_y = 0;
	r_space.rocket_delta_z = 0;

	r_space.thrust_x = 0;			// current thruster value, in fuel units
	r_space.thrust_y = 0;
	r_space.thrust_z = 0;

	r_game.game_mode = mode;

	// move to the rocket start position
	compute_rocket_next_position();
	compute_rocket_cable_lengths();
	
	/* use set_rocket_position for motor test bring up */
	if ((true || (GAME_AT_HOME == mode)) && (GAME_GO_HOME != mode)) {
		// TODO ################
		set_rocket_position();
	}
	if (false || (GAME_GO_HOME == mode)) {
		// Move Rocket and wait until rocket is in position
		move_rocket_position();
		// TODO ################
	}
}


/*
 * compute_next_position : use vectors to compute next incremental position for rocket
 *
 */

void compute_rocket_next_position ()
 {
	int32_t	rocket_fuel_used=0;
	int32_t	rocket_thrust_inc_x=THRUST_UMETER_INC_X;
	int32_t	rocket_thrust_inc_y=THRUST_UMETER_INC_Y;
	int32_t	rocket_thrust_inc_z=THRUST_UMETER_INC_Z;

	r_space.thrust_x=0;
	r_space.thrust_y=0;
	r_space.thrust_z=0;

	if (GAME_XYZ_MOVE == r_game.game) {
		// fast absolute xy changes in 'move' mode
		rocket_thrust_inc_x = 1000;
		rocket_thrust_inc_y = 1000;
	}

	// Convert joystick to thrust values
	if (r_space.rocket_fuel > 0) {
		// Thruster X is 'on-left or 'on-right' or 'off'
		r_space.thrust_x = r_control.analog_x - JOYSTICK_X_MID;
		if (r_space.thrust_x < -JOYSTICK_DELTA_XY_MIN) {
			r_space.rocket_delta_x = -rocket_thrust_inc_x;
			rocket_fuel_used += FUEL_X_INC;
		}
		if (r_space.thrust_x > JOYSTICK_DELTA_XY_MIN ) {
			r_space.rocket_delta_x = rocket_thrust_inc_x;
			rocket_fuel_used += FUEL_X_INC;
		}

		// Thruster Y is 'on-forward or 'on-backward' or 'off'
		r_space.thrust_y = r_control.analog_y - JOYSTICK_Y_MID;
		if (r_space.thrust_y < -JOYSTICK_DELTA_XY_MIN) {
			r_space.rocket_delta_y = -rocket_thrust_inc_y;
			rocket_fuel_used += FUEL_Y_INC;
		}
		if (r_space.thrust_y > JOYSTICK_DELTA_XY_MIN ) {
			r_space.rocket_delta_y = rocket_thrust_inc_y;
			rocket_fuel_used += FUEL_Y_INC;
		}

		// Thruster Z is 'proportion-up or 'proportion-down' or 'off'
		r_space.thrust_z = r_control.analog_z - JOYSTICK_Z_MID;
		if (r_space.thrust_z < -JOYSTICK_DELTA_Z_MIN) {
			r_space.rocket_delta_z += (r_space.thrust_z+JOYSTICK_DELTA_Z_MIN)*rocket_thrust_inc_z;
			rocket_fuel_used += FUEL_Z_INC;
		}
		if (r_space.thrust_z > JOYSTICK_DELTA_Z_MIN) {
			r_space.rocket_delta_z += (r_space.thrust_z-JOYSTICK_DELTA_Z_MIN)*rocket_thrust_inc_z;
			rocket_fuel_used += FUEL_Z_INC;
		}
	}

	r_space.rocket_goal_x += r_space.rocket_delta_x;
	r_space.rocket_goal_y += r_space.rocket_delta_y;
	r_space.rocket_goal_z += r_space.rocket_delta_z;

	if (GAME_XYZ_MOVE != r_game.game) {
		// Acceleration due to gravity
		if (GAME_GRAVITY_NONE != r_game.gravity_option) {
			r_space.rocket_delta_z -= GRAVITY_UMETER_PER_SECOND;
		}
	} else {
		// Cancel any inertial and gravity motion, also any fuel usage
		r_space.rocket_delta_x = 0;
		r_space.rocket_delta_y = 0;
		r_space.rocket_delta_z = 0;
	}

	// Burn that fuel
	r_space.rocket_fuel -= rocket_fuel_used;
	if ((GAME_FUEL_NOLIMIT == r_game.fuel_option) || (GAME_XYZ_MOVE == r_game.game)) {
		if (r_space.rocket_fuel < 100) r_space.rocket_fuel = FUEL_SUPPLY_INIT;
	}

	// Assert Limits
	if (GAME_SIMULATE == r_game.game_mode) {
		if (r_space.rocket_goal_x < X_POS_MIN)  r_space.rocket_goal_x = X_POS_MIN;
		if (r_space.rocket_goal_x > X_POS_MAX)  r_space.rocket_goal_x = X_POS_MAX;
		if (r_space.rocket_goal_y < Y_POS_MIN)  r_space.rocket_goal_y = Y_POS_MIN;
		if (r_space.rocket_goal_y > Y_POS_MAX)  r_space.rocket_goal_y = Y_POS_MAX;
		if (r_space.rocket_goal_z < Z_POS_MIN)  r_space.rocket_goal_z = Z_POS_MIN;
		if (r_space.rocket_goal_z > Z_POS_MAX)  r_space.rocket_goal_z = Z_POS_MAX;
	} else {
		if (r_space.rocket_goal_x < GAME_X_POS_MIN)  r_space.rocket_goal_x = GAME_X_POS_MIN;
		if (r_space.rocket_goal_x > GAME_X_POS_MAX)  r_space.rocket_goal_x = GAME_X_POS_MAX;
		if (r_space.rocket_goal_y < GAME_Y_POS_MIN)  r_space.rocket_goal_y = GAME_Y_POS_MIN;
		if (r_space.rocket_goal_y > GAME_Y_POS_MAX)  r_space.rocket_goal_y = GAME_Y_POS_MAX;
		if (r_space.rocket_goal_z < GAME_Z_POS_MIN)  r_space.rocket_goal_z = GAME_Z_POS_MIN;
		if (r_space.rocket_goal_z > GAME_Z_POS_MAX)  r_space.rocket_goal_z = GAME_Z_POS_MAX;
	}
 }

/*
 * square_root_binary_search : approximation for square root function
 *
 */

static int sqrt_cnt=0;	// loop protection counter

// Newton-Raphson method
int32_t sqrt_with_accuracy(int32_t x, int32_t init_guess)
{	int32_t next_guess;

	if (++sqrt_cnt > 20) {
		PRINT("#############SQRT_TOOMANY(%ld,%ld\n",x,init_guess);
		return init_guess;
	}
	
	// if we reach 0=init_guess, then the number and answer is zero
	if (0 == init_guess) return 0;

	next_guess = (init_guess + (x/init_guess))/2;
	if (abs(init_guess - next_guess) < 2)
		return(next_guess);
	else
		return(sqrt_with_accuracy(x, next_guess));
};

/*
 * compute_rocket_cable_lengths : compute the rocket position to cable lengths
 *
 */

static void do_compute_cable_length(int32_t tower) {
	int32_t x,y,z,length;

	sqrt_cnt=0;

	// we will use millimeters for the intermedate calculation to avoid overflow
	x=(r_space.rocket_goal_x - r_towers[tower].pos_x + r_towers[tower].mount_pos_x)/1000;
	y=(r_space.rocket_goal_y - r_towers[tower].pos_y + r_towers[tower].mount_pos_y)/1000;
	z=(r_space.rocket_goal_z - r_towers[tower].pos_z + r_towers[tower].mount_pos_z)/1000;
	r_towers[tower].length_goal = sqrt_with_accuracy((x*x)+(y*y)+(z*z),500)*1000;

	// Calculate needed cable deployment change
	length = r_towers[tower].length_goal - r_towers[tower].length;
}

void compute_rocket_cable_lengths ()
 {
 	do_compute_cable_length(ROCKET_TOWER_NW);
 	do_compute_cable_length(ROCKET_TOWER_NE);
 	do_compute_cable_length(ROCKET_TOWER_SW);
 	do_compute_cable_length(ROCKET_TOWER_SE);
 }

/*
 * set_rocket_position : preset without movement the Rocket position
 *
 */

static void set_rocket_position ()
 {
	r_towers[ROCKET_TOWER_NW].step_count = (r_towers[ROCKET_TOWER_NW].length_goal*10L) / ROCKET_TOWER_STEPS_PER_UM10;
	r_towers[ROCKET_TOWER_NE].step_count = (r_towers[ROCKET_TOWER_NW].length_goal*10L) / ROCKET_TOWER_STEPS_PER_UM10;
	r_towers[ROCKET_TOWER_SW].step_count = (r_towers[ROCKET_TOWER_NW].length_goal*10L) / ROCKET_TOWER_STEPS_PER_UM10;
	r_towers[ROCKET_TOWER_SE].step_count = (r_towers[ROCKET_TOWER_NW].length_goal*10L) / ROCKET_TOWER_STEPS_PER_UM10;

	if (IO_MOTOR_ENABLE && (GAME_SIMULATE != r_game.game_mode)) {
		rocket_position_send();
		rocket_command_send(ROCKET_MOTOR_CMD_PRESET);
	}
	
 }

/*
 * move_rocket_initial_position : move to the Rocket initial position
 *
 */

static void move_rocket_position ()
 {
	r_towers[ROCKET_TOWER_NW].step_count = (r_towers[ROCKET_TOWER_NW].length_goal*10L) / ROCKET_TOWER_STEPS_PER_UM10;
	r_towers[ROCKET_TOWER_NE].step_count = (r_towers[ROCKET_TOWER_NW].length_goal*10L) / ROCKET_TOWER_STEPS_PER_UM10;
	r_towers[ROCKET_TOWER_SW].step_count = (r_towers[ROCKET_TOWER_NW].length_goal*10L) / ROCKET_TOWER_STEPS_PER_UM10;
	r_towers[ROCKET_TOWER_SE].step_count = (r_towers[ROCKET_TOWER_NW].length_goal*10L) / ROCKET_TOWER_STEPS_PER_UM10;

	if (IO_MOTOR_ENABLE && (GAME_SIMULATE != r_game.game_mode)) {
		rocket_position_send();
		rocket_command_send(ROCKET_MOTOR_CMD_DEST);
	}
 }

/*
 * move_rocket_next_position : incrementally move the Rocket position
 *
 */

static void do_move_tower(struct ROCKET_TOWER_S *tower) {
	int32_t step_goal;
	tower->length = tower->length_goal;

	step_goal = (tower->length_goal *10L)/ROCKET_TOWER_STEPS_PER_UM10;
	tower->step_diff = step_goal - tower->step_count;
	tower->step_count = step_goal;
}

void move_rocket_next_position ()
 {
	r_space.rocket_x = r_space.rocket_goal_x;
	r_space.rocket_y = r_space.rocket_goal_y;
	r_space.rocket_z = r_space.rocket_goal_z;

	do_move_tower(&r_towers[ROCKET_TOWER_NW]);
	do_move_tower(&r_towers[ROCKET_TOWER_NE]);
	do_move_tower(&r_towers[ROCKET_TOWER_SW]);
	do_move_tower(&r_towers[ROCKET_TOWER_SE]);

	if (IO_MOTOR_ENABLE && (GAME_SIMULATE != r_game.game_mode)) {
		if (r_towers[ROCKET_TOWER_NW].step_diff ||
		    r_towers[ROCKET_TOWER_NE].step_diff ||
		    r_towers[ROCKET_TOWER_SW].step_diff ||
		    r_towers[ROCKET_TOWER_SE].step_diff) {
			// there is movement for the rocket
			rocket_increment_send(
				r_towers[ROCKET_TOWER_NW].step_diff,
				r_towers[ROCKET_TOWER_NE].step_diff,
				r_towers[ROCKET_TOWER_SW].step_diff,
				r_towers[ROCKET_TOWER_SE].step_diff);
		    }
	}

 }

/*
 * query_rocket_progress : query and return progress of rocket motion (in percent)
 *
 */

uint8_t query_rocket_progress ()
 {
	uint8_t buf[10];
	uint32_t len = 1;
	
	if (IO_MOTOR_ENABLE) {
		buf[0] = (uint8_t) 101;
		i2c_read(i2c,buf,len,ROCKET_MOTOR_I2C_ADDRESS);
	} else {
		if ((r_space.rocket_x == r_space.rocket_goal_x) &&
	    	(r_space.rocket_y == r_space.rocket_goal_y) &&
	    	(r_space.rocket_z == r_space.rocket_goal_z) ) {
			buf[0] = (uint8_t) 100;
		} else {
			buf[0] = (uint8_t) 50;
		}
	}
	return(buf[0]);
 }

/*
 * rocket_increment_send : increment a rocket motor
 *
 */

void rocket_increment_send (int32_t increment_nw, int32_t increment_ne, int32_t increment_sw, int32_t increment_se)
 {
	uint8_t buf[10];

	buf[0]=(uint8_t) ROCKET_MOTOR_CMD_NEXT;
	buf[1]=(uint8_t) ((increment_nw & 0x00ff00L) >> 8);
	buf[2]=(uint8_t) ((increment_nw & 0x0000ffL)     );
	buf[3]=(uint8_t) ((increment_ne & 0x00ff00L) >> 8);
	buf[4]=(uint8_t) ((increment_ne & 0x0000ffL)     );
	buf[5]=(uint8_t) ((increment_sw & 0x00ff00L) >> 8);
	buf[6]=(uint8_t) ((increment_sw & 0x0000ffL)     );
	buf[7]=(uint8_t) ((increment_se & 0x00ff00L) >> 8);
	buf[8]=(uint8_t) ((increment_se & 0x0000ffL)     );

	i2c_polling_write (i2c, buf, 9, ROCKET_MOTOR_I2C_ADDRESS);
 }

/*
 * rocket_position_send : send the motor positions
 *
 */

static void do_rocket_position_send (uint8_t tower_number, struct ROCKET_TOWER_S *tower)
 {
	uint8_t buf[10];
	buf[0]=(uint8_t) tower_number;
	buf[1]=(uint8_t) ((tower->step_count & 0x00ff000000L) >> 24);
	buf[2]=(uint8_t) ((tower->step_count & 0x0000ff0000L) >> 16);
	buf[3]=(uint8_t) ((tower->step_count & 0x000000ff00L) >>  8);
	buf[4]=(uint8_t) ((tower->step_count & 0x00000000ffL)      );
	i2c_polling_write (i2c, buf, 5, ROCKET_MOTOR_I2C_ADDRESS);
 }

void rocket_position_send ()
{
	do_rocket_position_send('1',&r_towers[ROCKET_TOWER_NW]);
	do_rocket_position_send('2',&r_towers[ROCKET_TOWER_NE]);
	do_rocket_position_send('3',&r_towers[ROCKET_TOWER_SW]);
	do_rocket_position_send('4',&r_towers[ROCKET_TOWER_SE]);
}

/*
 * rocket_command_send : send a motor command
 *
 */

void rocket_command_send (uint8_t command)
 {
	uint8_t buf[10];
	buf[0]=(uint8_t) command;
	i2c_polling_write (i2c, buf, 1, ROCKET_MOTOR_I2C_ADDRESS);
 }
