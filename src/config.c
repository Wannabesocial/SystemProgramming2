#include "config.h"
#include "ipc.h"
#include "queue.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <linux/limits.h>


void cf_save_data(char *str, char *directory, char *ip, uint16_t *port){
    
    char delimeter[] = {'@', ':', '\0'}, *token;
    int count = 0;

    token = strtok(str, delimeter);

    while(token != NULL){

        count++;

        switch(count){

            case 1:
                strcpy(directory, token);
                break;
            
            case 2:
                strcpy(ip, token);
                break;
            
            case 3:
                *port = (uint16_t) atoi(token);
                break;
            
            // Normaly we will never go here
            //default:
                //printf("Something goes wrong\n");
        }

        token = strtok(NULL, delimeter);
    }
}

void cf_read_confige(const char *config_path, _queue *queue){

    FILE *file;
    int buff_size = MAX_PATH + IPv4_SIZE + 3 + 5; // +3 for <'@'> <':'> <'\0'>, +5 for port [0,65535]
    char buff1[buff_size], buff2[buff_size];

    if((file = fopen(config_path, "r")) == NULL){
        perror("Error in (cf_read_confige)");
        exit(EXIT_FAILURE);
    }

    char src_dir[MAX_PATH/2], src_ip[IPv4_SIZE], trg_dir[MAX_PATH/2], trg_ip[IPv4_SIZE];
    uint16_t src_port, trg_port;

    while(fscanf(file, "%s %s", buff1, buff2) == 2){ // Read all the configuration file

        cf_save_data(buff1, src_dir, src_ip, &src_port);
        cf_save_data(buff2, trg_dir, trg_ip, &trg_port);

        // We ensure we do not save dublicates
        if(q_exist_node(queue, NONE, src_dir, trg_dir, src_ip, trg_ip, src_port, trg_port) == false)
            q_push(queue, NONE, src_dir, trg_dir, src_ip, trg_ip, src_port, trg_port);

    }

    fclose(file);
}

