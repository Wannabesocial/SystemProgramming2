#ifndef MANAGER_UTILS_H
#define MANAGER_UTILS_H

#include <stdint.h>
#include "queue.h"
#include "manager_threads.h"

#define RIGED_FILE_PATH "/aaaaaaaaaaaaa" // This ensure us that if we do fopen in pwd"/" we will try open a directory and not a file  

/* Read from a socket byte-byte until i read '\n' or '\0'. Save what i read return the specific charachter */
char ma_ut_read_LIST(const int socket, char *buff);

/* Read from a socket and write to enather socket */
void ma_ut_PULL_PUSH(const int socket_src, const int socket_trg, const int full_size);

/* This is all the job that workers must do. Also write the reports in the Logfile */
void ma_ut_workers_job(const char *src_ip, const uint16_t src_port, const char *trg_ip, const uint16_t trg_port, 
    const char *src_path, const char *trg_path, const char *file_name, const char *logfile_path);

/* Take the Queue that was populated from configuration file and create tasks (aka populate one ather Queue). We sent LIST and receave the file names */
void ma_ut_task_generation(_queue *config_queue, _queue *task_queue, const char *logfile);

/* Make the logfile message and report it in the logfile */
void ma_ut_create_and_report(const char *logfile_path, const int full_size, const char *error_msg, const bool is_pull, const char *file_name,
    const char *src_path, const char *src_ip, const uint16_t src_port,
    const char *trg_path, const char *trg_ip, const uint16_t trg_port);


/* Base on a KEY SET delete all the potitions of the DEPOSITOR that has a mutch */
void ma_ut_depositor_delete_3key(_shared_data *sd, const char *src_dir, const char *src_ip, const uint16_t src_port);

/* Try to find if exist a node with a specific SET KEY if yes return a pointer else NULL */
_node_data *ma_ut_FindReturn_depositor_1key(_shared_data *sd, const char *src_dir);

/* Try to find if exist a node with a specific SET KEY */ 
bool ma_ut_Find_depositor_3key(_shared_data *sd, const char *src_dir, const char *src_ip, const uint16_t src_port);

/* Do the operation ADD from CONSOLE. Report back to CONSOLE */
void ma_ut_do_add(_shared_data *sd, char *source_msg, char *target_msg, const int pr_socket);

/* Do the operation CANCEL from CONSOLE. Report back to CONSOLE */
void ma_ut_do_cancel(_shared_data *sd, char *source_dir, const int pr_socket);

#endif