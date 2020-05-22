#pragma once

#include <unistd.h>
#include "settings.h"

/*
          x------->
     y    _______
     |   |       |
     |   |       |
     v   |_______|

     Top left is (0,0)
     Bottom right is (BOARD_COLS-1, BOARD_ROWS-1)
*/

// Uniquely identifies a single cell in the board
typedef struct {
    int x;
    int y;
} pos_t;

void init_board();
void teardown_board();

pid_t board_get(pos_t p);
void board_set(pos_t p, pid_t val);
