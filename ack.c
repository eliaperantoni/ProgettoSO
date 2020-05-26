#include <sys/shm.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/sem.h>

#include "ack.h"

static int shm_id;
static ack *ptr = NULL;

static int sem_id;

void init_ack_table() {
    shm_id = shmget(IPC_PRIVATE, ACK_TABLE_BYTES, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    ptr = shmat(shm_id, NULL, 0);
    memset(ptr, 0, ACK_TABLE_BYTES);

    sem_id = semget(IPC_PRIVATE,1, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    semctl(sem_id, 0, SETVAL, 1);
}

void teardown_ack_table() {
    shmdt(ptr);
    shmctl(shm_id, IPC_RMID, NULL);

    semctl(sem_id, 0, IPC_RMID);
}

// Global vars are initialized to 0.
// We'll use this to check if a rock is free.
ack test_ack;

int ack_is_row_free(int row_i) {
    return memcmp(&ptr[row_i], &test_ack, sizeof(ack)) == 0;
}

void add_ack(msg *msg_ptr) {
    ack new_ack = {
            .pid_sender = msg_ptr->pid_sender,
            .pid_receiver = msg_ptr->pid_receiver,
            .message_id =  msg_ptr->id,
            .timestamp = time(0),
    };

    struct sembuf op = {.sem_num = 0, .sem_op = -1};
    semop(sem_id, &op, 1);

    for (int row_i = 0; row_i < ACK_TABLE_ROWS; row_i++) {
        if (ack_is_row_free(row_i)) {
            ptr[row_i] = new_ack;
            break;
        }
    }

    // TODO Handle full table

    op.sem_op = +1;
    semop(sem_id, &op, 1);
}
