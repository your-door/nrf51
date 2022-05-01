#include "nrf.h"
#include "callback.h"

#include <stddef.h>
#include <stdlib.h>

#ifdef LOG
#include "timer.h"
#include "rtt/SEGGER_RTT.h"
#endif

typedef struct cb_data {
    void (*onCBWithData)(uint8_t data[16]);
    uint8_t* data;

    void (*onCB)();
    void (*onFreeCB)();

    void* next;
} cb_data;

static cb_data* cur;

void linked_list_add(cb_data* cb_slot)
{
    cb_slot->next = NULL;

    // Check if we have a head
    if (cur == NULL) 
    {
        cur = cb_slot;
        return;
    }

    cb_data* current = cur;
    while (current->next != NULL)
    {
        current = current->next;
    }

    current->next = cb_slot;
}

void callback_add_data(void (*cb)(uint8_t data[16]), uint8_t data[16], void (*freeCB)())
{
    #ifdef LOG
    SEGGER_RTT_printf(0, "%u> CALLBACK: Adding callback with data\r\n", timer_get_seconds());
    #endif

    cb_data* cb_slot;
    cb_slot = (cb_data*) malloc(sizeof(cb_data));
    cb_slot->onFreeCB = freeCB;
    cb_slot->data = data;
    cb_slot->onCBWithData = cb;
    cb_slot->onCB = NULL;
    linked_list_add(cb_slot);
}

void callback_add(void (*cb)(), void (*freeCB)())
{
    #ifdef LOG
    SEGGER_RTT_printf(0, "%u> CALLBACK: Adding callback without data\r\n", timer_get_seconds());
    #endif

    cb_data* cb_slot;
    cb_slot = (cb_data*) malloc(sizeof(cb_data));
    cb_slot->onFreeCB = freeCB;
    cb_slot->onCB = cb;
    cb_slot->data = NULL;
    cb_slot->onCBWithData = NULL;
    linked_list_add(cb_slot);
}

void fire_item(cb_data* data)
{
    #ifdef LOG
    SEGGER_RTT_printf(0, "%u> CALLBACK: Calling callback: ", timer_get_seconds());
    SEGGER_RTT_printf(0, "0x%8.8x\r\n", (int) data);
    #endif

    if (data->onCB != NULL) 
    {
        #ifdef LOG
        SEGGER_RTT_printf(0, "%u> CALLBACK: Calling callback without data\r\n", timer_get_seconds());
        #endif

        data->onCB();
    }

    if (data->onCBWithData != NULL)
    {
        #ifdef LOG
        SEGGER_RTT_printf(0, "%u> CALLBACK: Calling callback with data\r\n", timer_get_seconds());
        #endif

        data->onCBWithData(data->data);
    }

    if (data->onFreeCB != NULL)
    {
        #ifdef LOG
        SEGGER_RTT_printf(0, "%u> CALLBACK: Calling free callback\r\n", timer_get_seconds());
        #endif

        data->onFreeCB();
    }
}

void callback_run()
{
    cb_data* next;
    while (cur != NULL)
    {
        fire_item(cur);
        next = (cb_data*) cur->next;
        free(cur);
        cur = next;
    }
}