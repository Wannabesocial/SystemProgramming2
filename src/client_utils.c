#include "client_utils.h"
#include "client_threads.h"
#include "ipc.h" // for MAX_RW_BUFF

#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>

int cl_ut_count_files(const char *path){

    // open directory
    DIR *dir = opendir(path);
    if(dir == NULL){
        // pthread_mutex_lock(&error_mtx);
        // perror("Error in (cl_ut_count_files)");
        // pthread_mutex_unlock(&error_mtx);
        return 0;
    }

    int sum = 0;
    struct dirent *de; 
    while((de = readdir(dir)) != NULL){ // traverse throught directory and count how many file exist

        if(strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
            continue;
        
        sum++;
    }

    closedir(dir);
    
    return sum;
}

void cl_ut_save_files(const char *path, char **files){

    // open directory
    DIR *dir = opendir(path);
    if(dir == NULL){
        // pthread_mutex_lock(&error_mtx);
        // perror("Error in (cl_ut_count_files)");
        // pthread_mutex_unlock(&error_mtx);
        return;
    }

    struct dirent *de;
    int i = 0; 
    while((de = readdir(dir)) != NULL){ // traverse throught directory and count how many file exist

        if(strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
            continue;
        
        files[i] = (char *) malloc(strlen(de->d_name) + 1); // +1 for '\0'
        if(files[i] == NULL){
            perror("Error in (cl_ut_save_files)");
            exit(EXIT_FAILURE);
        }

        strcpy(files[i], de->d_name);
        i++;
    }

    closedir(dir);
}

_client_actions cl_ut_find_action(const char *str_action){

    _client_actions client_actions;

    for(client_actions = 0; client_actions < ALL_CLIENT_ACTIONS; client_actions++)
        if(strcmp(str_action, str_client_actions[client_actions]) == 0)
            break;

    return client_actions;
}

void cl_ut_first_request_msg(const int socket, char *buff){

    char c;
    int i = 0;

    char tmp[MAX_PATH + 4 + 2 + 3]; // 4 (LIST, PULL, PUSH), 2 max whitespaces, 3 = ('-')('1')('\0')

    while(1){

        if(read(socket, &c, 1) != 1){
            perror("Error in (cl_ut_first_request_msg)");
            exit(EXIT_FAILURE);
        }

        tmp[i++] = c;

        if(c == '\0')
            break;
    }

    // remove the -1 is cinda useless if exist 
    if(strcmp(tmp + strlen(tmp) - 2, "-1") == 0)
        tmp[strlen(tmp) - 3] = '\0';

    strcpy(buff, tmp);
}

char *cl_ut_relevant_path(const char *path){

    char cwd[MAX_PATH], *relative_path;

    if(getcwd(cwd, sizeof(cwd)) == NULL){
        perror("Error in (cl_ut_relevant_path)");
        exit(EXIT_FAILURE);
    }
    
    if((relative_path = (char *) malloc(strlen(cwd) + strlen(path) + 1)) == NULL){ // +1 for '\0'
        perror("Error in (cl_ut_relevant_path)");
        exit(EXIT_FAILURE);
    }

    sprintf(relative_path, "%s%s", cwd, path);

    return relative_path;
}

long int cl_ut_file_size(const char *path, char *msg){

    FILE *file = fopen(path, "r");
    if(file == NULL){ // some error acursed

        pthread_mutex_lock(&error_mtx);
        // perror("Error in (cl_ut_file_size)");
        char *str = strerror(errno);
        pthread_mutex_unlock(&error_mtx);

        strcpy(msg, str);
        return -1;
    }

    fseek(file, 0L, SEEK_END);
    long int size = ftell(file);
    fclose(file);

    return size;
}

void cl_ut_read_file(const int socket, const char *file, const long int tottal_bytes){

    char buff[MAX_RW_BUFF]; // read and write slowly all the file
    int fd;

    // Most likely we will never get a error becouse previusly we open the file to find the size
    ipc_check((fd = open(file, O_RDONLY)), "cl_ut_read_file");

    long int readed_bytes = 0;
    size_t just_read_bytes, writed_bytes, just_writed_bytes;

    while(tottal_bytes != readed_bytes){

        ipc_check(just_read_bytes = read(fd, buff, MAX_RW_BUFF), "cl_ut_read_file"); // read as much as you can
        readed_bytes += just_read_bytes;

        // safe write what i read
        writed_bytes = 0;
        while(just_read_bytes != writed_bytes){

            ipc_check(just_writed_bytes = write(socket, buff + writed_bytes, just_read_bytes - writed_bytes), "cl_ut_read_file");
            writed_bytes += just_writed_bytes;
        }
    }

    close(fd);
}




