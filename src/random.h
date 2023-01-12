#ifndef DOOR_RANDOM_H__
#define DOOR_RANDOM_H__

/**
 * @brief Init the random hardware
 * 
 * @param cb callback which gets called when hardware is ready and has at least generates 8 bytes of random data
 */
void random_init(void (*cb)());

/**
 * @brief Get the current random values. The internal generation will start afterwards and roll over the random data again until 8 new bytes are populated
 * 
 * @return uint8_t* pointer to a uint8 array with 8 items
 */
uint8_t* random_get(void);

#endif