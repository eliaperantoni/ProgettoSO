#include "steps.h"

#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "err_exit.h"

#define STEP_ROW_BYTES (DEV_COUNT * 3 + (DEV_COUNT - 1))
#define STEP_ROW_EOL "\n"

step *steps;

void init_steps(const char* path) {
    int fd = open(path, S_IRUSR);

    struct stat f_stat;
    fstat(fd, &f_stat);

    long steps_count = f_stat.st_size / (long) (STEP_ROW_BYTES + sizeof(STEP_ROW_EOL) - 1);

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
