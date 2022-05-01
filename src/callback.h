#ifndef DOOR_CALLBACK_H_
#define DOOR_CALLBACK_H_

void callback_add_data(void (*cb)(uint8_t data[16]), uint8_t data[16], void (*freeCB)());
void callback_add(void (*cb)(), void (*freeCB)());
void callback_run();

#endif
