#include "manager_threads.h"
#include "manager_utils.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>


pthread_mutex_t logfile_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t data_mtx = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t cond_filler = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_worker = PTHREAD_COND_INITIALIZER; 
pthread_cond_t cond_manager = PTHREAD_COND_INITIALIZER;


void *ma_th_filler(void *arg){

    _shared_data *sd = (_shared_data *) arg;

    while(1){

        // Wait until there no task in the Depositor and until Queue has a tasks
        pthread_mutex_lock(&data_mtx);
        while(sd->depositor_size != 0 || q_is_empty(&(sd->task_queue))){

             if(sd->shutdown == true){
                pthread_mutex_unlock(&data_mtx);
                //printf("End filler\n");
                return NULL;
            }

            pthread_cond_wait(&cond_filler, &data_mtx);
        }

        // This mean that Depositor is empty and exist at least on task in the Task Queue
        for(int i = 0; i < sd->depositor_max_size; i++){ // Try to populate all the Depositor

            if(q_is_empty(&(sd->task_queue))) // We have no task left in the Queue
                break;

            // Usefull information for worker threads
            sd->depositor[i] = q_pop(&(sd->task_queue));
            sd->depositor_size++;                     
        }

        //printf("\nMove %d tasks\n", sd->depositor_size);

        if(q_is_empty(&(sd->task_queue)))
            pthread_cond_signal(&cond_manager);

        pthread_cond_broadcast(&cond_worker); // Notify ALL the Workers becouse exist at least one task
        pthread_mutex_unlock(&data_mtx);
    }
}

void *ma_th_worker(void *arg){

    _shared_data *sd = (_shared_data *) arg;
    _q_node *task;
    int i;
    char *max_path_src, *max_path_trg;

    while(1){

        // Wait until exist at least one task in the Depositor
        pthread_mutex_lock(&data_mtx);
        while(sd->depositor_size == 0){

            if(sd->shutdown == true){
                pthread_mutex_unlock(&data_mtx);
                //printf("End worker\n");
                return NULL;
            }

            pthread_cond_wait(&cond_worker, &data_mtx);
        }

        // Find and take the first task you will find
        for(i = 0; i < sd->depositor_max_size; i++){

            if(sd->depositor[i] != NULL)
                break;
        }

        // Usefull information for Filler thread
        task = sd->depositor[i];
        sd->depositor[i] = NULL;
        sd->depositor_size--;
        sd->working_count++;

        if(sd->depositor_size == 0){
            pthread_cond_signal(&cond_filler);
            pthread_cond_signal(&cond_manager);
        }
        
        pthread_mutex_unlock(&data_mtx);

        
        // Here we do our task
        _node_data *nd = &(task->node_data);

        if((max_path_src = (char *) malloc(strlen(nd->source_dir) + strlen(nd->filename) + 2)) == NULL){ // +2 for <'/'> <'\0'>
            perror("Error in (ma_th_worker)");
            exit(EXIT_FAILURE);
        }

        if((max_path_trg = (char *) malloc(strlen(nd->target_dir) + strlen(nd->filename) + 2)) == NULL){ // +2 for <'/'> <'\0'>
            perror("Error in (ma_th_worker)");
            exit(EXIT_FAILURE);            
        }

        sprintf(max_path_src, "%s/%s", nd->source_dir, nd->filename);
        sprintf(max_path_trg, "%s/%s", nd->target_dir, nd->filename);

        ma_ut_workers_job(nd->source_ip, nd->source_port, nd->target_ip, nd->target_port, max_path_src, max_path_trg, nd->filename, sd->logfile);

        // Update the shared data that i end my task
        pthread_mutex_lock(&data_mtx);
        sd->working_count--;
        if(sd->working_count == 0)
            pthread_cond_signal(&cond_manager);
        pthread_mutex_unlock(&data_mtx);

        // Dealocate the memmory i do not need anymore
        q_delete_node(task);
        free(max_path_src); free(max_path_trg); 
    }
}


_shared_data *ma_th_create_shared_data(const int depositor_max_size, char *logfile_path){

    _shared_data *sd;

    if((sd = (_shared_data *) malloc(sizeof(_shared_data))) == NULL){
        perror("Error in (ma_th_create_shared_data)");
        exit(EXIT_FAILURE);
    }

    if((sd->depositor = (_q_node **) malloc(sizeof(_q_node *) * depositor_max_size)) == NULL){
        perror("Error in (ma_th_create_shared_data)");
        exit(EXIT_FAILURE);        
    }

    // Inialize the data
    for(int i = 0; i < depositor_max_size; i++)
        sd->depositor[i] = NULL;

    q_init(&(sd->task_queue));
    sd->depositor_size = 0;
    sd->depositor_max_size = depositor_max_size;
    sd->working_count = 0;
    sd->shutdown = false;
    sd->logfile = logfile_path; // We do not alocate more memmory, does not need. (logfile_path if from main, args)

    return sd;
}

void ma_th_destroy_shared_data(_shared_data *sd){

    q_delete(&(sd->task_queue)); // This propably is useless. Becouse Queue must be empty to terminate the manager

    for(int i = 0; i < sd->depositor_max_size; i++)
        if(sd->depositor[i] != NULL)
            q_delete_node(sd->depositor[i]);


    free(sd->depositor);
    free(sd);
}

void ma_th_destroy_mutex_cond(){

    // Destroy mutex
    pthread_mutex_destroy(&logfile_mtx);
    pthread_mutex_destroy(&data_mtx);

    // Destroy condition variables
    pthread_cond_destroy(&cond_filler);
    pthread_cond_destroy(&cond_manager);
    pthread_cond_destroy(&cond_worker);
}




