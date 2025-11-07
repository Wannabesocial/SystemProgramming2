#ifndef CONFIG_H
#define CONFIG_H

#include "queue.h"

/* Read a configuration file and save the data of it in a Queue (it is a temporary save) */
void cf_read_confige(const char *config_path, _queue *queue);

/* Helpful function that separate data */
void cf_save_data(char *str, char *directory, char *ip, uint16_t *port);

#endif