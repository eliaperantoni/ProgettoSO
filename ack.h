#pragma once
#include <unistd.h>
#include <time.h>
#include <time.h>
#include <stdbool.h>

#include "msg.h"
#include "settings.h"

#define ACK_TABLE_ROWS 10
#define ACK_TABLE_BYTES sizeof(ack) * ACK_TABLE_ROWS

typedef struct {
    pid_t pid_sender;
    pid_t pid_receiver;
    int message_id;
    time_t timestamp;
    int zombie_at;
} ack;

int init_ack_table();
int teardown_ack_table();

int lock_ack_table();
int unlock_ack_table();

_Noreturn void ack_manager_loop();

int add_ack(msg *msg_ptr);

bool has_dev_received_msg(pid_t dev_pid, int msg_id);

int init_feedback_queue(int key);
int teardown_feedback_queue();

typedef struct {
    long message_id;
    ack acks[DEV_COUNT];
} feedback;
