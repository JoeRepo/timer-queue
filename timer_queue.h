#ifndef TIMER_QUEUE_H
#define TIMER_QUEUE_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#define TIMER_QUEUE_TEST


#define TIMER_QUEUE_MEMORY_SIZE 4
#if defined(TIMER_QUEUE_TEST)

extern volatile uint16_t TSRC;



#define TIMER_TICKS_SOURCE	TSRC

#define TIMER_CLEAR_INTERRUPT()
#define TIMER_RESET_COUNT() TIMER_TICKS_SOURCE = 0;
#define TIMER_SET_THRESHOLD(ticks) 	
#else
#define TIMER_TICKS_SOURCE	TCNT1

#define TIMER_CLEAR_INTERRUPT()
#define TIMER_RESET_COUNT() TIMER_TICKS_SOURCE = 0;
#define TIMER_SET_THRESHOLD(ticks) 	OCR1A = ticks
#endif


typedef volatile uint16_t timer_ticks_t;
typedef volatile uint16_t *p_timer_ticks_t;
typedef uint8_t timer_queue_handle_t;
typedef void(timer_queue_callback_t)();

struct dll_node_t {
	struct dll_node_t *next;
	timer_ticks_t ticks;
	bool single_shot;
	timer_queue_handle_t handle;
	timer_queue_callback_t *callback;
} dll_node;

extern timer_queue_handle_t curr_handle;
extern timer_queue_callback_t *curr_callback;
extern bool timer_queue_fired;

bool timer_queue_add(timer_ticks_t ticks, void *cb(), bool single_shot, timer_queue_handle_t handle);
bool timer_queue_remove(timer_queue_handle_t handle);
void timer_queue_isr(void);
void timer_queue_init(void);
bool timer_queue_test(void);

#endif