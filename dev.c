#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <signal.h>

#include "dev.h"
#include "steps.h"
#include "msg.h"
#include "ack.h"

char fifo_path[64];
int fifo_fd;

int init_fifo(pid_t pid) {
    sprintf(fifo_path, "/tmp/dev_fifo.%d", pid);
    if (mkfifo(fifo_path, S_IRUSR | S_IWUSR) == -1) return -1;

    fifo_fd = open(fifo_path, O_RDONLY | O_NONBLOCK);
    if (fifo_fd == -1) return -1;

    return 0;
}

int teardown_fifo() {
    if (fifo_fd != 0 && fifo_fd != -1) {
        if (close(fifo_fd) == -1) return -1;
        if (unlink(fifo_path) == -1) return -1;
    }

    return 0;
}

// Process ID of the device
pid_t pid;
// Position on board of the device
pos_t current_pos;
// Head for the list of messages that this device contains.
// `messages->next` will point to the field inside the first `msg` that this device contains
list_handle_t messages = null_list_handle;

void print_status() {
    char msg_ids[256] = "";
    list_handle_t *iter;
    list_for_each(iter, &messages) {
        char msg_id[8];
        sprintf(msg_id, "%d, ", list_entry(iter, msg, list_handle)->id);
        strcat(msg_ids, msg_id);
    }

    // Remove trailing `, ` if message id list is not empty
    if (msg_ids[0] != '\0') {
        msg_ids[strlen(msg_ids) - 2] = '\0';
    }

    printf("%d %d %d msgs: %s\n", pid, current_pos.x, current_pos.y, msg_ids);
}

static void sighandler(int sig) {
    if(teardown_fifo() == -1)
        perror("[DEVICE] Could not teardown FIFO");

    list_handle_t *iter;
    list_for_each(iter, &messages) {
        msg *m = list_entry(iter, msg, list_handle);
        free(m);
    }

    exit(0);
}

static void fatal(char *msg) {
    perror(msg);
    kill(getppid(), SIGTERM);
    exit(1);
}

_Noreturn void device_loop(int dev_i) {
    if (signal(SIGTERM, sighandler) == SIG_ERR) fatal("[DEVICE] Setting signal handler");

    pid = getpid();
    if (init_fifo(pid) == -1) fatal("[DEVICE] Creating FIFO");

    while (true) {
        // First scan:
        //  1) Send messages
        //  2) Receive messages
        //  3) Move
        if (await_turn(dev_i) == -1) fatal("[DEVICE] Awaiting turn, 1st scan");

        // 1) Send messages
        for (int x = 0; x < BOARD_COLS; x++) {
            for (int y = 0; y < BOARD_ROWS; y++) {
                pos_t pos = {
                        .x = x,
                        .y = y,
                };

                pid_t target_pid;
                // If no device there, skip cell
                if ((target_pid = board_get(pos)) == 0 || target_pid == pid) continue;

                // There is someone there, compute distance
                double dist = sqrt(pow(current_pos.x - x, 2) + pow(current_pos.y - y, 2));

                list_handle_t *iter;
                list_for_each(iter, &messages) {
                    msg *m = list_entry(iter, msg, list_handle);

                    if (lock_ack_table() == -1) fatal("[DEVICE] Locking ACK table");
                    bool should_send = dist <= m->max_dist && !has_dev_received_msg(target_pid, m->id);
                    if (unlock_ack_table() == -1) fatal("[DEVICE] Unlocking ACK table");

                    if (should_send) {
                        m->pid_sender = pid;
                        m->pid_receiver = target_pid;
                        if (send_msg(m) == -1) fatal("[DEVICE] Broadcasting message");
                    }
                }
            }
        }

        // Remove all messages because we have already sent them
        list_handle_t *iter;
        list_for_each(iter, &messages) {
            msg *m = list_entry(iter, msg, list_handle);
            free(m);
        }
        messages.next = NULL;

        // 2) Receive messages
        // Keep reading messages but only alloc on heap if a message exists.
        read_loop:
        while (1) {
            // Use stack allocated message as buffer.
            msg msg_temp;
            // Remember that `fifo_fd` is open in non-blocking mode. Reads that happend concurrently with a write
            // should set `errno` to `EWOULDBLOCK' or `EAGAIN` but we gangster and we don't care.
            int br = read(fifo_fd, &msg_temp, sizeof(msg));
            if (br < sizeof(msg)) break;

            // Skip message if already have a copy of it.
            // This may happen if multiple devices send the same message to us because they work in previous turns
            // and we have to way to add an ACK in the meantime.
            list_for_each(iter, &messages) {
                msg *m = list_entry(iter, msg, list_handle);
                // We need to use goto since continue will just skip a message.
                // Remember `list_for_each` is a loop in itself.
                if (m->id == msg_temp.id) goto read_loop;
            }

            // Malloc a message
            msg *msg_ptr = malloc(sizeof(msg));
            if (msg_ptr == NULL) fatal("[DEVICE] Allocating memory for message");

            // Copy the struct from the stack to the heap
            memcpy(msg_ptr, &msg_temp, sizeof(msg));
            // Make sure list handle is not dirty
            msg_ptr->list_handle = (list_handle_t) null_list_handle;
            list_insert_after(&messages, &msg_ptr->list_handle);

            if (lock_ack_table() == -1) fatal("[DEVICE] Locking ACK table");
            int add_ack_res = add_ack(msg_ptr);
            if (unlock_ack_table() == -1) fatal("[DEVICE] Unlocking ACK table");

            if (add_ack_res == -1) {
                printf("[DEVICE] ACK table is full!");
                kill(getppid(), SIGTERM);
                exit(1);
            }
        }

        // 3) Move
        if (current_step > 0) {
            // Remove old position
            board_set(current_pos, 0);
        }

        // Set new position
        current_pos = steps_mem_ptr[current_step][dev_i];
        board_set(current_pos, pid);

        if (pass_turn(dev_i) == -1) fatal("[DEVICE] Passing turn, 1st scan");

        // Second scan, print status and remove messages
        if (await_turn(dev_i) == -1) fatal("[DEVICE] Awaiting turn, 2nd scan");
        print_status();
        if (pass_turn(dev_i) == -1) fatal("[DEVICE] Passing turn, 2nd scan");

        current_step++;
    }
}
