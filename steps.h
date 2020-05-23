#pragma once

#include "settings.h"
#include "board.h"

// A step is an array of DEV_COUNT positions
typedef pos_t step[DEV_COUNT];
// How many steps are there
extern long steps_count;
// Steps is a pointer to a block of memory that contains all the steps to be taken by the devices.
// Access it like `steps[step_index][device_index]`
extern step* steps;

void init_steps(const char* path);
void teardown_steps();

void init_mov_semaphores();
void teardown_mov_semaphores();

extern int current_step;

#define in_turn(dev_i, code) await_turn(dev_i); {code} pass_turn(dev_i)

void await_turn(int dev_i);
void pass_turn(int dev_i);

void perform_step();
