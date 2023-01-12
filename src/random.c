#include "nrf.h"
#include "random.h"
#include "timer.h"
#include "compiler.h"

#include <string.h>
#include <stdlib.h>

#ifdef LOG
#include "rtt/SEGGER_RTT.h"
#endif

static uint8_t value[8];                            // We only generate IVs 8 bytes long
static uint8_t index = 0;
static void (*onFirstDataCB)();

void RNG_IRQHandler(void)
{
    #ifdef LOG
    SEGGER_RTT_printf(0, "%u> RNG: Interrupt\r\n", timer_get_seconds());
    #endif

    if (NRF_RNG->EVENTS_VALRDY) 
    {
        NRF_RNG->EVENTS_VALRDY = 0;
        value[index++] = NRF_RNG->VALUE;

        if (index == 8)
        {
            #ifdef LOG
            SEGGER_RTT_printf(0, "%u> RNG: Generated 8 bytes. Stopping RNG\r\n", timer_get_seconds());
            #endif

            NRF_RNG->TASKS_STOP = 1;

            if (onFirstDataCB != NULL) 
            {
                onFirstDataCB();
                onFirstDataCB = NULL;
            }
        }
    }
}

void random_init(void (*cb)()) 
{
    onFirstDataCB = cb;

    NVIC_DisableIRQ(RNG_IRQn);

    NRF_RNG->CONFIG = RNG_CONFIG_DERCEN_Msk;        // Bias correction
    NRF_RNG->INTENSET = RNG_INTENSET_VALRDY_Msk;    

    NVIC_ClearPendingIRQ(RNG_IRQn);
    NVIC_EnableIRQ(RNG_IRQn);
    NVIC_SetPriority(RNG_IRQn, 0);

    NRF_RNG->TASKS_START = 1;
}

uint8_t* random_get(void) 
{
    uint8_t* data = (uint8_t*) malloc(8 * sizeof(uint8_t));
    memcpy(data, value, 8);

    // Reset and generate new bytes
    index = 0;
    NRF_RNG->TASKS_START = 1;

    return data;
}