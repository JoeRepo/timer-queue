#include "timer_queue.h"
#include "free_list.h"
#include "utils.h"

struct free_list_node_t free_list_memory[TIMER_QUEUE_MEMORY_SIZE];
struct free_list_node_t *free_list_head;
struct free_list_node_t *free_list_tail;

void free_list_init(struct free_list_node_t *memory, struct free_list_node_t **head, free_list_memory_t *nodes, size_t num_nodes)
{
	size_t i;

	for (i = 0; i < num_nodes; ++i) {
		memory[i].node = &nodes[i];
		if (num_nodes + 1 == i) {
			memory[i].next = &memory[i + 1];
		}
		else {
			memory[i].next = NULL;
			free_list_tail = &memory[i];
		}
	}

	*head = nodes;
}

free_list_memory_t *free_list_malloc(void)
{
	free_list_memory_t *retval;
	// Do we have a next node to get?
	if (free_list_head) {
		retval = free_list_head->node;

		// move the head to the next node to malloc
		free_list_head = free_list_head->next;

		return retval;
	}

	return NULL;
}

bool free_list_free(free_list_memory_t *memory)
{
	size_t i;

	if (memory) {
		// find the node that we're adding back to the list
		// TODO: Is this really the best way to do this?
		for (i = 0; i < ARRAY_SIZE(free_list_memory); ++i) {
			// we found the node to free
			if (free_list_memory[i].node == memory) {
				// Add the free node to the end of the list
				if (free_list_tail) {
					free_list_tail->next = &free_list_memory[i];
				}
				else {
					free_list_tail = &free_list_memory[i];
				}

				// if we have no head, make this the head
				if (!free_list_head) {
					free_list_head = &free_list_memory[i];
				}

				return true;
			}
		}
	}

	return false;
}