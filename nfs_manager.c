#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <linux/limits.h>
#include <netdb.h>
#include <time.h>

#include "ipc.h"
#include "manager_utils.h"
#include "client_threads.h"
#include "queue.h"
#include "config.h"
#include "log.h"
#include "manager_threads.h"

int main(int argc, char **argv){

    int max_workers, depositor_size, port;
    char *logfile, *configfile;

    if(argc != 11 && argc != 9){
        printf("./nfs_manager -l <manager_logfile> -c <config_file> -n <worker_limit> -p <port_number> -b <bufferSize>\n");
        ma_th_destroy_mutex_cond();
        return 1;
    }

    // Defoult workers limit = 5
    if(argc == 9){
        max_workers = 5;
        port = atoi(argv[6]);
        depositor_size = atoi(argv[8]);
    }else{
        max_workers = atoi(argv[6]);
        port = atoi(argv[8]);
        depositor_size = atoi(argv[10]);
    }   

    logfile = argv[2], configfile = argv[4];

    int pu_socket, pr_socket;
    pthread_t *workers, filler;
    _shared_data *sd;
    _queue config_queue;
    char *from_console_msg, delimeter[] = {' ', '\0'}, finnal_msg[2 * MAX_PATH], time_str_start[TIME_SIZE], time_str_end[TIME_SIZE];
    char *src_token, *trg_token, console_action[15], source_msg[MAX_PATH], target_msg[MAX_PATH];
    
    // Initialize our stractures
    sd = ma_th_create_shared_data(depositor_size, logfile); // Create shared data
    log_init_logfile(logfile); // Clear the logfile from the last program execution. Just for safety
    q_init(&config_queue); // Init our Queue
    
    // Will listen for the CONSOLE
    ipc_check(pu_socket = socket(AF_INET, SOCK_STREAM, 0), "socket"); // Create socket
    ipc_check(ipc_bind_on_port(pu_socket, port), "bind_on_port"); // Bind the port and ip to socket
    ipc_check(listen(pu_socket, MAX_Q_REQUESTS), "listen"); // Listen, init the pending queue

    // Populate the tasks in the TASK QUEUE. Does not MUTEX LOCK couse there exist only the MAIN THREAD
    cf_read_confige(configfile, &config_queue); // Read the configuration file and save it
    ma_ut_task_generation(&config_queue, &(sd->task_queue), logfile); // Transfer the tasks with the filenames

    // Create the Thread Pool (Worker thread) + the Fill thread
    if((workers = (pthread_t *) malloc(sizeof(pthread_t) * max_workers)) == NULL){
        perror("Error in (main)");
        exit(EXIT_FAILURE);
    }

    ipc_thread_check(pthread_create(&filler, NULL, &ma_th_filler, (void *)sd), "pthread_create"); // Filler thread
    for(int i = 0; i < max_workers; i++)
        ipc_thread_check(pthread_create(&workers[i], NULL, &ma_th_worker, (void *)sd), "pthread_create"); // Worker threads
    

    ipc_check(pr_socket = accept(pu_socket, NULL, NULL), "accept"); // Waiting for the CONSOLE to connect

    from_console_msg = ipc_safe_read(pr_socket); // Read the Console message
    while(strcmp(from_console_msg, "shutdown") != 0){

        // Find the operation (ADD, CANCEL)
        strcpy(console_action, strtok(from_console_msg, delimeter));
        strcpy(source_msg, strtok(NULL, delimeter));

        if(strcmp(console_action, "add") == 0){
            
            strcpy(target_msg, strtok(NULL, delimeter));
            ma_ut_do_add(sd, source_msg, target_msg, pr_socket);

        }
        else{
            ma_ut_do_cancel(sd, source_msg, pr_socket);
        }

        free(from_console_msg);
        from_console_msg = ipc_safe_read(pr_socket); // Read the next Console message
    }

    ipc_get_time(time_str_start);
    
    // Try to Shutdown
    pthread_mutex_lock(&data_mtx);
    while(q_is_empty(&(sd->task_queue)) == false || sd->working_count != 0 || sd->depositor_size != 0){
        pthread_cond_wait(&cond_manager, &data_mtx);
    }
    pthread_mutex_unlock(&data_mtx);

    // Make sure that the THREADS will terminate
    pthread_mutex_lock(&data_mtx);
    sd->shutdown = true;
    pthread_cond_broadcast(&cond_worker); // wake up Workers
    pthread_cond_signal(&cond_filler);  // wake up Filler
    pthread_mutex_unlock(&data_mtx);    

    // Wait for the Threads to terminate
    ipc_thread_check(pthread_join(filler, NULL), "pthread_join"); // Wait for Filler thread to end
    for(int i = 0; i < max_workers; i++)
        ipc_thread_check(pthread_join(workers[i], NULL), "pthread_join"); // Wait for Worker threads to end

    // Send Final message to CONSOLE
    ipc_get_time(time_str_end);
    sprintf(finnal_msg, "[%s] Shutting down manager...\n[%s] Waiting for all active workers to finish.\n"
        "[%s] Processing remaining queued tasks.\n[%s] Manager shutdown complete.", time_str_start, time_str_start, time_str_start, time_str_end);
    ipc_safe_write(pr_socket, finnal_msg, true);    
    

    // Dealocate memmory for our stractures
    ma_th_destroy_shared_data(sd); // This delete the task Queue
    ma_th_destroy_mutex_cond();
    free(workers);
    q_delete(&config_queue); // These are propably empty but we must be sure
    free(from_console_msg);

    // Close the socket
    close(pu_socket); close(pr_socket);

    return 0;
}