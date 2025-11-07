/*
    Here we have the header file for usefull functions that we will use for manager threading and some that thread will call for simplicity
*/

#ifndef MANAGER_THREADS_H
#define MANAGER_THREADS_H

#include <pthread.h>
#include "queue.h"

extern pthread_mutex_t logfile_mtx;    // Ensure that only one worker or manager will write the report in logfile at a time
extern pthread_mutex_t data_mtx;       // Ensure that only one thread will change the shared data 

extern pthread_cond_t cond_filler;     // condiiton variable for the Filler
extern pthread_cond_t cond_worker;     // condiiton variable for the Worker
extern pthread_cond_t cond_manager;    // condiiton variable for the Manager

typedef struct _shared_data{
    _queue task_queue;          // filler
    _q_node **depositor;        // filler/worker
    int depositor_size;         // filler/worker
    int depositor_max_size;     // filler/worker/manager
    char *logfile;              // worker. Necessary so worker can report on log file after an operation
    int working_count;          // worker. Necessary to count how many threads has end their work. Manager must know when to terminate
    bool shutdown;              // filler/worker. This prevents the termination when we start the Threads 
}_shared_data;

/* Create the shared data and init it */
_shared_data *ma_th_create_shared_data(const int depositor_max_size, char *logfile_path);

/* Dealocate the memmory for the shared data */
void ma_th_destroy_shared_data(_shared_data *sd);

/* Destroy the mutex and the condition variables */
void ma_th_destroy_mutex_cond();

// Both of them arg = *_shared_data

/* Filler thread. Take task from Queue and populate the Depositor */
void *ma_th_filler(void *arg);

/* Worker thread. Take a task from the Depositor and do the hard work */
void *ma_th_worker(void *arg);




#endif
