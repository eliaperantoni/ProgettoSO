#pragma once

#include "settings.h"
#include "board.h"

// A step is an array of DEV_COUNT positions
typedef pos_t step[DEV_COUNT];
// How many steps are there
extern long steps_count;
// Steps is a pointer to a block of memory that contains all the steps to be taken by the devices.
// Access it like `steps[step_index][device_index]`
extern step* steps_mem_ptr;

int init_steps(char* path);
void teardown_steps();

int init_mov_semaphores();
int teardown_mov_semaphores();

extern int current_step;

#define in_turn(dev_i, code) await_turn(dev_i); {code} pass_turn(dev_i)

int await_turn(int dev_i);
int pass_turn(int dev_i);

int perform_step();
