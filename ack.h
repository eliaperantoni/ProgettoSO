#pragma once
#include <unistd.h>
#include <time.h>
#include <time.h>
#include <stdbool.h>

#include "msg.h"

#define ACK_TABLE_ROWS 10
#define ACK_TABLE_BYTES sizeof(ack) * ACK_TABLE_ROWS

typedef struct {
    pid_t pid_sender;
    pid_t pid_receiver;
    int message_id;
    time_t timestamp;
} ack;

void init_ack_table();
void teardown_ack_table();

_Noreturn void ack_manager_loop();

void add_ack(msg *msg_ptr);

bool has_dev_received_msg(pid_t dev_pid, int msg_id);
