#include "steps.h"

#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/sem.h>

#include "err_exit.h"

#define STEP_ROW_BYTES (DEV_COUNT * 3 + (DEV_COUNT - 1))
#define STEP_ROW_EOL "\n"

long steps_count;
step *steps;

void init_steps(char *path) {
    int fd = open(path, S_IRUSR);

    struct stat f_stat;
    fstat(fd, &f_stat);

    steps_count = f_stat.st_size / (long) (STEP_ROW_BYTES + sizeof(STEP_ROW_EOL) - 1);

    try steps = malloc(sizeof(step) * steps_count)
        catchNil("malloc steps table")

    char buf[STEP_ROW_BYTES];
    for (int step_i = 0; step_i < steps_count; step_i++) {
        read(fd, buf, STEP_ROW_BYTES);
        lseek(fd, 1, SEEK_CUR);

        for (int dev_i = 0; dev_i < DEV_COUNT; dev_i++) {
            steps[step_i][dev_i].x = buf[dev_i * 4] - '0';
            steps[step_i][dev_i].y = buf[dev_i * 4 + 2] - '0';
        }
    }

    close(fd);
}

void teardown_steps() {
    free(steps);
}

static int id = 0;

void init_mov_semaphores() {
    id = semget(IPC_PRIVATE, DEV_COUNT + 1, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    unsigned char *initializer[DEV_COUNT + 1] = {0};
    semctl(id, 0, SETALL, initializer);
}

void teardown_mov_semaphores() {
    semctl(id, 0, IPC_RMID);
}

int current_step = 0;

void await_turn(int dev_i) {
    struct sembuf op = {.sem_num = dev_i, .sem_op = -1};
    semop(id, &op, 1);
}

void pass_turn(int dev_i) {
    struct sembuf op = {.sem_num = dev_i + 1, .sem_op = +1};
    semop(id, &op, 1);
}

void perform_step() {
    for (int i = 0; i < 2; i++) {
        struct sembuf op = {.sem_num = 0, .sem_op = +1};
        semop(id, &op, 1);

        op.sem_num = DEV_COUNT;
        op.sem_op = -1;
        semop(id, &op, 1);
    }

    current_step++;
}
