#include <stdint.h>
#include <stdbool.h>

#define TIMER_QUEUE_MEMORY_SIZE 4
#define ARRAY_SIZE(x) (sizeof(x)/(sizeof(*x)))



struct dll_node_t {
	struct dll_node_t *next;
	timer_ticks_t ticks;
	bool single_shot;
	void(*callback)();
} dll_node;

typedef uint32_t timer_ticks_t;
typedef uint32_t *p_timer_ticks_t;
typedef struct dll_node_t free_list_memory_t;
typedef uint8_t timer_queue_handle_t;

struct free_list_node_t {
	struct free_list_memory_t *node;
	struct free_list_node_t *next;
} free_list_node;

struct free_list_node_t free_list_memory[TIMER_QUEUE_MEMORY_SIZE];
struct free_list_node_t *free_list_head;
struct free_list_node_t *free_list_tail;
struct dll_node_t timer_queue_memory[TIMER_QUEUE_MEMORY_SIZE];
struct dll_node_t *timer_queue_head;
size_t curr_queue_memory_size = 0;
size_t p_next_empty_mode = 0;

p_timer_ticks_t ticks_source;

void free_list_init(struct free_list_node_t *memory, struct free_list_node_t **head, free_list_memory_t *nodes, size_t num_nodes)
{
	size_t i;

	for (i = 0; i < num_nodes; ++i) {
		memory[i].node = &nodes[i];
		if (num_nodes + 1 == i) {
			memory[i].next = &memory[i+1];
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

bool timer_start(void)
{
	return true;
}

bool timer_queue_add(timer_ticks_t ticks, void *cb(), bool single_shot, timer_queue_handle_t handle)
{
	bool retval;
	struct dll_node_t *new_node;
	struct dll_node_t *prev;

	timer_ticks_t ticks_elapsed;
	timer_ticks_t ticks_left;
	struct dll_node_t *p_list;

	if (ticks) {
		if (curr_queue_memory_size < ARRAY_SIZE(timer_queue_memory)) {
			new_node = free_list_malloc();
			if (new_node) {
				new_node->callback = cb;
				new_node->single_shot = single_shot;

				// If we have a timer queue to go through, find out how many ticks have elapsed and go through the queue to find out where we put the timer
				if (timer_queue_head) {
					p_list = timer_queue_head;

					ticks_elapsed = *ticks_source;
					ticks_left = timer_queue_head->ticks - ticks_elapsed;
					// The new node has the smallest amount of ticks, make it the new head and decrement all of the other nodes
					if (ticks < ticks_left) {
						new_node->next = timer_queue_head;
						new_node->ticks = ticks;

						// decrement the time of the other nodes
						for (p_list = timer_queue_head; p_list != NULL; p_list = p_list->next) {
							p_list->ticks -= ticks;
						}

						// decrement the current head with how much time has elapsed
						// TODO:  it could be possible to go negative here, do a sub and limit function
						timer_queue_head->ticks -= *ticks_source;

						// make the new node the head and restart the timer
						timer_queue_head = new_node;
						timer_start();

						retval = true;
					}
					// Otherwise we need to find where this new node belongs in the list
					else {
						new_node->ticks = ticks;

						// We already have the ticks for the head, start at the next node and iterate through until we find the first node with more ticks than our new node
						for (p_list = timer_queue_head->next; (p_list != NULL) && (ticks_elapsed < ticks); p_list = p_list->next) {
							ticks_elapsed += p_list->ticks;

							prev = p_list;
						}
						while (ticks > ticks_left) {

						}
					}

				}
				else {
					new_node->next = NULL;
					timer_queue_head = new_node;
				}
			}
			else {
				retval = false;
			}
		}
		else {
			retval = false;
		}
	}
	else {
		retval = false;
	}

	return retval;
}