#pragma once
#include <unistd.h>
#include <time.h>

#define ACK_TABLE_ROWS 100
#define ACK_TABLE_BYTES sizeof(ack) * ACK_TABLE_ROWS

typedef struct {
    pid_t pid_sender;
    pid_t pid_receiver;
    int message_id;
    time_t timestamp;
} ack;

void init_ack_table();
void teardown_ack_table();
