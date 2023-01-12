#ifndef DOOR_BLE_H__
#define DOOR_BLE_H__

#include <stdint.h>

#define M_BD_ADDR_SIZE              (6)     /* BLE device address size. */

void ble_init();
void ble_send_on_channel(uint8_t channel_index, uint8_t * data, void (*cb)());
void ble_pdu_init(uint8_t * data);

#endif