#include "nrf.h"
#include "clock.h"
#include "timer.h"

#ifdef LOG
#include "rtt/SEGGER_RTT.h"
#endif

int main(void) 
{
  #ifdef LOG
  SEGGER_RTT_printf(0, "Booted up... \r\n");
  #endif

  // Init timers
  clock_init();
  timer_init();

  // Power management
  NRF_POWER->TASKS_LOWPWR = 1;
  NRF_POWER->DCDCEN = 1;

  #ifdef LOG
  SEGGER_RTT_printf(0, "Power settings configured (LOWPWR and DCDC enable) ... \r\n");
  #endif
  
  while(1)
  {     
    // Enter System ON sleep mode
    __WFE();  
    // Make sure any pending events are cleared
    __SEV();
    __WFE();
  }
}
