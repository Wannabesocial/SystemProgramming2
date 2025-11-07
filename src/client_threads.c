/*
    Implamatation of client thread functions

    26/5/2025
*/

#include "client_threads.h"
#include "client_utils.h"
#include "ipc.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <linux/limits.h>

pthread_mutex_t error_mtx = PTHREAD_MUTEX_INITIALIZER;

void cl_th_list(const int pr_socket, const char *relevant_path){

    int num_files, len;
    char *buff, **files;

    num_files = cl_ut_count_files(relevant_path);

    if((files = (char **) malloc(sizeof(char *) * num_files)) == NULL){
        perror("Error in (cl_th_fill)");
        exit(EXIT_FAILURE); 
    }

    cl_ut_save_files(relevant_path, files);

    len = 2; // 2 for '.' and '\0'
    for(int i = 0; i < num_files; i++)
        len += strlen(files[i]) + 1; // +1 for '\n'

    if((buff = (char *) malloc(len)) == NULL){
        perror("Error in (cl_th_fill)");
        exit(EXIT_FAILURE); 
    }

    // Ready the buffer with the message in format (file'\n'file'\n'.......'\n'.)
    char tmp[NAME_MAX + 2];   buff[0] = '\0';

    for(int i = 0; i < num_files; i++){
        sprintf(tmp , "%s\n", files[i]);
        strcat(buff, tmp);
    }
    strcat(buff, "."); // we add the temrinal charachter "."

    ipc_safe_write(pr_socket, buff, true);
    free(buff);

    // dealocate memmory for the files
    for(int i = 0; i < num_files; i++)
        free(files[i]);
    free(files);

    //printf("LIST done\n");
}
 
void cl_th_pull(const int pr_socket, const char *relevant_path){

    char error_msg[128], whitespace = ' ', str_file_size[MAX_STR_SIZE + 2]; // +2 for ' ' and '\0' 
    long file_size;

    // we start sending the <file size> <" ">
    file_size = cl_ut_file_size(relevant_path, error_msg);
    sprintf(str_file_size, "%ld ", file_size);
    ipc_safe_write(pr_socket, str_file_size, false);
 
    if(file_size == -1L){ // we got a error tring to open the file from above
        ipc_safe_write(pr_socket, error_msg, true);
    }
    else{ // No problem in file now we must transfer the input slowly
        cl_ut_read_file(pr_socket, relevant_path, file_size);
    }

    //printf("PULL done\n");
}

void cl_th_push(const int pr_socket, const char *relevant_path){

    // first time we got here we are sure we have chunck_size = -1 
    int file_fd;

    ipc_check(file_fd = open(relevant_path, O_CREAT | O_WRONLY | O_TRUNC, 0666), "cl_th_push"); // create a file, if already exist trancate it

    // now we must read slowly (chunk-chunk) and write to the file
    int chunk_size, readed_bytes, max_bytes_to_read; // try to read at most bytes

    char buff[MAX_RW_BUFF];
    ssize_t just_read_bytes;

    chunk_size = ipc_read_size(pr_socket);
    while(chunk_size != 0){

        readed_bytes = 0;
        while(chunk_size != readed_bytes){ // safe read from the socket

            max_bytes_to_read = (chunk_size - readed_bytes > MAX_RW_BUFF - 1) ? (MAX_RW_BUFF - 1) : (chunk_size - readed_bytes); // -1 couse we will add '\0'

            ipc_check(just_read_bytes = read(pr_socket, buff, max_bytes_to_read), "cl_th_push");
            buff[just_read_bytes] = '\0';
            ipc_safe_write(file_fd, buff, false);

            readed_bytes += just_read_bytes;
        }

        chunk_size = ipc_read_size(pr_socket); // continiues to the next chunk 
    }

    close(file_fd); // close the file
    //printf("PUSH done\n");
}

void *cl_th_action(void *arg){
    pthread_detach(pthread_self()); // we do not care about the output 
     
    int pr_socket = *((int *)arg); // unpack the data
    char *relevant_path, action[5], link[MAX_PATH], buff[MAX_PATH + 4 + 1]; // 4 (LIST, PULL, PUSH), 1 max whitespaces
    _client_actions cl_action;

    cl_ut_first_request_msg(pr_socket, buff);
    memcpy(action, buff, 5); // save action
    action[4] = '\0';
    strcpy(link, buff + 5); // save the link

    relevant_path = cl_ut_relevant_path(link);

    cl_action = cl_ut_find_action(action); // convert string --> int
    switch(cl_action){

        case LIST:
            cl_th_list(pr_socket, relevant_path);
            break;

        case PULL:
            cl_th_pull(pr_socket, relevant_path);
            break;
        
        case PUSH:
            cl_th_push(pr_socket, relevant_path);
            break;
        
        // was given a invalid action diferent from (LIST, PULL, PUSH)
        //default:
            //printf("Invalid Action. Accept Only (LIST, PUSH, PULL)\n");
    }

    close(pr_socket);
    free(arg); free(relevant_path);
}
