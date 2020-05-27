#include <stdio.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/stat.h>

#include "board.h"

#define BOARD_BYTES sizeof(pid_t) * BOARD_ROWS * BOARD_COLS

static int id = 0;
static pid_t *ptr;

int init_board() {
    id = shmget(IPC_PRIVATE, BOARD_BYTES, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    if(id == -1) return -1;

    ptr = shmat(id, NULL, 0);
    if(ptr == (pid_t*)-1) return -1;

    memset(ptr, 0, BOARD_BYTES);

    return 0;
}

int teardown_board() {
    if(shmdt(ptr) == -1) return -1;
    if(shmctl(id, IPC_RMID, NULL) == -1) return -1;
    return 0;
}

pid_t board_get(pos_t p) {
    return ptr[p.y * BOARD_COLS + p.x];
}

void board_set(pos_t p, pid_t val) {
    ptr[p.y * BOARD_COLS + p.x] = val;
}

void display_board() {
    printf("╔");
    for (int i = 0; i < BOARD_ROWS; i++) printf("═");
    printf("╗\n");

    for (int i = 0; i < BOARD_ROWS; i++) {
        printf("║");

        for (int j = 0; j < BOARD_COLS; j++) {
            pos_t pos = {i, j};
            pid_t pid = board_get(pos);
            char *str = pid == 0 ? " " : "#";
            printf("%s", str);
        }

        printf("║\n");
    }

    printf("╚");
    for (int i = 0; i < BOARD_ROWS; i++) printf("═");
    printf("╝\n");
}
