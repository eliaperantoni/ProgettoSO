#include "ack.h"
#include <sys/shm.h>
#include <string.h>
#include <sys/stat.h>

static int id = 0;
static ack *ptr = NULL;
static int size = 0;

void init_ack_table() {
    id = shmget(IPC_PRIVATE, ACK_TABLE_BYTES, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    ptr = shmat(id, NULL, 0);
    memset(ptr, 0, ACK_TABLE_BYTES);
}

void teardown_ack_table() {
    shmdt(ptr);
    shmctl(id, IPC_RMID, NULL);
    ptr = NULL;
    id = 0;
}

void add_ack(pid_t pid_sender, pid_t pid_receiver, int message_id) {
    ack new_ack = {
            .pid_sender = pid_sender,
            .pid_receiver = pid_receiver,
            .message_id = message_id,
            .timestamp = time(0),
    };
    ptr[size++] = new_ack;
}
