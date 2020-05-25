#include <string.h>
#include <stdio.h>
#include <fcntl.h>

#include "msg.h"

void send_msg(int id, char *content, float max_dist, pid_t dst_pid) {
    msg msg = {
            .id = id,
            .max_dist = max_dist,
            .pid_sender = getpid(),
            .pid_receiver = dst_pid,
    };
    strcpy(msg.content, content);

    char fifo_path[64];
    sprintf(fifo_path, "/tmp/dev_fifo.%d", dst_pid);

    int fifo_fd = open(fifo_path, O_WRONLY);

    write(fifo_fd, &msg, sizeof(msg));

    close(fifo_fd);
}
