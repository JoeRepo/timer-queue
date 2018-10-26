#include <assert.h>

#include "timer_queue.h"
#include "free_list.h"
#include "utils.h"

p_timer_ticks_t ticks_source;
struct dll_node_t timer_queue_memory[TIMER_QUEUE_MEMORY_SIZE];
struct dll_node_t *timer_queue_head = NULL;
size_t curr_queue_memory_size = 0;
size_t p_next_empty_mode = 0;
timer_queue_handle_t curr_handle;
timer_queue_callback_t *curr_callback;
bool timer_queue_fired = false;

volatile uint16_t TSRC = 0;

bool timer_start(void)
{
	return true;
}

void timer_queue_init(void)
{
	free_list_init(free_list_memory, &free_list_head, timer_queue_memory, ARRAY_SIZE(timer_queue_memory));
	ticks_source = &TIMER_TICKS_SOURCE;
}

// NOTE: if memory allocation and freeing is done on the heap, this should be done outside of the ISR
// NOTE: The callback is not called in the ISR.  It should be called outside of the ISR.
// We do not know what the callback does and it is much safer to call it outside of the interrupt context
void timer_queue_isr(void)
{
	struct dll_node_t *temp;

	// Start counting again right away
	TIMER_CLEAR_INTERRUPT();
	TIMER_RESET_COUNT();

	// Update the global variables
	timer_queue_fired = true;
	curr_callback = timer_queue_head->callback;
	curr_handle = timer_queue_head->handle;

	// If we don't re-add this timer, just move down the list and free the current node
	temp = timer_queue_head;
	timer_queue_head = timer_queue_head->next;

	if (temp->single_shot) {
		if (timer_queue_head) {
			TIMER_SET_THRESHOLD(timer_queue_head->ticks);
		}
	}
	// Otherwise, we need to add the node that just fired back into the queue
	else {
		timer_queue_add(temp->ticks, temp->callback, false, temp->handle);
		TIMER_SET_THRESHOLD(timer_queue_head->ticks);
	}

	free_list_free(temp);
}

bool timer_queue_remove(timer_queue_handle_t handle)
{
	bool handle_found = false;
	struct dll_node_t *p_list;
	struct dll_node_t *node_to_free;
	struct dll_node_t *prev = NULL;
	timer_ticks_t ticks;

	// Find out where the node is that we need to remove
	for (p_list = timer_queue_head; p_list != NULL; p_list = p_list->next) {
		if (p_list->handle == handle) {
			handle_found = true;
			break;
		}

		prev = p_list;
	}

	if (handle_found) {
		// Save our data
		node_to_free = p_list;
		ticks = node_to_free->ticks;

		// If we're removing the head, move the head pointer and decriment everything down the list by the ticks elapsed
		if (node_to_free == timer_queue_head) {
			timer_queue_head = node_to_free->next;

			// Adjust the tick rate for the next element in the list, since we're looking at the head we adjust for the current ticks elapsed as well
			if (timer_queue_head) {
				timer_queue_head->ticks += (node_to_free->ticks - *ticks_source);
			}

			// Reset the timer with the new head
			TIMER_RESET_COUNT();
			TIMER_SET_THRESHOLD(timer_queue_head->ticks);
		}
		// Otherwise just go down the list
		else {
			// Adjust the tick rate for the next item in the list
			if (node_to_free->next) {
				node_to_free->next->ticks += node_to_free->ticks;
			}

			prev->next = node_to_free->next;
		}

		free_list_free(node_to_free);
	}

	return handle_found;
}

bool timer_queue_add(timer_ticks_t ticks, void *cb(), bool single_shot, timer_queue_handle_t handle)
{
	bool retval = true;
	struct dll_node_t *new_node;
	struct dll_node_t *prev = NULL;

	timer_ticks_t ticks_elapsed;
	timer_ticks_t ticks_left;
	struct dll_node_t *p_list;

	if (ticks) {
		if (curr_queue_memory_size < ARRAY_SIZE(timer_queue_memory)) {
			new_node = free_list_malloc();
			if (new_node) {
				new_node->callback = cb;
				new_node->single_shot = single_shot;
				new_node->handle = handle;
				new_node->ticks = ticks;

				// If we have a timer queue to go through, find out how many ticks have elapsed and go through the queue to find out where we put the timer
				if (timer_queue_head) {
					p_list = timer_queue_head;

					ticks_elapsed = *ticks_source;
					ticks_left = timer_queue_head->ticks - ticks_elapsed;
					// The new node has the smallest amount of ticks, make it the new head and decrement all of the other nodes
					if (ticks < ticks_left) {
						new_node->next = timer_queue_head;

						// decrement the time of the next node
						new_node->next->ticks -= ticks;

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
						// Start at the head and iterate through until we find the first node with more ticks than our new node
						for (p_list = timer_queue_head; (p_list != NULL); p_list = p_list->next) {
							if ((p_list->ticks + ticks_elapsed) > ticks) {
								break;
							}

							ticks_elapsed += p_list->ticks;

							prev = p_list;
						}
						
						// Store where we're putting the new node
						prev->next = new_node;
						new_node->next = p_list;
						// decrement the new node by the amount of ticks that have passed
						new_node->ticks = (ticks - ticks_elapsed);

						// decrement the time of the next node
						if (p_list) {
							p_list->ticks -= (ticks - ticks_elapsed);
						}

						retval = true;
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

// TODO: doesn't test the callback, ISR, or how it handles with a timer running
bool timer_queue_test(void)
{
	size_t i;
	struct dll_node_t *p_list;

	timer_queue_init();

	timer_queue_add(1000, NULL, false, 2);
	timer_queue_add(500, NULL, true, 1);
	timer_queue_add(2000, NULL, false, 4);
	timer_queue_add(1500, NULL, true, 3);

	p_list = timer_queue_head;
	for (i = 0; i < 4; ++i) {
		assert(p_list->handle == i + 1);
		assert(p_list->ticks == 500);
		p_list = p_list->next;
	}

	timer_queue_remove(1);
	timer_queue_remove(3);

	p_list = timer_queue_head;
	for (i = 0; i <2; ++i) {
		assert(p_list->handle == (i + 1) * 2);
		assert(p_list->ticks == 1000);
		p_list = p_list->next;
	}

	timer_queue_add(500, NULL, true, 1);
	timer_queue_add(1500, NULL, true, 3);

	p_list = timer_queue_head;
	for (i = 0; i < 4; ++i) {
		assert(p_list->handle == i + 1);
		assert(p_list->ticks == 500);
		p_list = p_list->next;
	}

	timer_queue_remove(4);
	timer_queue_remove(2);
	timer_queue_remove(3);
	timer_queue_remove(1);

	assert(timer_queue_head == NULL);

	timer_queue_add(1000, NULL, false, 3);
	timer_queue_add(500, NULL, true, 4);
	timer_queue_add(2000, NULL, false, 1);
	timer_queue_add(1500, NULL, true, 2);

	p_list = timer_queue_head;
	for (i = 0; i < 4; ++i) {
		assert(p_list->handle == 4 - i);
		assert(p_list->ticks == 500);
		p_list = p_list->next;
	}

	timer_queue_remove(1);
	timer_queue_remove(2);
	timer_queue_remove(3);
	timer_queue_remove(4);

	assert(timer_queue_head == NULL);

	return 0;
}