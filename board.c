#include "board.h"

#include <stdio.h>;
#include <string.h>
#include <sys/shm.h>
#include <sys/stat.h>

#define BOARD_ROWS 10
#define BOARD_COLS 10
#define BOARD_BYTES sizeof(int) * BOARD_ROWS * BOARD_COLS

static int id = 0;
static pid_t *ptr = NULL;

void init_board() {
    id = shmget(IPC_PRIVATE, BOARD_BYTES, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    ptr = shmat(id, NULL, 0);
    memset(ptr, 0, BOARD_BYTES);
}

void teardown_board() {
    shmdt(ptr);
    shmctl(id, IPC_RMID, NULL);
    ptr = NULL;
    id = 0;
}

pid_t board_get(const pos_t p) {
    return ptr[p.x + BOARD_COLS * p.y];
}

void board_set(const pos_t p, const pid_t val) {
    ptr[p.x + BOARD_COLS * p.y] = val;
}

void display_board() {
    printf("╔");
    for (int i = 0; i < BOARD_ROWS; i++) printf("═");
    printf("╗\n");

    for (int i = 0; i < BOARD_ROWS; i++) {
        printf("║");

        for (int j = 0; j < BOARD_COLS; j++) {
            pos_t pos= {i, j};
            pid_t pid = board_get(pos);
            char* str = pid == 0 ? " " : "#";
            printf("%s", str);
        }

        printf("║\n");
    }

    printf("╚");
    for (int i = 0; i < BOARD_ROWS; i++) printf("═");
    printf("╝\n");
}
