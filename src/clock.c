#include "nrf.h"
#include "clock.h"
#include "timer.h"

#ifdef LOG
#include "rtt/SEGGER_RTT.h"
#endif

void POWER_CLOCK_IRQHandler(void)
{
    if (NRF_CLOCK->EVENTS_CTTO)
    {
        #ifdef LOG
        SEGGER_RTT_printf(0, "%u> CLOCK: Syncing LFCLK against HFCLK\r\n", timer_get_seconds());
        #endif
        
        NRF_CLOCK->EVENTS_CTTO = 0;

        // Wait for HF clock
        NRF_CLOCK->TASKS_HFCLKSTART = 1;
        while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);        

        // Do calibration          
        NRF_CLOCK->TASKS_CAL = 1;

        // Wait until calibration finished
        while (NRF_CLOCK->EVENTS_DONE == 0);

        // Stop HFCLK
        NRF_CLOCK->TASKS_HFCLKSTOP = 1;

        // Restart timer
        NRF_CLOCK->TASKS_CTSTART = 1;
        NRF_CLOCK->EVENTS_DONE = 0;  

        #ifdef LOG
        SEGGER_RTT_printf(0, "%u> CLOCK: Done syncing LFCLK\r\n", timer_get_seconds());
        #endif       
    }
}

void clock_init(void) 
{
    NRF_CLOCK->LFCLKSRC = (CLOCK_LFCLKSRC_SRC_RC << CLOCK_LFCLKSRC_SRC_Pos); // We use the internal RC reference, its enough for us
    NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;
    NRF_CLOCK->TASKS_LFCLKSTART = 1;

    // Wait for the low frequency clock to start
    while (NRF_CLOCK->EVENTS_LFCLKSTARTED == 0);
    NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;
    NRF_CLOCK->CTIV = 32;               // Every 8 seconds
    NRF_CLOCK->TASKS_CTSTART = 1;

    // Enable interrupts
    NRF_CLOCK->INTENSET = CLOCK_INTENSET_CTTO_Msk;

    NVIC_ClearPendingIRQ(POWER_CLOCK_IRQn);
    NVIC_EnableIRQ(POWER_CLOCK_IRQn);

    #ifdef LOG
    SEGGER_RTT_printf(0, "%u> CLOCK: LFCLK setup against internal RC\r\n", timer_get_seconds());
    #endif
}

