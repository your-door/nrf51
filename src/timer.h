#ifndef DOOR_TIMER_H__
#define DOOR_TIMER_H__

void timer_init(void (*cb)());
uint8_t timer_add(void (*cb)(), uint32_t interval);
uint32_t timer_get_seconds();

#endif