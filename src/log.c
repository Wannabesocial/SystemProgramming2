#include "log.h"

#include "stdio.h"
#include "stdlib.h"

void log_init_logfile(const char *logfile_path){

    FILE *logfile;

    if((logfile = fopen(logfile_path , "w")) == NULL){
        perror("fopen in (log_init_ManagerLogfile)");
        exit(EXIT_FAILURE);
    }

    fclose(logfile);
}

void log_write_logfile(const char *logfile_path, const char *message){

    FILE *logfile;

    if((logfile = fopen(logfile_path, "a")) == NULL){
        perror("fopen in (log_write_ManagerLogfile)");
        exit(EXIT_FAILURE);
    }

    fprintf(logfile, "%s\n", message);
    fclose(logfile);
}
