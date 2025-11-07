#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/limits.h>
#include <signal.h>

#include "client_threads.h"
#include "client_utils.h"
#include "ipc.h"
#include "manager_threads.h"

int pu_socket; // public socket. The start of the communication with the outer world

// A nice way to terminate the NSF_CLIENT without any inpute interaction such "shutdown"
void terminate(int x){
    signal(SIGINT, &terminate);
    //printf("\nTerminating......\n");
    pthread_mutex_destroy(&error_mtx);
    ma_th_destroy_mutex_cond();
    close(pu_socket);
    exit(EXIT_SUCCESS);
}


int main(int argc, char **argv){
    
    if(argc != 3){
        printf("<./nfs_client> <-p> <port>\n");
        return 1;
    }

    if(strcmp(argv[1], "-p") != 0){
        printf("<-p>???\n");
        return 1;
    }
    
    signal(SIGINT, &terminate);

    pthread_t th;
    int *ptr_pr_socket;
    int pr_socket; // private socket. Now we can sent and reseave some data via internet

    ipc_check(pu_socket = socket(AF_INET, SOCK_STREAM, 0), "socket"); // Create socket
    ipc_check(ipc_bind_on_port(pu_socket, atoi(argv[2])), "bind_on_port"); // Bind the port and ip to socket
    ipc_check(listen(pu_socket, MAX_Q_REQUESTS), "listen"); // Listen, init the pending queue

    while(1){

        ipc_check(pr_socket = accept(pu_socket, NULL, NULL), "accept"); // Accept. I don't really care about the infos (ip, port, etc..) from the connected process

        if((ptr_pr_socket = (int *) malloc(sizeof(int))) == NULL){
            perror("Error in (nfs_client.c)");
            exit(EXIT_FAILURE);
        }

        *ptr_pr_socket = pr_socket;
        if(pthread_create(&th, NULL, &cl_th_action, (void *) ptr_pr_socket) != 0){ // could not make a thread
            perror("Error in nfs_client.c");
            exit(EXIT_FAILURE);
        }
    }

    return 0;
}