#include <stdio.h>
#include <sys/msg.h>
#include <stdlib.h>
#include <fcntl.h>

#include "msg.h"
#include "ack.h"

int main(int argc, char * argv[]) {
    if (argc != 2) {
        printf("Usage: client <msg_queue_key>\n");
        return 1;
    }

    int queue_id = msgget(atoi(argv[1]), S_IRUSR | S_IWUSR);

    msg msg = {
            .list_handle = null_list_handle,
            .pid_sender = getpid(),
    };

    printf("PID: ");
    scanf(" %d", &msg.pid_receiver);

    printf("MESSAGE ID: ");
    scanf(" %d", &msg.id);

    printf("CONTENT: ");
    scanf(" %[^\n]s", msg.content);

    printf("MAX DIST: ");
    scanf(" %lf", &msg.max_dist);

    send_msg(&msg);

    feedback feedback;
    msgrcv(queue_id, &feedback, sizeof(feedback) - sizeof(long), msg.id, 0);

    return 0;
}