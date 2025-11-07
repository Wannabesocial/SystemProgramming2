#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "ipc.h"
#include "log.h"

int main(int argc, char **argv){

    if(argc != 7){
        printf("./nfs_console -l <console-logfile> -h <host_IP> -p <host_port>\n");
        return 1;
    }

    int port = atoi(argv[6]);
    char *logfile = argv[2];

    struct hostent *he;

    if((he = gethostbyname(argv[4])) == NULL){
        perror("Error in (main)");
        exit(EXIT_FAILURE);
    }

    struct in_addr **addr_list = (struct in_addr **)he->h_addr_list;
    char *ip = inet_ntoa(*addr_list[0]);

    char *res, delimeter[] = {'.', '\0'};
    bool SHUTDONW = false, valid;
    int pu_socket;
    char user_input[MAX_PATH], time_str[TIME_SIZE], msg[MAX_PATH + 100];

    log_init_logfile(logfile);

    ipc_check(pu_socket = ipc_connect_to_server(ip, port), "nfs_console"); // Try to connect to MANAGER

    while(!SHUTDONW){

        //printf(">");
        fgets(user_input, sizeof(user_input), stdin);
        user_input[strlen(user_input) - 1] = '\0'; // remove the '\n'

        if(strcmp(user_input, "shutdown") == 0)
            SHUTDONW = true;

        // Save the command in LOGFILE
        ipc_get_time(time_str);
        sprintf(msg, "[%s] Command %s", time_str, user_input);

        log_write_logfile(logfile, msg);

        ipc_safe_write(pu_socket, user_input, true);
        res = ipc_safe_read(pu_socket);

        // We go more tha one line message (shutdown msgs, added more than one file or cancel more than one directories)
        if(res[strlen(res)] == '\n')
            res[strlen(res)] = '\0';

        printf("%s\n", res);

        // Write only if i do not have SHUTDOWN
        if(SHUTDONW == false)
            log_write_logfile(logfile, res);

        free(res);
    }

    close(pu_socket);
    return 0;
}