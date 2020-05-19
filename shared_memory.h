/// @file shared_memory.h
/// @brief Contiene la definizioni di variabili e funzioni
///         specifiche per la gestione della MEMORIA CONDIVISA.

#pragma once

typedef struct segment {
    int key;
    void* ptr;
} segment;

segment segment_init(size_t size);
void segment_teardown(segment* seg);
