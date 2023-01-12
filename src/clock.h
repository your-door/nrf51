#ifndef DOOR_CLOCK_H__
#define DOOR_CLOCK_H__

void clock_init(void (*cb)());
void clock_start_hf(void (*cb)());
void clock_stop_hf(void (*cb)());

#endif