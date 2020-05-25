#include "msg.h"
#include <stdio.h>

int main(int argc, char * argv[]) {
    pid_t dst_pid;
    printf("PID: ");
    scanf(" %d", &dst_pid);

    char fifo_path[64];
    sprintf(fifo_path, "/tmp/dev_fifo.%d", dst_pid);

    send_msg(1, "Hello World!", 1, dst_pid);
    send_msg(2, "Hello World!", 1, dst_pid);
    send_msg(3, "Hello World!", 1, dst_pid);
    send_msg(4, "Hello World!", 1, dst_pid);

    printf("SENT TO %d\n", dst_pid);

    pause();

    return 0;
}