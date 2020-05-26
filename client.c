#include "msg.h"
#include <stdio.h>

int main(int argc, char * argv[]) {
    pid_t dst_pid;
    printf("PID: ");
    scanf(" %d", &dst_pid);

    char fifo_path[64];
    sprintf(fifo_path, "/tmp/dev_fifo.%d", dst_pid);

    msg m = {
            .id = 1,
            .content = "Hello World!",
            .pid_sender = getpid(),
            .pid_receiver = dst_pid,
            .max_dist = 1,
            .list_handle = null_list_handle,
    };

    send_msg(&m);

    printf("SENT TO %d\n", dst_pid);

    pause();

    return 0;
}