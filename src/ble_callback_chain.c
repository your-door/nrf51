#include "ble_callback_chain.h"
#include "ble.h"
#include "clock.h"
#include "timer.h"

#ifdef LOG
#include "rtt/SEGGER_RTT.h"
#endif

static uint8_t adv_pdu[40];

static void (*ble_timerEventDoneCB)();
static uint8_t ble_timer_slot;

uint8_t* ble_get_adv_pdu() 
{
    return &(adv_pdu);
}

void reschedule_ble_data(void)
{
    #ifdef LOG
    SEGGER_RTT_printf(0, "%u> BLE CB: HFCLK stopped. Telling timer to reschedule\r\n", timer_get_seconds());
    #endif

    ble_timerEventDoneCB(ble_timer_slot);
}

void finished_ble_data(void)
{
    // Stop HFCLK again
    clock_stop_hf(reschedule_ble_data);
}

void send_ble_data_on_channel_39(void)
{
    // Send data on channel
    ble_send_on_channel(39, adv_pdu, finished_ble_data);
}

void send_ble_data_on_channel_38(void)
{
    // Send data on channel
    ble_send_on_channel(38, adv_pdu, send_ble_data_on_channel_39);
}

void send_ble_data_on_channel_37(void) 
{
    #ifdef LOG
    SEGGER_RTT_printf(0, "%u> CORE: HFCLK started. BLE init next\r\n", timer_get_seconds());
    #endif

    // Init ble
    ble_init();

    // Send data on channel
    ble_send_on_channel(37, adv_pdu, send_ble_data_on_channel_38);
}

void ble_callback_chain(void (*doneCB)()) 
{
    ble_timerEventDoneCB = doneCB;

    #ifdef LOG
    SEGGER_RTT_printf(0, "%u> CORE: Got BLE adv timer event\r\n", timer_get_seconds());
    #endif

    clock_start_hf(send_ble_data_on_channel_37);
}

void ble_callback_chain_register()
{
    ble_timer_slot = timer_add(ble_callback_chain, 3);
}

