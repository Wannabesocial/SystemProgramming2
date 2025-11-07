#include "queue.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void q_init(_queue *queue){
    queue->start = NULL;
    queue->end = NULL;
}

void q_print(_queue *queue){

    printf("Queue inforamtions\n");    

    for(_q_node *q_node = queue->start; q_node != NULL; q_node = q_node->next)
        printf("[SRC:%s/%s %s:%u] [TRG:%s/%s %s:%u]\n", q_node->node_data.source_dir, q_node->node_data.filename, q_node->node_data.source_ip, q_node->node_data.source_port,
            q_node->node_data.target_dir, q_node->node_data.filename, q_node->node_data.target_ip, q_node->node_data.target_port);

    printf("\n");
}

bool q_is_empty(_queue *queue){
    return (queue->start == NULL) ? true : false;
}

/* A helpfull funtction that allocate memmory for us */
_q_node *init_new_node(const char *source_dir, const char *target_dir, const char *filename){

    _q_node *new_node;

    if((new_node = (_q_node *) malloc(sizeof(_q_node))) == NULL){
        perror("Error in (ready_new_node 1)");
        exit(EXIT_FAILURE);
    }

    if((new_node->node_data.source_dir = (char *) malloc(strlen(source_dir) + 1)) == NULL){
        perror("Error in (ready_new_node 2)");
        exit(EXIT_FAILURE);
    }

    if((new_node->node_data.target_dir = (char *) malloc(strlen(target_dir) + 1)) == NULL){
        perror("Error in (ready_new_node 3)");
        exit(EXIT_FAILURE);
    }    

    if((new_node->node_data.filename = (char *) malloc(strlen(filename) + 1)) == NULL){
        perror("Error in (ready_new_node 4)");
        exit(EXIT_FAILURE);
    }

    return new_node;
}

void q_push(_queue *queue, const char *filename,
    const char *source_dir, const char *target_dir,
    const char *source_ip, const char *target_ip,
    const uint16_t source_port, const uint16_t target_port)
{
    _q_node *new_node = init_new_node(source_dir, target_dir, filename);

    // Transfer all the data
    strcpy(new_node->node_data.source_dir, source_dir);
    strcpy(new_node->node_data.target_dir, target_dir);
    strcpy(new_node->node_data.filename, filename);
    strcpy(new_node->node_data.source_ip, source_ip);
    strcpy(new_node->node_data.target_ip, target_ip);
    new_node->node_data.source_port = source_port;
    new_node->node_data.target_port = target_port;

    // Insert it at the end
    new_node->next = NULL;

    if(q_is_empty(queue))
        queue->start = new_node;
    else
        queue->end->next = new_node;

    queue->end = new_node;
}

_q_node *q_pop(_queue *queue){

    _q_node *popped_node = queue->start;
    queue->start = queue->start->next;

    if(q_is_empty(queue))
        queue->end = NULL;

    return popped_node;
}

void q_delete_node(_q_node *q_node){

    free(q_node->node_data.source_dir);
    free(q_node->node_data.target_dir);
    free(q_node->node_data.filename);

    free(q_node);
}

void q_delete(_queue *queue){

    _q_node *q_node;

    while(q_is_empty(queue) == false){
        q_node = q_pop(queue);
        q_delete_node(q_node);
    }
}

void q_delete_by_3key(_queue *queue, const char *source_dir, const char *source_ip, const uint16_t source_port){

    _q_node *current, *previous, *tmp;

    // We try to delete the first node until first node has no key or until queue is empty
    current = queue->start;
    while(1){

        if(current == NULL) // We delete all the Queue
            return;

        if(strcmp(current->node_data.source_dir, source_dir) != 0 || 
            strcmp(current->node_data.source_ip, source_ip) != 0 || current->node_data.source_port != source_port) // First node does not match
            break;
        
        // First node match
        q_delete_node(q_pop(queue));
        current = queue->start;
    }

    // We seartch throught the remaind Queue (aka until we find NULL). We are sure that exist at least one node
    current = queue->start->next; // the second node. NULL if exist only one node
    previous = queue->start; // the first node
    while(current != NULL){

        if(strcmp(current->node_data.source_dir, source_dir) != 0 ||
            strcmp(current->node_data.source_ip, source_ip) != 0 || current->node_data.source_port != source_port){ // We do not have a match. We condiniu our search

            previous = current;
            current = current->next;
            continue;
        }

        // We have a match
        tmp = current;
        previous->next = current->next;
        current = current->next;
        q_delete_node(tmp);
    }

    queue->end = previous; // Ensure that the end pointer will be in the end of the QUEUE
}

bool q_exist_node(_queue *queue, const char *filename,
    const char *source_dir, const char *target_dir,
    const char *source_ip, const char *target_ip,
    const uint16_t source_port, const uint16_t target_port)
{
    bool exist = false;
    _node_data *nd;

    for(_q_node *q_node = queue->start; q_node != NULL; q_node = q_node->next){ // Iterate through all the Queue

        nd = &(q_node->node_data); // shortcut
        if(strcmp(nd->filename, filename) == 0 && strcmp(nd->source_dir, source_dir) == 0 && 
            strcmp(nd->target_dir, target_dir) == 0 && strcmp(nd->source_ip, source_ip) == 0 &&
            strcmp(nd->target_ip, target_ip) == 0 && nd->source_port == source_port && nd->target_port == target_port){

            exist = true;
            break;
        }
    }
    
    return exist;
}

bool q_Find_queue_3key(_queue *queue, const char *src_dir, const char *src_ip, const uint16_t src_port){

    bool exist = false;
    _node_data *nd;

    for(_q_node *q_node = queue->start; q_node != NULL; q_node = q_node->next){

        nd = &(q_node->node_data); // shortcut
        if(strcmp(nd->source_dir, src_dir) == 0 && strcmp(nd->source_ip, src_ip) == 0 && nd->source_port == src_port){
            exist = true;
            break;
        }
    }
    return exist;
}

_node_data *q_Find_queue_1key(_queue *queue, const char *src_dir){

    _node_data *nd;
    _node_data *re_value = NULL;

    for(_q_node *q_node = queue->start; q_node != NULL; q_node = q_node->next){

        nd = &(q_node->node_data); // shortcut
        if(strcmp(nd->source_dir, src_dir) == 0){
            re_value = nd;
            break;
        }
    }

    return re_value;
}


