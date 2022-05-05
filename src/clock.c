#include "nrf.h"
#include "clock.h"
#include "timer.h"
#include "compiler.h"

#include <stddef.h>

#ifdef LOG
#include "rtt/SEGGER_RTT.h"
#endif

static void (*onInitCB)();
static void (*onHFCLKStartedCB)();

RAM_CODE void POWER_CLOCK_IRQHandler(void)
{
    #ifdef LOG
    SEGGER_RTT_printf(0, "%u> CLOCK: Interrupt\r\n", timer_get_seconds());
    #endif

    if (NRF_CLOCK->EVENTS_LFCLKSTARTED) 
    {
        NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;

        #ifdef LOG
        SEGGER_RTT_printf(0, "%u> CLOCK: LFCLK setup against external XTAL\r\n", timer_get_seconds());
        #endif

        if (onInitCB != NULL)
        {
            onInitCB();
            onInitCB = NULL;
        }     
    }

    if (NRF_CLOCK->EVENTS_HFCLKSTARTED) 
    {
        NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;

        #ifdef LOG
        SEGGER_RTT_printf(0, "%u> CLOCK: HFCLK started up\r\n", timer_get_seconds());
        #endif

        if (onHFCLKStartedCB != NULL)
        {
            onHFCLKStartedCB();
            onHFCLKStartedCB = NULL;
        }
    }
}

void clock_init(void (*cb)()) 
{
    NVIC_DisableIRQ(POWER_CLOCK_IRQn);
    onInitCB = cb;

    // Configure interrupts first
    NRF_CLOCK->INTENSET = CLOCK_INTENSET_LFCLKSTARTED_Msk | CLOCK_INTENSET_HFCLKSTARTED_Msk;

    NVIC_ClearPendingIRQ(POWER_CLOCK_IRQn);
    NVIC_EnableIRQ(POWER_CLOCK_IRQn);
    NVIC_SetPriority(POWER_CLOCK_IRQn, 0);

    NRF_CLOCK->LFCLKSRC = (CLOCK_LFCLKSRC_SRC_Xtal << CLOCK_LFCLKSRC_SRC_Pos); // We use external XTAL since tile mate has one
    NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;
    NRF_CLOCK->TASKS_LFCLKSTART = 1;
}

RAM_CODE void clock_start_hf(void (*cb)())
{
    onHFCLKStartedCB = cb;
    NRF_CLOCK->TASKS_HFCLKSTART = 1;
}

RAM_CODE void clock_stop_hf(void (*cb)())
{
    NRF_CLOCK->TASKS_HFCLKSTOP = 1;
 
    #ifdef LOG
    SEGGER_RTT_printf(0, "%u> CLOCK: HFCLK stopped\r\n", timer_get_seconds());
    #endif
 
    if (cb != NULL) 
    {
        cb();
    }
}

