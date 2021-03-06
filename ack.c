#include <sys/shm.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>

#include "ack.h"
#include "steps.h"

static int ack_table_shm_id;
static ack *ack_table_ptr;

static int ack_table_sem_id;

int init_ack_table() {
    ack_table_shm_id = shmget(IPC_PRIVATE, ACK_TABLE_BYTES, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    if (ack_table_shm_id == -1) return -1;

    ack_table_ptr = shmat(ack_table_shm_id, NULL, 0);
    if (ack_table_ptr == (ack *) -1) return -1;

    memset(ack_table_ptr, 0, ACK_TABLE_BYTES);

    ack_table_sem_id = semget(IPC_PRIVATE, 1, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    if (ack_table_sem_id == -1) return -1;

    if (semctl(ack_table_sem_id, 0, SETVAL, 1) == -1) return -1;

    return 0;
}

int teardown_ack_table() {
    if (ack_table_ptr != NULL && ack_table_ptr != (ack *) -1) {
        if (shmdt(ack_table_ptr) == -1) return -1;
        if (shmctl(ack_table_shm_id, IPC_RMID, NULL) == -1) return -1;
    }

    if (ack_table_sem_id != 0 && ack_table_sem_id != -1) {
        if (semctl(ack_table_sem_id, 0, IPC_RMID) == -1) return -1;
    }

    return 0;
}

static int queue_id;

int init_feedback_queue(int key) {
    queue_id = msgget(key, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    if (queue_id == -1) return -1;

    return 0;
}

int teardown_feedback_queue() {
    if (queue_id != 0 && queue_id != -1)
        if (msgctl(queue_id, IPC_RMID, NULL) == -1) return -1;

    return 0;
}

int lock_ack_table() {
    struct sembuf op = {.sem_num = 0, .sem_op = -1};
    while (true) {
        int res = semop(ack_table_sem_id, &op, 1);
        if (res != -1) return 0;
        else if (errno == EINTR) continue;
        else return -1;
    }
}

int unlock_ack_table() {
    struct sembuf op = {.sem_num = 0, .sem_op = +1};
    while (true) {
        int res = semop(ack_table_sem_id, &op, 1);
        if (res != -1) return 0;
        else if (errno == EINTR) continue;
        else return -1;
    }
}

// Global vars are initialized to 0.
// We'll use this to check if a rock is free.
ack test_ack;

static bool is_row_free(int row_i) {
    return memcmp(ack_table_ptr + row_i, &test_ack, sizeof(ack)) == 0;
}

int add_ack(msg *msg_ptr) {
    ack new_ack = {
            .pid_sender = msg_ptr->pid_sender,
            .pid_receiver = msg_ptr->pid_receiver,
            .message_id =  msg_ptr->id,
            .timestamp = time(0),
            .zombie_at = -1,
    };

    int row_i;
    for (row_i = 0; row_i < ACK_TABLE_ROWS; row_i++) {
        if (is_row_free(row_i)) {
            ack_table_ptr[row_i] = new_ack;
            break;
        }
    }

    // Did not find an empty row
    if (row_i == ACK_TABLE_ROWS) return -1;

    return 0;
}

bool has_dev_received_msg(pid_t dev_pid, int msg_id) {
    for (int row_i = 0; row_i < ACK_TABLE_ROWS; row_i++) {
        ack *ack = ack_table_ptr + row_i;
        if (ack->pid_receiver == dev_pid && ack->message_id == msg_id) return true;
    }

    return false;
}

static int comparator(const void *a, const void *b) {
    return ((ack *) b)->message_id - ((ack *) a)->message_id;
}

void display_ack_table() {
    printf("=== ACK TABLE =================\n");
    for (int row_i = 0; row_i < ACK_TABLE_ROWS; row_i++) {
        printf("%d ", row_i);
        if (is_row_free(row_i)) {
            printf("EMPTY\n");
        } else {
            ack *ack = ack_table_ptr + row_i;
            printf("%d %d %s\n", ack->pid_receiver, ack->message_id, ack->zombie_at != -1 ? "ZOMBIE" : "");
        }
    }
    printf("===============================\n\n");
}

static void fatal(char *msg) {
    perror(msg);
    kill(getppid(), SIGTERM);
    exit(1);
}

void sighandler(int sig) {
    if (sig == SIGUSR1)
        current_step++;
    else if (sig == SIGTERM) {
        if (teardown_feedback_queue() == -1)
            perror("[ACK MANAGER] Could not teardown feedback queue");

        exit(0);
    }
}

_Noreturn void ack_manager_loop(int msg_queue_key) {
    if (signal(SIGTERM, sighandler) == SIG_ERR)
        fatal("[ACK MANAGER] Setting SIGTERM handler");

    // Unblock SIGUSR1 and set handler
    sigset_t sigset;
    if (sigemptyset(&sigset) == -1)
        fatal("[ACK MANAGER] Setting empty signal set");
    if (sigaddset(&sigset, SIGUSR1) == -1)
        fatal("[ACK MANAGER] Adding SIGUSR1 signal set");
    if (sigprocmask(SIG_UNBLOCK, &sigset, NULL) == -1)
        fatal("[ACK MANAGER] Unblocking from signal set");

    if (signal(SIGUSR1, sighandler) == SIG_ERR)
        fatal("[ACK MANAGER] Setting SIGUSR1 handler");

    if (init_feedback_queue(msg_queue_key) == -1)
        fatal("[ACK MANAGER] Initializing feedback queue");

    while (true) {
        unsigned int remaining = 5;
        while (remaining > 0) remaining = sleep(remaining);

        // Lock semaphore
        if (lock_ack_table() == -1)
            fatal("[ACK MANAGER] Acquiring ack table mutex");

        // Sort in descending order relative to message id.
        // This will cluster acks with the same message id together.
        // Empty rows are left last since message_id is greater than 0.
        qsort(ack_table_ptr, ACK_TABLE_ROWS, sizeof(ack), comparator);

        // `message_id` is the message id of the cluster we're at now
        int message_id = -1, streak = 0;
        // Scan rows.
        // As soon as we encounter an empty row we can stop scanning.
        for (int row_i = 0; row_i < ACK_TABLE_ROWS && !is_row_free(row_i); row_i++) {
            // Delete zombies when they are too old
            if (ack_table_ptr[row_i].zombie_at != -1 && current_step - ack_table_ptr[row_i].zombie_at >= 2) {
                memset(ack_table_ptr, 0, sizeof(ack) * DEV_COUNT);
                row_i += DEV_COUNT - 1;
                continue;
            }

            if (ack_table_ptr[row_i].message_id == message_id) {
                streak++;
                if (streak == DEV_COUNT) {
                    // Send message to client
                    feedback feedback_msg = {
                            .message_id = message_id,
                    };
                    memcpy(feedback_msg.acks, ack_table_ptr + row_i - DEV_COUNT + 1, sizeof(ack[DEV_COUNT]));
                    if (msgsnd(queue_id, &feedback_msg, sizeof(feedback) - sizeof(long), 0) == -1)
                        perror("[ACK MANAGER] WARNING Couldn't send feedback to client");

                    // Zombify all ACKs for this message
                    for (int go_back = 0; go_back < DEV_COUNT; go_back++)
                        ack_table_ptr[row_i - go_back].zombie_at = current_step;
                }
            } else {
                message_id = ack_table_ptr[row_i].message_id;
                streak = 1;
            }
        }

        // Unlock semaphore
        if (unlock_ack_table() == -1)
            fatal("[ACK MANAGER] Releasing ack table mutex");
    }
}
