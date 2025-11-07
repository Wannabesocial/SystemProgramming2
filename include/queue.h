/*
    Σε αυτο το κοματι εχουμε header συναρτισεις και structs προκειμενου να υλοποιησουμε ενα link list. Στην ουσια θα χρησημοποιησουμε ενα link list σαν queue
*/


#ifndef QUEUE_H
#define QUEUE_H

#define IPv4_SIZE 15    // Max size of a IPv4 xxx.xxx.xxx.xxx
#define NONE "NONE"     // This means that there is no file

#include <stdint.h>
#include <stdbool.h>

/* The information we need for queue */
typedef struct _node_data{
    char *source_dir;
    char *target_dir;
    char *filename;
    char source_ip[IPv4_SIZE + 1]; // +1 for '\0'
    char target_ip[IPv4_SIZE + 1]; // +1 for '\0
    uint16_t source_port;
    uint16_t target_port;
}_node_data;

/* Every node of the queue */
typedef struct _q_node{
    _node_data node_data;
    struct _q_node *next; 
}_q_node;

/* Our Queue with O(1) push and pop */
typedef struct _queue{
    _q_node *start; 
    _q_node *end;
}_queue;



//------------------------------------------------------------------------------------


/* Initialize the Queue both start and end in NULL */
void q_init(_queue *queue);

/* Just print the Queue. Usefull for debugging */
void q_print(_queue *queue);

/* Check if Queue is empty */
bool q_is_empty(_queue *queue);

/* Insert data in the end of the Queue */
void q_push(_queue *queue, const char *filename,
    const char *source_dir, const char *target_dir,
    const char *source_ip, const char *target_ip,
    const uint16_t source_port, const uint16_t target_port);

/* Pop the node in the start of the Queue and return it. You must dealocate the memmory */
_q_node *q_pop(_queue *queue);

/* A helpfull funtction that take a Queue node and delete it (aka dealocate memmory) */
void q_delete_node(_q_node *q_node);

/* Delete all the Queue. Usefull only for debugging */
void q_delete(_queue *queue);

/* Find and delete in a Queue all the nodes base on the KEY SET:source directory.
Return true or false depending on whether something was deleted or not */
void q_delete_by_3key(_queue *queue, const char *source_dir, const char *source_ip, const uint16_t source_port);

/* Find if a node exist in a queue alrady */
bool q_exist_node(_queue *queue, const char *filename,
    const char *source_dir, const char *target_dir,
    const char *source_ip, const char *target_ip,
    const uint16_t source_port, const uint16_t target_port);

/* Find if a node with a specific set of KEYS exist */
bool q_Find_queue_3key(_queue *queue, const char *src_dir, const char *src_ip, const uint16_t src_port);

/* Find if a node with a specific set of KEYS exist. Return a pointer or NULL if not exist */
_node_data *q_Find_queue_1key(_queue *queue, const char *src_dir);

#endif