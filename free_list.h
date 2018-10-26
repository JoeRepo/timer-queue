#ifndef FREE_LIST_H
#define FREE_LIST_H

#include "timer_queue.h"

typedef struct dll_node_t free_list_memory_t;

struct free_list_node_t {
	free_list_memory_t *node;
	struct free_list_node_t *next;
} free_list_node;

extern struct free_list_node_t free_list_memory[TIMER_QUEUE_MEMORY_SIZE];
extern struct free_list_node_t *free_list_head;

void free_list_init(struct free_list_node_t *memory, struct free_list_node_t **head, free_list_memory_t *nodes, size_t num_nodes);
free_list_memory_t *free_list_malloc(void);
bool free_list_free(free_list_memory_t *memory);



#endif