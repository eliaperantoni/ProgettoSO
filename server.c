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
    while(wait(NULL));
}

// Remove IPC, allocated memory, and such
void teardown() {
    teardown_board();
    teardown_steps();
}

// Kill children, wait for them to terminate, teardown, and then exit
void die(int code) {
    for(int i=0;i<DEV_COUNT;i++)
        kill(pids.devs[i], SIGTERM);
    kill(pids.ack_manager, SIGTERM);

    wait_children();

    teardown();
    exit(code);
}

int ack_manager() {
    printf("ACK manager!\n");
    return 0;
}

int device(int i) {
    printf("Device %d!\n", i);
    return 0;
}

int main(int argc, char * argv[]) {
    init_board();
    init_steps("./input/file_posizioni.txt");

    // Spawn ACK Manager
    if(!(pids.ack_manager = fork()))
        return ack_manager();

    // Spawn devices
    for(int i=0;i<DEV_COUNT;i++)
        if(!(pids.devs[i] = fork()))
            return device(i);

    pause();
}