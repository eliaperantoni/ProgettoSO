#include "board.h"

#include <string.h>
#include <sys/shm.h>
#include <sys/stat.h>

#define BOARD_ROWS 10
#define BOARD_COLS 10
#define BOARD_BYTES sizeof(int) * BOARD_ROWS * BOARD_COLS

static int key = 0;
static pid_t *ptr = NULL;

void init_board() {
    key = shmget(IPC_PRIVATE, BOARD_BYTES, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    ptr = shmat(key, NULL, 0);
    memset(ptr, 0, BOARD_BYTES);
}

void teardown_board() {
    shmdt(ptr);
    shmctl(key, IPC_RMID, NULL);
    ptr = NULL;
    key = 0;
}

pid_t board_get(const pos_t p) {
    return ptr[p.x + BOARD_COLS * p.y];
}

void board_set(const pos_t p, const pid_t val) {
    ptr[p.x + BOARD_COLS * p.y] = val;
}


