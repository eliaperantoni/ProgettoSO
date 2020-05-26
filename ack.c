#include <sys/shm.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/sem.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "ack.h"
#include "settings.h"

static int shm_id;
static ack *ptr = NULL;

static int sem_id;

void init_ack_table() {
    shm_id = shmget(IPC_PRIVATE, ACK_TABLE_BYTES, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    ptr = shmat(shm_id, NULL, 0);
    memset(ptr, 0, ACK_TABLE_BYTES);

    sem_id = semget(IPC_PRIVATE, 1, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
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

bool ack_is_row_free(int row_i) {
    return memcmp(ptr + row_i, &test_ack, sizeof(ack)) == 0;
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

bool has_dev_received_msg(pid_t dev_pid, int msg_id) {
    for (int row_i = 0; row_i < ACK_TABLE_ROWS; row_i++) {
        ack *ack = ptr + row_i;
        if (ack->pid_receiver == dev_pid && ack->message_id == msg_id) return true;
    }
    return false;
}

int comparator(const void *a, const void *b) {
    return ((ack *) b)->message_id - ((ack *) a)->message_id;
}

void display_ack_table() {
    printf("=== ACK TABLE =================\n");
    for (int row_i = 0; row_i < ACK_TABLE_ROWS; row_i++) {
        printf("%d ", row_i);
        if (ack_is_row_free(row_i)) {
            printf("EMPTY\n");
        } else {
            ack *ack = ptr + row_i;
            printf("%d %d\n", ack->pid_receiver, ack->message_id);
        }
    }
    printf("===============================\n");
}

_Noreturn void ack_manager_loop() {
    while (true) {
        sleep(5);

        // Lock semaphore
        struct sembuf op = {.sem_num = 0, .sem_op = -1};
        semop(sem_id, &op, 1);

        // Sort in descending order relative to message id.
        // This will cluster acks with the same message id together.
        // Empty rows are left last since message_id is greater than 0.
        qsort(ptr, ACK_TABLE_ROWS, sizeof(ack), comparator);

        // `message_id` is the message id of the cluster we're at now
        int message_id = -1, streak = 0;
        // Scan rows.
        // As soon as we encounter an empty row we can stop scanning.
        for (int row_i = 0; row_i < ACK_TABLE_ROWS && !ack_is_row_free(row_i); row_i++) {
            if (ptr[row_i].message_id == message_id) {
                streak++;
                if (streak == DEV_COUNT) {
                    // TODO Send message to client
                    // Reset this row + the last 4 to empty rows
                    memset(ptr + row_i - DEV_COUNT + 1, 0, sizeof(ack) * DEV_COUNT);
                }
            } else {
                message_id = ptr[row_i].message_id;
                streak = 1;
            }
        }

        // Unlock semaphore
        op.sem_op = +1;
        semop(sem_id, &op, 1);
    }
}
