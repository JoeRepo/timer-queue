#ifndef FREE_LIST_H
#define FREE_LIST_H

#include "timer_queue.h"

typedef struct dll_node_t free_list_memory_t;

struct free_list_node_t {
	struct free_list_memory_t *node;
	struct free_list_node_t *next;
} free_list_node;

#endif