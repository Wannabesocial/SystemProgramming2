#include "manager_utils.h"
#include "manager_threads.h"
#include "ipc.h"
#include "config.h"
#include "log.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char ma_ut_read_LIST(const int socket, char *buff){

    char c;
    int i = 0;

    while(1){

        if(read(socket, &c, 1) != 1){  
            perror("Error in (ma_ut_read_LIST)");
            exit(EXIT_FAILURE);
        }

        if(c == '\n' || c == '\0'){
            buff[i] = '\0';
            break;
        }

        buff[i++] = c;
    }

    return c;
}

void ma_ut_PULL_PUSH(const int socket_src, const int socket_trg, const int full_size){

    char buff[MAX_RW_BUFF], str_size[MAX_STR_SIZE + 2]; // +1 '\0' +1 ' ' (whitespace)
    int readed_bytes = 0, max_bytes_to_read;
    ssize_t just_read_bytes;

    while(readed_bytes != full_size){

        max_bytes_to_read = (full_size - readed_bytes > MAX_RW_BUFF - 1) ? (MAX_RW_BUFF - 1) : (full_size - readed_bytes); // try to read at most bytes
        
        ipc_check(just_read_bytes = read(socket_src, buff, max_bytes_to_read), "ma_ut_PULL_PUSH"); // safe read from one client
        buff[just_read_bytes] = '\0';

        // safe write to enather client with chunks
        sprintf(str_size, "%ld ", just_read_bytes);
        ipc_safe_write(socket_trg, str_size, false); // write the chunk size
        ipc_safe_write(socket_trg, buff, false); // write the actual data

        readed_bytes += just_read_bytes;
    }
}

void ma_ut_workers_job(const char *src_ip, const uint16_t src_port, const char *trg_ip, const uint16_t trg_port,
 const char *src_path, const char *trg_path, const char *file_name, const char *logfile_path){

    char riged_msg_sent[64], riged_msg_read[64];
    int socket_src, socket_trg; // One socket for reading PULL, one for writing PUSH
    ssize_t just_read_bytes;

    // Try to connect to client that i will receave the data aka PULL
    if((socket_src = ipc_connect_to_server(src_ip, src_port)) == -1)
        return;
    
    // Try to connect to client that i will sent the data aka PUSH
    if((socket_trg = ipc_connect_to_server(trg_ip, trg_port)) == -1){

        // We must sent a riged message to read client so he does not sent us back data
        sprintf(riged_msg_sent, "PULL %s", RIGED_FILE_PATH);
        ipc_safe_write(socket_src, riged_msg_sent, true);

        // We read the error message from the client. We do not care saving them
        while(1){
            ipc_check(just_read_bytes = read(socket_src, riged_msg_read, sizeof(riged_msg_read)), "ma_ut_test");
            if(riged_msg_read[just_read_bytes - 1] == '\0') // we finaly read all the message
                break;
        }

        close(socket_src); // Close the opened socket
        return;
    }

    // Both conections was done with no problems
    char *msg_pull_client, *msg_push_client, *error_msg;
    int full_size;

    if((msg_pull_client = (char *) malloc(strlen(src_path) + 4 + 2)) == NULL){ // +4 for <PULL> +2 <'\0'> <' '>
        close(socket_src); close(socket_trg);
        perror("Error in (ma_ut_test)");
        exit(EXIT_FAILURE);
    }

    if((msg_push_client = (char *) malloc(strlen(trg_path) + 4 + 5)) == NULL){// +4 for <PUSH> +5 <'\0'> 2x<' '> <'-''1'>
        close(socket_src); close(socket_trg);
        perror("Error in (ma_ut_test)");
        exit(EXIT_FAILURE);        
    }

    // Sent PULL command to the PULL client
    sprintf(msg_pull_client, "PULL %s", src_path);
    ipc_safe_write(socket_src, msg_pull_client, true);
    
    // Sent PUSH command to the PUSH client
    sprintf(msg_push_client, "PUSH %s -1", trg_path); 
    ipc_safe_write(socket_trg, msg_push_client, true);

    // Read the full size of the file
    if((full_size = ipc_read_size(socket_src)) == -1){ // an error occurred

        strcpy(msg_push_client, "0 "); // terminate PUSH client
        ipc_safe_write(socket_trg, msg_push_client, true);

        // READ error msg and report in logfile
        error_msg = ipc_safe_read(socket_src);

        pthread_mutex_lock(&logfile_mtx);
        ma_ut_create_and_report(logfile_path, full_size, error_msg, true, file_name,
            src_path, src_ip, src_port, trg_path, trg_ip, trg_port);
        pthread_mutex_unlock(&logfile_mtx);

        free(error_msg);

        // Close the opened sockets and free the memmory
        free(msg_pull_client); free(msg_push_client);
        close(socket_src); close(socket_trg);
        return;
    }

    // Report when we start the PULL-PUSH data 
    pthread_mutex_lock(&logfile_mtx);
    ma_ut_create_and_report(logfile_path, full_size, error_msg, true, file_name,
            src_path, src_ip, src_port, trg_path, trg_ip, trg_port);
    pthread_mutex_unlock(&logfile_mtx);

    // All goes well we are gonna copy-paste the file betwenn the 2 clients slowly
    ma_ut_PULL_PUSH(socket_src, socket_trg, full_size);
    
    strcpy(msg_push_client, "0 "); // terminate PUSH client
    ipc_safe_write(socket_trg, msg_push_client, true);

    // Report when we end the PULL-PUSH data
    pthread_mutex_lock(&logfile_mtx);
    ma_ut_create_and_report(logfile_path, full_size, error_msg, false, file_name,
            src_path, src_ip, src_port, trg_path, trg_ip, trg_port);
    pthread_mutex_unlock(&logfile_mtx);

    // Close the opened sockets and free the memmory
    free(msg_pull_client); free(msg_push_client);
    close(socket_src); close(socket_trg);
}

void ma_ut_task_generation(_queue *config_queue, _queue *task_queue, const char *logfile){

    int socket;
    char *from_cl_msg, *to_cl_msg, *token, delimiter[] = {'\n', '\0'};
    char report_msg[2 * MAX_PATH + 1], time_str[TIME_SIZE];

    // Itarate throught configuration Queue
    for(_q_node *q_node = config_queue->start; q_node != NULL; q_node = q_node->next){

        // Make sure SERVER exist
        if((socket = ipc_connect_to_server(q_node->node_data.source_ip, q_node->node_data.source_port)) == -1)
            continue;
            
        // Server exist.
        if((to_cl_msg = (char *) malloc(strlen(q_node->node_data.source_dir) + 6)) == NULL){ // +4 "LIST", +1 ' '(whitespace), +1 '\0'
            perror("Error in (ma_ut_task_generation)");
            close(socket);
            exit(EXIT_FAILURE);
        } 

        // Send the Command
        sprintf(to_cl_msg, "LIST %s", q_node->node_data.source_dir);
        ipc_safe_write(socket, to_cl_msg, true);

        from_cl_msg = ipc_safe_read(socket); // read the message (list of files)

        // All go good lets take one by one the file names
        token = strtok(from_cl_msg, delimiter);
        while(strcmp(token, ".") != 0){ // If we find "." this mean that we read all the list (list of files)

            q_push(task_queue, token,
                q_node->node_data.source_dir, q_node->node_data.target_dir,
                q_node->node_data.source_ip, q_node->node_data.target_ip,
                q_node->node_data.source_port, q_node->node_data.target_port);

            // Write to TERMINAL and to LOGFILE
            ipc_get_time(time_str); 
            sprintf(report_msg, "[%s] Added file: %s/%s@%s:%d -> %s/%s@%s:%d", time_str,
                q_node->node_data.source_dir, token, q_node->node_data.source_ip, q_node->node_data.source_port,
                q_node->node_data.target_dir, token, q_node->node_data.target_ip, q_node->node_data.target_port);

            printf("%s\n", report_msg); // Termianl
            log_write_logfile(logfile, report_msg); // Logfile

            token = strtok(NULL, delimiter);
        }

        free(from_cl_msg); free(to_cl_msg);
        close(socket);
    }
}

void ma_ut_create_and_report(const char *logfile_path, const int full_size, const char *error_msg, const bool is_pull, const char *file_name,
    const char *src_path, const char *src_ip, const uint16_t src_port,
    const char *trg_path, const char *trg_ip, const uint16_t trg_port)
{
    char logfile_msg[2 * MAX_PATH + 1], time_buff[TIME_SIZE];

    ipc_get_time(time_buff);

    if(full_size == -1) // This means we got an error in PULL
        sprintf(logfile_msg, "[%s] [%s@%s:%d] [%s@%s:%d] [%ld] [PULL] [ERROR] [File: %s - %s]", time_buff, src_path, src_ip, src_port,
            trg_path, trg_ip, trg_port, pthread_self(), file_name, error_msg);
    else
        if(is_pull)
            sprintf(logfile_msg, "[%s] [%s@%s:%d] [%s@%s:%d] [%ld] [PULL] [SUCCESS] [%d bytes pulled]", time_buff, src_path, src_ip, src_port,
                trg_path, trg_ip, trg_port, pthread_self(), full_size);
        else
            sprintf(logfile_msg, "[%s] [%s@%s:%d] [%s@%s:%d] [%ld] [PUSH] [SUCCESS] [%d bytes pushed]", time_buff, src_path, src_ip, src_port,
                trg_path, trg_ip, trg_port, pthread_self(), full_size);
    

    log_write_logfile(logfile_path, logfile_msg);
}

_node_data *ma_ut_FindReturn_depositor_1key(_shared_data *sd, const char *src_dir){

    _node_data *nd;
    _node_data *re_value = NULL;

    for(int i = 0; i < sd->depositor_max_size; i++){

        if(sd->depositor[i] == NULL) // Until we find an antry
            continue;
        
        nd = &(sd->depositor[i]->node_data);
        if(strcmp(nd->source_dir, src_dir) == 0){ // I have a match
            re_value = nd;
            break;
        }
    }

    return re_value;
}

bool ma_ut_Find_depositor_3key(_shared_data *sd, const char *src_dir, const char *src_ip, const uint16_t src_port){

    bool exist = false;
    _node_data *nd;

    for(int i = 0; i < sd->depositor_max_size; i++){

        if(sd->depositor[i] == NULL) // Until we find an antry
            continue;
        
        nd = &(sd->depositor[i]->node_data);
        if(strcmp(nd->source_dir, src_dir) == 0 && strcmp(nd->source_ip, src_ip) == 0 && nd->source_port == src_port){ // I have a match
            exist = true;
            break;
        }
    }
    return exist;
}

void ma_ut_depositor_delete_3key(_shared_data *sd, const char *src_dir, const char *src_ip, const uint16_t src_port){

    _node_data *nd;

    for(int i = 0; i < sd->depositor_max_size; i++){

        if(sd->depositor[i] == NULL) // Until we find actualy entry
            continue;

        nd = &(sd->depositor[i]->node_data);
        if(strcmp(nd->source_dir, src_dir) != 0 || strcmp(nd->source_ip, src_ip) != 0 || nd->source_port != src_port) // Does not have a fit
            continue;

        // We have a match
        q_delete_node(sd->depositor[i]);
        sd->depositor_size--;
        sd->depositor[i] = NULL;
    }
}

void ma_ut_do_add(_shared_data *sd, char *source_msg, char *target_msg, const int pr_socket){

    char src_dir[MAX_PATH/2], src_ip[IPv4_SIZE], trg_dir[MAX_PATH/2], trg_ip[IPv4_SIZE];
    char msg[MAX_PATH + 100], time_str[TIME_SIZE], *list_result, *token, delimiter[] = {'\n', '\0'};
    uint16_t src_port, trg_port;
    bool found;
    int cl_socket;

    // Ectract usefull information
    cf_save_data(source_msg, src_dir, src_ip, &src_port);
    cf_save_data(target_msg, trg_dir, trg_ip, &trg_port);

    
    pthread_mutex_lock(&data_mtx);

    // Check the DEPOSITOR and the TASK QUEUE
    if(ma_ut_Find_depositor_3key(sd, src_dir, src_ip, src_port) == true || q_Find_queue_3key(&(sd->task_queue), src_dir, src_ip, src_port) == true){
        ipc_get_time(time_str);
        sprintf(msg, "[%s] Already in queue: %s@%s:%d", time_str, src_dir, src_ip, src_port);
        
        printf("%s\n", msg); // Teminal
        ipc_safe_write(pr_socket, msg, true); // Console

        pthread_mutex_unlock(&data_mtx);
        return;
    }

    pthread_mutex_unlock(&data_mtx);


    // Directory does not exist. We start the sync.
    
    cl_socket = ipc_connect_to_server(src_ip, src_port); // Connect to CLIENT
    sprintf(msg, "LIST %s", src_dir);
    ipc_safe_write(cl_socket, msg, true); // send the command
    list_result = ipc_safe_read(cl_socket); // read the result

    pthread_mutex_lock(&data_mtx);

    // All go good lets take one by one the file names
    token = strtok(list_result, delimiter);
    while(strcmp(token, ".") != 0){ // If we find "." this mean that we read all the list (list of files)

        q_push(&(sd->task_queue), token, src_dir, trg_dir, src_ip, trg_ip, src_port, trg_port);

        // Write to TERMINAL and to LOGFILE
        ipc_get_time(time_str); 
        sprintf(msg, "[%s] Added file: %s/%s@%s:%d -> %s/%s@%s:%d", time_str,
            src_dir, token, src_ip, src_port,
            trg_dir, token, trg_ip, trg_port);

        printf("%s\n", msg); // Termianl

        pthread_mutex_lock(&logfile_mtx);
        log_write_logfile(sd->logfile, msg); // Logfile
        pthread_mutex_unlock(&logfile_mtx);

        strcat(msg, "\n");
        ipc_safe_write(pr_socket, msg, false); // To CONSOLE

        token = strtok(NULL, delimiter);
    }    

    // We add task in our Queue. Try to wake up the FILLER
    pthread_cond_signal(&cond_filler);

    pthread_mutex_unlock(&data_mtx);


    ipc_safe_write(pr_socket, "", true); // Console knows to temrinate the read. Send '\0'

    free(list_result);
    close(cl_socket);
}

void ma_ut_do_cancel(_shared_data *sd, char *source_dir, const int pr_socket){

    bool find = false;
    _node_data *nd;
    char msg[MAX_PATH + 100], src_ip[IPv4_SIZE], time_str[TIME_SIZE];
    uint16_t src_port;

    pthread_mutex_lock(&data_mtx);

    // Search the TASK Queue
    while((nd = q_Find_queue_1key(&(sd->task_queue), source_dir)) != NULL ||
        (nd = ma_ut_FindReturn_depositor_1key(sd, source_dir)) != NULL){

        // Save the IP and PORT
        strcpy(src_ip, nd->source_ip);
        src_port = nd->source_port;

        // Delete all the task in QUEUE TASK and in DEPOSITOR that has match
        q_delete_by_3key(&(sd->task_queue), source_dir, src_ip, src_port); 
        ma_ut_depositor_delete_3key(sd, source_dir, src_ip, src_port);
    
        // Ready the MSG
        ipc_get_time(time_str);
        sprintf(msg, "[%s] Synchronization stopped for %s@%s:%d", time_str, source_dir, src_ip, src_port);

        printf("%s\n", msg); // TERMINAL

        pthread_mutex_lock(&logfile_mtx);
        log_write_logfile(sd->logfile, msg); // LOGFILE
        pthread_mutex_unlock(&logfile_mtx);

        strcat(msg, "\n");
        ipc_safe_write(pr_socket, msg, false); // CONSOLE

        find = true;
    }

    // We propably delete TASKS from DEPOSITOR we must signal the FILLER
    pthread_cond_signal(&cond_filler);

    pthread_mutex_unlock(&data_mtx);


    if(find == true){
        strcpy(msg, "");
        ipc_safe_write(pr_socket, msg, true); // CONSOLE know i send all the message
    }
    else{
        ipc_get_time(time_str);
        sprintf(msg, "[%s] Directory not being synchronized: %s", time_str, source_dir);

        printf("%s\n", msg); // TERMINAL
        ipc_safe_write(pr_socket, msg, true); // CONSOLE 
    }
}
