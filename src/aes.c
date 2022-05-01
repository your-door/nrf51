#include "nrf.h"
#include "aes.h"
#include "timer.h"
#include "callback.h"

#include <string.h>
#include <stdlib.h>

#ifdef LOG
#include "rtt/SEGGER_RTT.h"
#endif

typedef struct {
    uint8_t key[16];
    uint8_t clear[16];
    uint8_t encrypted[16];
} ecb_data;

static void (*onECBDoneCB)(uint8_t encrypted[16]);

void aes_cb_free()
{
    free((void *) NRF_ECB->ECBDATAPTR);
}

void ECB_IRQHandler(void)
{
    #ifdef LOG
    SEGGER_RTT_printf(0, "%u> AES: Interrupt\r\n", timer_get_seconds());
    #endif


    if (NRF_ECB->EVENTS_ENDECB)
    {
        NRF_ECB->EVENTS_ENDECB = 0;

        // Get the data out of the struct
        ecb_data *data = (ecb_data *) NRF_ECB->ECBDATAPTR;
        callback_add_data(onECBDoneCB, data->encrypted, aes_cb_free);
    }
}

void aes_init(void)
{
    NVIC_DisableIRQ(ECB_IRQn);

    NRF_ECB->INTENSET = ECB_INTENSET_ENDECB_Msk;

    NVIC_ClearPendingIRQ(ECB_IRQn);
    NVIC_EnableIRQ(ECB_IRQn);
    NVIC_SetPriority(ECB_IRQn, 0);
}

void aes_encrypt(uint8_t key[16], uint8_t iv[16], uint8_t data[16], void (*cb)(uint8_t encrypted[16]))
{
    onECBDoneCB = cb;

    // Allocate ECB data struct
    ecb_data* data_ecb;
    data_ecb = (ecb_data*) malloc(sizeof(ecb_data));
    memcpy(data_ecb->key, key, 16);

    // Since we only have ECB in hardware we "emulate" CCM by XOR IV and data
    for(uint8_t i = 0; i < 16; i++)
    {
        data_ecb->clear[i] = data[i] ^ iv[i];
    }

    NRF_ECB->ECBDATAPTR = (uint32_t) data_ecb;
    NRF_ECB->TASKS_STARTECB = 1;
}