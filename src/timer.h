#ifndef DOOR_TIMER_H__
#define DOOR_TIMER_H__

void timer_init(void);
void timer_add(void (*cb)(), uint32_t interval);
uint64_t timer_get_seconds();

#endif