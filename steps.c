#include "steps.h"

#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/sem.h>

#define STEP_ROW_BYTES (DEV_COUNT * 3 + (DEV_COUNT - 1))
#define STEP_ROW_EOL "\n"

long steps_count;
step *steps;

int init_steps(char *path) {
    int fd = open(path, S_IRUSR);
    if(fd == -1) return -1;

    struct stat f_stat;
    if(fstat(fd, &f_stat) == -1) return -1;

    steps_count = f_stat.st_size / (long) (STEP_ROW_BYTES + sizeof(STEP_ROW_EOL) - 1);

   steps = malloc(sizeof(step) * steps_count);
   if(steps == NULL) return -1;

    char buf[STEP_ROW_BYTES];
    for (int step_i = 0; step_i < steps_count; step_i++) {
        if(read(fd, buf, STEP_ROW_BYTES) < STEP_ROW_BYTES) return -1;
        if(lseek(fd, 1, SEEK_CUR) == -1) return -1;

        for (int dev_i = 0; dev_i < DEV_COUNT; dev_i++) {
            steps[step_i][dev_i].x = buf[dev_i * 4] - '0';
            steps[step_i][dev_i].y = buf[dev_i * 4 + 2] - '0';
        }
    }

    if(close(fd) == -1) return -1;

    return 0;
}

void teardown_steps() {
    free(steps);
}

static int id = 0;

int init_mov_semaphores() {
    id = semget(IPC_PRIVATE, DEV_COUNT + 1, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    if(id == -1) return -1;

    unsigned char *initializer[DEV_COUNT + 1] = {0};
    if(semctl(id, 0, SETALL, initializer) == -1) return -1;

    return 0;
}

int teardown_mov_semaphores() {
    if(semctl(id, 0, IPC_RMID) == -1) return -1;
    return 0;
}

int current_step = 0;

int await_turn(int dev_i) {
    struct sembuf op = {.sem_num = dev_i, .sem_op = -1};
    if(semop(id, &op, 1) == -1) return -1;
    return 0;
}

int pass_turn(int dev_i) {
    struct sembuf op = {.sem_num = dev_i + 1, .sem_op = +1};
    if(semop(id, &op, 1) == -1) return -1;
    return 0;
}

int perform_step() {
    for (int i = 0; i < 2; i++) {
        struct sembuf op = {.sem_num = 0, .sem_op = +1};
        if(semop(id, &op, 1) == -1) return -1;

        op.sem_num = DEV_COUNT;
        op.sem_op = -1;
        if(semop(id, &op, 1) == -1) return -1;
    }

    current_step++;
    return 0;
}
