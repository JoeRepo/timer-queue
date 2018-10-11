#include "timer_queue.h"
#include "utils.h"

p_timer_ticks_t ticks_source;
struct dll_node_t timer_queue_memory[TIMER_QUEUE_MEMORY_SIZE];
struct dll_node_t *timer_queue_head;
size_t curr_queue_memory_size = 0;
size_t p_next_empty_mode = 0;
timer_queue_handle_t curr_handle;
timer_queue_callback_t *curr_callback;
bool timer_queue_fired = false;

bool timer_start(void)
{
	return true;
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

	if (timer_queue_head->single_shot) {
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
				new_node->handle = handle;

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
						// We already have the ticks for the head, start at the next node and iterate through until we find the first node with more ticks than our new 
						for (p_list = timer_queue_head->next; (p_list != NULL); p_list = p_list->next) {
							if ((p_list->ticks + ticks_elapsed) > ticks) {
								break;
							}

							ticks_elapsed += p_list->ticks;

							prev = p_list;
						}
						
						// decrement the new node by the amount of ticks that have passed
						new_node->ticks = (ticks - ticks_elapsed);

						// decrement the time of the other nodes
						for (p_list; p_list != NULL; p_list = p_list->next) {
							p_list->ticks -= ticks;
						}

						// We found out where to put the new node, insert the new node
						prev->next = new_node;
						new_node->next = p_list;

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