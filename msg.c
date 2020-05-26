#include <string.h>
#include <stdio.h>
#include <fcntl.h>

#include "msg.h"

void send_msg(msg* m) {
    char fifo_path[64];
    sprintf(fifo_path, "/tmp/dev_fifo.%d", m->pid_receiver);

    int fifo_fd = open(fifo_path, O_WRONLY);

    write(fifo_fd, m, sizeof(msg));

    close(fifo_fd);
}
