/// @file shared_memory.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche per la gestione della MEMORIA CONDIVISA.

#include <sys/shm.h>
#include <unistd.h>
#include "err_exit.h"
#include "shared_memory.h"

segment segment_init(size_t size) {
    int key = shmget(IPC_PRIVATE, size, IPC_CREAT | IPC_EXCL);
    void* ptr = shmat(key, NULL, 0);
    segment seg = {key, ptr};
    return seg;
}

void segment_teardown(segment* seg) {
    shmdt(seg->ptr);
    shmctl(seg->key, IPC_RMID, NULL);
}
