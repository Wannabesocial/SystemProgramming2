/*
    Header file for all the useful functions that i will need for Inter Process Communication (Sockets,.. etc)

    Created in 25/5/2025
*/

#ifndef IPC_H
#define IPC_H

#define _POSIX_C_SOURCE 200809L


#include <stdbool.h>
#include <stdint.h>

#define MAX_Q_REQUESTS 20   // The max number of request that will wait on queue (listen)
#define MAX_RW_BUFF 20      // The max number of bytes that can readed or write from files, fd , etc.. at ones
#define MAX_STR_SIZE 20      // The max size of digits a chuck size can have
#define SMALL_MSG 64        // a intermediate for reading small messages is parts 
#define MAX_PATH 1024       // the max path that we can have
#define TIME_SIZE 20        // date Buffer Size .Format "Year-Month-Date Hours:Minutes:Seconts" (20 = 2*5 + 4(year) + 1(space) + 4(--,::) + 1('\0')) 

/* All functions around sockets return -1 on false so all handled the same way. We pass return value and name of the function. */
void ipc_check(const int result, const char *fun);

/* Bind a socket ,receive request from any address (0.0.0.0) with a specific PORT */
int ipc_bind_on_port(const int socket, const unsigned short port);

/* Write all the bytes from a array to a discriptor. In a safe way. We right or not the '\0' */
void ipc_safe_write(const int d, const char *buff, const bool null_terminated);

/* Read from a socket the size of a chunk or file. Read byte-byte until i find whitespace ' '. */
int ipc_read_size(const int fd);

/* Try to make a connection in a SERVER-CLIENT concept. You are the CLIENT that connect in a SERVER. Connect base on IP and a PORT. Return the socket
If there on no such SERVER return -1 */
int ipc_connect_to_server(const char *ip, const uint16_t port);

/* Read in a safe way from a socket until you find '\0' and save the message. You must free the buffer after you end */
char *ipc_safe_read(const int socket);

/* Get the actual time */
void ipc_get_time(char *time_buff);

/* A vary small function only to make our code more simple */
void ipc_thread_check(const int result, const char *fun);

#endif