/*
    Implematation of usefull IPC functions

    Created in 25/5/2025
*/


#include "ipc.h"
#include "client_threads.h"
#include "manager_threads.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h> // for sockaddr_in 
#include <arpa/inet.h> // for hton* 

void ipc_check(const int result, const char *fun){

    if(result != -1) // all goes well
        return;
    
    char error_msg[64];
    sprintf(error_msg, "Error in (%s)", fun);

    pthread_mutex_lock(&error_mtx);
    perror(error_msg);
    pthread_mutex_unlock(&error_mtx);

    exit(EXIT_FAILURE);
}

void ipc_thread_check(const int result, const char *fun){

    if(result == 0)
        return;

    char error_msg[64];
    sprintf(error_msg, "Error in (%s)", fun);

    pthread_mutex_lock(&error_mtx);
    perror(error_msg);
    pthread_mutex_unlock(&error_mtx);

    exit(EXIT_FAILURE);
}

int ipc_bind_on_port(const int socket, const unsigned short port){

    struct sockaddr_in server;
    
    memset(&server, 0, sizeof(server)); // init the memmory
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY); // we listent to all addresses ip = 0.0.0.0
    server.sin_port = htons(port);

    return bind(socket, (struct sockaddr *) &server, sizeof(server));
}

void ipc_safe_write(const int fd, const char *buff, const bool null_terminated){

    size_t tottal_bytes = (null_terminated) ? (strlen(buff) + 1) : strlen(buff); // +1 so we can write the '\0'
    size_t writed_bytes = 0;
    ssize_t len;

    while(tottal_bytes != writed_bytes){

        if((len = write(fd, buff + writed_bytes, tottal_bytes - writed_bytes)) == -1){

            pthread_mutex_lock(&error_mtx);
            perror("Error in (ipc_safe_write)");
            pthread_mutex_unlock(&error_mtx);

            exit(EXIT_FAILURE);
        }

        writed_bytes += (size_t)len;
    }
}

int ipc_read_size(const int fd){

    char c, str_size[MAX_STR_SIZE + 1]; // +1 for '\0'
    int i = 0;

    while(1){

        if(read(fd, &c, 1) != 1){

            pthread_mutex_lock(&error_mtx);
            perror("Error in (ipc_read_size)");
            pthread_mutex_lock(&error_mtx);

            exit(EXIT_FAILURE);
        }

        if(c == ' '){
            str_size[i] = '\0';
            break;
        }

        str_size[i++] = c;
    }

    return atoi(str_size);
}

int ipc_connect_to_server(const char *ip, const uint16_t port){

    int pu_socket; // public socket
    struct sockaddr_in server_addr;

    // Information for the connection
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);

    ipc_check(pu_socket = socket(AF_INET, SOCK_STREAM, 0), "ipc_connect_to_server"); // Create socket

    // Create conection if posible
    if(connect(pu_socket, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1){
        perror("Error wtf");
        pu_socket = -1;
    }
        

    return pu_socket;
}

char *ipc_safe_read(const int socket){

    char tmp[SMALL_MSG], *buff1, *buff2, *tmp_buff;
    size_t max_size = 1; // '\0'
    ssize_t just_read_bytes;


    if((buff1 = (char *) malloc(2)) == NULL){
        perror("Error in (ipc_safe_read)");
        exit(EXIT_FAILURE);
    }
    strcpy(buff1,"\0");

    while(1){

        ipc_check(just_read_bytes = read(socket, tmp, SMALL_MSG - 1), "ipc_safe_read");

        if(tmp[just_read_bytes - 1] != '\0'){ // last byte that i read
            tmp[just_read_bytes] = '\0';
        }

        max_size += strlen(tmp);

        // Copy old data to new buffer then copy the data you just read
        if((buff2 = malloc(max_size)) == NULL){
            perror("Error in (ipc_safe_read)");
            exit(EXIT_FAILURE);
        }
        strcpy(buff2, buff1);
        strcat(buff2, tmp);

        free(buff1);

        // Swap
        buff1 = buff2;
        buff2 = NULL;

        if(tmp[just_read_bytes - 1] == '\0')
            break;
    }
    return buff1;
}

void ipc_get_time(char *time_buff){

    time_t current_time = time(NULL);
    struct tm tmp;
    localtime_r(&current_time, &tmp);
    strftime(time_buff, TIME_SIZE, "%Y-%m-%d %H:%M:%S", &tmp);

}

