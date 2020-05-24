#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>

#include "dev.h"
#include "steps.h"
#include "err_exit.h"

char fifo_path[64];
int fifo_fd;

void init_fifo(pid_t pid) {
    sprintf(fifo_path, "/tmp/dev_fifo.%d", pid);
    mkfifo(fifo_path, S_IRUSR | S_IWUSR);
    fifo_fd = open(fifo_path, O_RDONLY | O_NONBLOCK);
}

void teardown_fifo() {
    close(fifo_fd);
    unlink(fifo_path);
}

pos_t current_pos;

_Noreturn void device_loop(int dev_i) {
    pid_t pid = getpid();
    init_fifo(pid);

    while (1) {
        in_turn(dev_i, {
            printf("MOVING DEV %d TO (%d, %d)\n", dev_i, current_pos.x, current_pos.y);
            if (current_step > 0) {
                // Remove old position
                board_set(current_pos, 0);
            }

            // Set new position
            current_pos = steps[current_step][dev_i];
            board_set(current_pos, pid);
        });
    }
}
