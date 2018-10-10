#ifndef TIMER_QUEUE_H
#define TIMER_QUEUE_H

#include <stdint.h>
#include <stdbool.h>

#define TIMER_QUEUE_MEMORY_SIZE 4

typedef uint32_t timer_ticks_t;
typedef uint32_t *p_timer_ticks_t;
typedef uint8_t timer_queue_handle_t;

struct dll_node_t {
	struct dll_node_t *next;
	timer_ticks_t ticks;
	bool single_shot;
	void(*callback)();
} dll_node;

#endif