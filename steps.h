#pragma once

#include "settings.h"
#include "board.h"

// A step is an array of DEV_COUNT positions
typedef pos_t step[DEV_COUNT];
// Steps is a pointer to a block of memory that contains all the steps to be taken by the devices.
// Access it like `steps[step_index][device_index]`
extern step* steps;

void init_steps(const char* path);
void teardown_steps();