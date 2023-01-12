#ifndef DOOR_BLE_CALLBACK_CHAIN_H__
#define DOOR_BLE_CALLBACK_CHAIN_H__

#include <stdint.h>

void ble_callback_chain_register();
uint8_t* ble_get_adv_pdu();

#endif
