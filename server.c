/// @file server.c
/// @brief Contiene l'implementazione del SERVER.

#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>

#include "settings.h"
#include "board.h"
#include "steps.h"

struct {
    pid_t ack_manager;
    pid_t devs[DEV_COUNT];
} pids;

void wait_children() {
    while (wait(NULL));
}

// Remove IPC, allocated memory, and such
void teardown() {
    teardown_board();
    teardown_steps();
    teardown_mov_semaphores();
}

// Kill children, wait for them to terminate, teardown, and then exit
void die(int code) {
    for (int i = 0; i < DEV_COUNT; i++)
        kill(pids.devs[i], SIGTERM);
    kill(pids.ack_manager, SIGTERM);

    wait_children();

    teardown();
    exit(code);
}

int ack_manager() {
    return 0;
}

_Noreturn int device(int dev_i) {
    while(1) {
        in_turn(dev_i, {
            if(current_step > 0) {
                pos_t prev_pos = steps[current_step - 1][dev_i];
                board_set(prev_pos, 0);
            }

            pos_t goto_pos = steps[current_step][dev_i];
            board_set(goto_pos, getpid());
        });
    }
}

int main(int argc, char *argv[]) {
    init_board();
    init_steps("./input/file_posizioni.txt");
    init_mov_semaphores();

    // Spawn ACK Manager
    if (!(pids.ack_manager = fork()))
        return ack_manager();

    // Spawn devices
    for (int i = 0; i < DEV_COUNT; i++)
        if (!(pids.devs[i] = fork()))
            device(i);

    for(int i=0;i<steps_count;i++) {
        printf("STEP %d\n", current_step);
        display_board();
        printf("\n");

        perform_step();
        sleep(2);
    }

    pause();
}