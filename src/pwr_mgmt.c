#include "nrf.h"

#define NRF52_ONRAM0_OFFRAM0    POWER_RAM_POWER_S0POWER_Off     << POWER_RAM_POWER_S0POWER_Pos      \
							  | POWER_RAM_POWER_S1POWER_Off     << POWER_RAM_POWER_S1POWER_Pos;

void power_management_init() 
{
    // We set some basic power saving measures here
    // We enable low power CPU mode, DC/DC regulator
    NRF_POWER->TASKS_LOWPWR = 1;
    NRF_POWER->DCDCEN = 1;

    // Turn off RAM we don't need
    NRF_POWER->RAM[1].POWER = NRF52_ONRAM0_OFFRAM0;
    NRF_POWER->RAM[2].POWER = NRF52_ONRAM0_OFFRAM0;
    NRF_POWER->RAM[3].POWER = NRF52_ONRAM0_OFFRAM0;
}