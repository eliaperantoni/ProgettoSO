#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

#include "dev.h"
#include "steps.h"
#include "msg.h"
#include "ack.h"

char fifo_path[64];
int fifo_fd;

void init_fifo(pid_t pid) {
    sprintf(fifo_path, "/tmp/dev_fifo.%d", pid);
    mkfifo(fifo_path, S_IRUSR | S_IWUSR);
    fifo_fd = open(fifo_path, O_RDONLY | O_NONBLOCK);
}

void teardown_fifo() {
    close(fifo_fd);
    unlink(fifo_path);
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

_Noreturn void device_loop(int dev_i) {
    pid = getpid();
    init_fifo(pid);

    while (true) {
        // First scan:
        //  1) Send messages
        //  2) Receive messages
        //  3) Move
        await_turn(dev_i);

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
                    if (dist <= m->max_dist && !has_dev_received_msg(target_pid, m->id)) {
                        m->pid_sender = pid;
                        m->pid_receiver = target_pid;
                        send_msg(m);
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
        while (1) {
            // Use stack allocated message as buffer.
            msg msg_temp;
            // Remember that `fifo_fd` is open in non-blocking mode. Reads that happend concurrently with a write
            // should set `errno` to `EWOULDBLOCK' or `EAGAIN` but we gangster and we don't care.
            int br = read(fifo_fd, &msg_temp, sizeof(msg));
            if (br < sizeof(msg)) break;

            // If read is successful then malloc and copy over the buffer using `memcpy`
            msg *msg_ptr = malloc(sizeof(msg));
            memcpy(msg_ptr, &msg_temp, sizeof(msg));
            list_insert_after(&messages, &msg_ptr->list_handle);

            add_ack(msg_ptr);
        }

        // 3) Move
        if (current_step > 0) {
            // Remove old position
            board_set(current_pos, 0);
        }

        // Set new position
        current_pos = steps[current_step][dev_i];
        board_set(current_pos, pid);

        pass_turn(dev_i);

        // Second scan, print status and remove messages
        await_turn(dev_i);
        print_status();
        pass_turn(dev_i);

        current_step++;
    }
}
