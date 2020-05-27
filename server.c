/// @file server.c
/// @brief Contiene l'implementazione del SERVER.

#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>

#include "dev.h"
#include "settings.h"
#include "board.h"
#include "steps.h"
#include "ack.h"

struct {
    pid_t ack_manager;
    pid_t devs[DEV_COUNT];
} pids;

// Kill children, wait for them to terminate, teardown, and then exit
void die(int code) {
    for (int i = 0; i < DEV_COUNT; i++)
        if(kill(pids.devs[i], SIGTERM) == -1)
            perror("[SERVER] Could not kill device");
    if(kill(pids.ack_manager, SIGTERM) == -1)
        perror("[SERVER] Could not kill ack manager");

    // wait children to terminate
    while(wait(NULL) != -1);

    // Teardown allocated memory and remove IPCs
    if(teardown_board() == -1)
        perror("[SERVER] Could not teardown down");
    teardown_steps();
    if(teardown_mov_semaphores() == -1)
        perror("[SERVER] Could not teardown movement semaphores");
    if(teardown_ack_table() == -1)
        perror("[SERVER] Could not teardown ack table");
    if(teardown_feedback_queue() == -1)
        perror("[SERVER] Could not teardown feedback queue");

    exit(code);
}

static void fatal(char *msg) {
    perror(msg);
    die(1);
}

static void sighandler(int sig) {
    die(0);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: server <msg_queue_key> <steps_file_path>\n");
        return 1;
    }

    // Block all signals but SIGTERM and set handler
    sigset_t sig_set;
    if (sigfillset(&sig_set) == -1)
        fatal("[SERVER] Filling signal set");
    if (sigdelset(&sig_set, SIGTERM) == -1)
        fatal("[SERVER] Removing SIGTERM from signal set");
    if (sigprocmask(SIG_SETMASK, &sig_set, NULL) == -1)
        fatal("[SERVER] Setting signal mask");

    if (signal(SIGTERM, sighandler) == SIG_ERR)
        fatal("[SERVER] Setting SIGTERM handler");

    if (init_board() == -1)
        fatal("[SERVER] Initializing board");
    if (init_steps(argv[2]) == -1)
        fatal("[SERVER] Initializing steps");
    if (init_mov_semaphores() == -1)
        fatal("[SERVER] Initializing movement semaphores");
    if (init_ack_table() == -1)
        fatal("[SERVER] Initializing ack table");
    if (init_feedback_queue(atoi(argv[1])) == -1)
        fatal("[SERVER] Initializing feedback queue");

    // Spawn ACK Manager
    if (!(pids.ack_manager = fork()))
        ack_manager_loop();

    // Spawn devices
    for (int dev_i = 0; dev_i < DEV_COUNT; dev_i++)
        if (!(pids.devs[dev_i] = fork()))
            device_loop(dev_i);

    for (int step_i = 0; step_i < steps_count; step_i++) {
        printf("## Step %d: device positions ###########\n", step_i);
        if(perform_step() == -1)
            fatal("[SERVER] Performing step");
        printf("#######################################\n\n");
        sleep(2);
    }
}