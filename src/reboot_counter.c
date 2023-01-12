#include "nrf.h"
#include "reboot_counter.h"
#include "compiler.h"

#ifdef LOG
#include "rtt/SEGGER_RTT.h"
#endif

void reboot_counter_init()
{
    // Get the "old" value
    uint32_t value = reboot_counter_get();

    #ifdef LOG
    SEGGER_RTT_printf(0, "0> REBOOT: Current stored reboot counter %u\r\n", value);
    #endif

    // Enable write
    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Een << NVMC_CONFIG_WEN_Pos;
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy);
    
    NRF_NVMC->ERASEUICR = 1;
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy);  

    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos;
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy);

    value++;

    #ifdef LOG
    SEGGER_RTT_printf(0, "0> REBOOT: Want to store reboot counter %u\r\n", value);
    #endif

    NRF_UICR->CUSTOMER[0] = value;
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy);  
    
    // Read only again
    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy){}
}

uint32_t reboot_counter_get()
{
    if (NRF_UICR->CUSTOMER[0] == 0xFFFFFFFF)
    {
        return 0;
    }

    return NRF_UICR->CUSTOMER[0];
}