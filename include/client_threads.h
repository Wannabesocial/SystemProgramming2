/*
    Here we have the header file for usefull functions that we will use for client threading and some that thread will call for simplicity

    26/5/2025
*/

#ifndef CLIENT_THREADS_H
#define CLIENT_THREADS_H

#include <pthread.h>

extern pthread_mutex_t error_mtx;


/* FILL. See if a specific folder exist and if yes return the files that it has as a list. Relevant path only. We pass only the fd of the sockets so we can close it layter */
void cl_th_list(const int pr_socket, const char *relevant_path);

/* PULL. Try to open a file for read so you can tranfer the data. If you canot open it for w/e reasons end the routin */
void cl_th_pull(const int pr_socket, const char *relevant_path);

/* PUSH. Create a file if already exist trancate it. Slowly read bytes from a socket then safe write it in the file. When you get 0 as chunck size terminate */
void cl_th_push(const int pr_socket, const char *relevant_path);

/*
    Read from a socket and decide what action you will do between LIST, PULL, PUSH

    arg = int *socket
*/
void *cl_th_action(void *arg);

#endif
