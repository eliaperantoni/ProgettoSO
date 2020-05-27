#include <stdio.h>
#include <sys/msg.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>

#include "msg.h"
#include "ack.h"

// Used to sort acks by timestamp
static int comparator(const void *a, const void *b) {
    return ((ack *) a)->timestamp - ((ack *) b)->timestamp;
}

static void fatal(char* msg) {
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: client <msg_queue_key>\n");
        return 1;
    }

    int queue_id = msgget(atoi(argv[1]), S_IRUSR | S_IWUSR);
    if(queue_id == -1) fatal("[CLIENT] Opening feedback queue");

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

    if(send_msg(&msg) == -1) fatal("[CLIENT] Sending message to device FIFO");

    feedback feedback;
    if(msgrcv(queue_id, &feedback, sizeof(feedback) - sizeof(long), msg.id, 0) == -1)
        fatal("[CLIENT] Receiving feedback from ACK manager");

    qsort(feedback.acks, DEV_COUNT, sizeof(ack), comparator);

    char output_path[256];
    sprintf(output_path, "out_%d.txt", msg.id);

    int fd = open(output_path, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);
    if(fd == -1) fatal("[CLIENT] Opening output file");

    char header[512];
    int char_count = sprintf(header, "Messaggio %d: %s\nLista acknowledgement:\n", msg.id, msg.content);

    write(fd, header, char_count);

    for (int i = 0; i < DEV_COUNT; i++) {
        ack *ack_ptr = feedback.acks + i;

        char formatted_time[64];
        strftime(formatted_time, 64, "%Y-%M-%d %H:%m:%S", localtime(&ack_ptr->timestamp));

        char row[256];
        char_count = sprintf(row, "%d, %d, %s\n", ack_ptr->pid_sender, ack_ptr->pid_receiver, formatted_time);

        if(write(fd, row, char_count) < char_count) fatal("[CLIENT] Writing to output file");
    }

    if(close(fd) == -1) fatal("[CLIENT] Closing output file");

    return 0;
}