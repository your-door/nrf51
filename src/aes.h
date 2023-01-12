#ifndef DOOR_AES_H__
#define DOOR_AES_H__

void aes_init(void);
void aes_encrypt(uint8_t key[16], uint8_t iv[16], uint8_t data[16], void (*cb)(uint8_t encrypted[16]));

#endif