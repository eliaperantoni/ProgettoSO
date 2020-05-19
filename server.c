/// @file server.c
/// @brief Contiene l'implementazione del SERVER.

#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>

#include "err_exit.h"
#include "defines.h"
#include "shared_memory.h"
#include "semaphore.h"
#include "fifo.h"

#define DEV_COUNT 5
#define BOARD_ROWS 10
#define BOARD_COLS 10

pid_t pids[DEV_COUNT + 1];

segment board, ack_table;

void wait_children() {
    while(wait(NULL));
}

// Remove IPC, allocated memory, and such
void teardown() {
    if(board.ptr != NULL) segment_teardown(&board);
    if(ack_table.ptr != NULL) segment_teardown(&ack_table);
}

// Kill children, wait for them to terminate, teardown, and then exit
void die(int code) {
    for(int i=0;i<=DEV_COUNT;i++)
        kill(pids[i], SIGTERM);

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
    board = segment_init(sizeof(int) * BOARD_ROWS * BOARD_COLS);
    if(!board.ptr) die(1);
    ack_table = segment_init(sizeof(int) * 100);
    if(!ack_table.ptr) die(1);

    // Spawn ACK Manager
    if(!(pids[0] = fork()))
        return ack_manager();

    // Spawn devices
    for(int i=0;i<DEV_COUNT;i++)
        if(!(pids[i+1] = fork()))
            return device(i);

    pause();
}