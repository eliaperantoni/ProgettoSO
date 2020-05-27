#pragma once

#include <unistd.h>

#include "list.h"

typedef struct {
    pid_t pid_sender;
    pid_t pid_receiver;
    int id;
    char content[256];
    double max_dist;
    list_handle_t list_handle;
} msg;

int send_msg(msg* m);
