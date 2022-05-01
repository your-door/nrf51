#include "nrf.h"
#include "clock.h"
#include "timer.h"
#include "callback.h"

#include <stddef.h>

#ifdef LOG
#include "rtt/SEGGER_RTT.h"
#endif

static void (*onInitCB)();
static void (*onHFCLKStartedCB)();

void clock_cb_free()
{
    onInitCB = NULL;
    onHFCLKStartedCB = NULL;
}

void POWER_CLOCK_IRQHandler(void)
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

        callback_add(onInitCB, clock_cb_free);        
    }

    if (NRF_CLOCK->EVENTS_HFCLKSTARTED) 
    {
        NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;

        #ifdef LOG
        SEGGER_RTT_printf(0, "%u> CLOCK: HFCLK started up\r\n", timer_get_seconds());
        #endif

        callback_add(onHFCLKStartedCB, clock_cb_free); 
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

void clock_start_hf(void (*cb)())
{
    onHFCLKStartedCB = cb;
    NRF_CLOCK->TASKS_HFCLKSTART = 1;
}

void clock_stop_hf(void (*cb)())
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

