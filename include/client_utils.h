/*
    Header file for helpfull functions for nfs_client.c

    26/5/2025
*/

#ifndef CLIENT_UTILS_H
#define CLIENT_UTILS_H

#define ALL_CLIENT_ACTIONS 3

typedef enum {LIST, PULL, PUSH} _client_actions;

static const char *str_client_actions[] = {"LIST", "PULL", "PUSH"};



/* Count usefull files (skip "." and "..") */
int cl_ut_count_files(const char *path);

/* Save the files from a directory in a array */
void cl_ut_save_files(const char *path, char **files);

/* Find the action base on a string. Convert string --> int */
_client_actions cl_ut_find_action(const char *str_action);

/* Read in a safe way the First msg of the request. Aka read until you find the terminate character '\0' */
void cl_ut_first_request_msg(const int socket, char *buff);

/* Make a given path relevant aka pwd/somepath */
char *cl_ut_relevant_path(const char *path);

/* Find the size of a file */
long int cl_ut_file_size(const char *path, char *msg);

/* Read a file in chunks and write it in a socket */
void cl_ut_read_file(const int socket, const char *file, const long int tottal_bytes);

#endif
