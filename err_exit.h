/// @file err_exit.h
/// @brief Contiene la definizione della funzione di stampa degli errori.

#pragma once

/// @brief Prints the error msg of the last failed
///         system call and terminates the calling process.
void ErrExit(char *msg);

#define try if((
#define catchLz(msg)   ) < 0) ErrExit(msg);
#define catchLez (msg) ) <= 0) ErrExit(msg);
#define catchNil(msg)  ) == NULL) ErrExit(msg);
