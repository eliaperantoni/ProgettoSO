#include <string.h>
#include <stdio.h>
#include <fcntl.h>

#include "msg.h"

int send_msg(msg* m) {
    char fifo_path[64];
    sprintf(fifo_path, "/tmp/dev_fifo.%d", m->pid_receiver);

    int fifo_fd = open(fifo_path, O_WRONLY);
    if(fifo_fd == -1) return -1;

    if(write(fifo_fd, m, sizeof(msg)) < sizeof(msg)) return -1;

    if(close(fifo_fd) == -1) return -1;
    return 0;
}
