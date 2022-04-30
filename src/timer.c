#include <stdlib.h>

#include "nrf.h"
#include "timer.h"

#ifdef LOG
#include "rtt/SEGGER_RTT.h"
#endif

// RTC stuff
#define LFCLK_FREQUENCY           (32768UL)                               // Low freq according to (nRF 51822 spec v3.3, 3.6, LFCLK). This freq is used by RTC (nRF 51822 spec v3.3, 4.3)
#define RTC_FREQUENCY             (8UL)                                   // How often do we need to update RTC ticks. This defines how accurate the timer will be
#define COUNTER_PRESCALER         ((LFCLK_FREQUENCY / RTC_FREQUENCY) - 1) // How often does the low frequency need to tick before RTC ticks once

typedef struct {
    uint32_t interval;
    void (*cb)();
} timer_def;

timer_def* slots[4];
uint8_t current_slot = 0;
uint32_t overflow_seconds = 0;

void timer_init(void) 
{
    NRF_RTC1->PRESCALER = COUNTER_PRESCALER;                                        // Set prescaler to a TICK of RTC_FREQUENCY

    // Enable overflow interrupts for "time" tracking
    NRF_RTC1->EVTENSET = RTC_EVTENSET_OVRFLW_Msk;
    NRF_RTC1->INTENSET = RTC_INTENSET_OVRFLW_Msk;

    NVIC_ClearPendingIRQ(RTC1_IRQn);
    NVIC_EnableIRQ(RTC1_IRQn);                                                      // Enable Interrupt for the RTC in the core
    NVIC_SetPriority(RTC1_IRQn, 0);

    // Start RTC
    NRF_RTC1->TASKS_START = 1;

    #ifdef LOG
    SEGGER_RTT_printf(0, "%u> TIMER: RTC1 started\r\n", timer_get_seconds());
    #endif     
}

void timer_add(void (*cb)(), uint32_t interval)
{
    // Check if we have slots left in RTC
    if (current_slot == 3) 
    {
        #ifdef LOG
        SEGGER_RTT_printf(0, "%u> TIMER: Not slots left in RTC\r\n", timer_get_seconds());
        #endif    
        return;
    }

    timer_def *timer_slot;
    timer_slot = malloc(sizeof(timer_def));
    timer_slot->interval = interval;
    timer_slot->cb = cb;
    slots[current_slot] = timer_slot;

    NRF_RTC1->CC[current_slot] = interval * RTC_FREQUENCY;                 // We use compare 0 for low freq sync    
    NRF_RTC1->INTENSET = 0x1UL << (16UL + current_slot);

    current_slot++;
}

uint64_t timer_get_seconds()
{
    return (NRF_RTC1->COUNTER + (overflow_seconds * 0xFFFFFF)) / RTC_FREQUENCY;
}

void RTC1_IRQHandler(void)
{
    // Check if we overflowed
    if (NRF_RTC1->EVENTS_OVRFLW) 
    {
        NRF_RTC1->EVENTS_OVRFLW = 0;
        overflow_seconds++;

        #ifdef LOG
        SEGGER_RTT_printf(0, "%u> TIMER: Got overflow\r\n", timer_get_seconds());
        #endif   
    }

    uint8_t counter = 0;
    while(counter < 3)
    {
        #ifdef LOG
        SEGGER_RTT_printf(0, "%u> TIMER: Checking interrupt state for compare %u\r\n", timer_get_seconds(), counter);
        #endif   

        // This will trigger every COMPARE_COUNTERTIME seconds
        if (NRF_RTC1->EVENTS_COMPARE[counter])
        {
            NRF_RTC1->EVENTS_COMPARE[counter] = 0;                                              // Reset interrupt

            // Get timer callback slot
            timer_def *timer_slot = slots[counter];
            if (timer_slot != NULL) 
            {
                timer_slot->cb();
                NRF_RTC1->CC[counter] += timer_slot->interval * RTC_FREQUENCY;                  // We use compare 0 for low freq sync
            }
        }

        counter++;
    }  
}