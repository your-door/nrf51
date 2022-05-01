#include "nrf.h"
#include "clock.h"
#include "timer.h"
#include "ble.h"
#include "main.h"
#include "random.h"
#include "aes.h"
#include "ble_callback_chain.h"
#include "callback.h"
#include "aes_callback_chain.h"

#include <string.h>
#include <stdlib.h>

#ifdef LOG
#include "rtt/SEGGER_RTT.h"
#endif

#define NRF52_ONRAM0_OFFRAM0    POWER_RAM_POWER_S0POWER_Off     << POWER_RAM_POWER_S0POWER_Pos      \
												      | POWER_RAM_POWER_S1POWER_Off     << POWER_RAM_POWER_S1POWER_Pos;

void sleep_for_interrupt()
{
  // Enter System ON sleep mode
  __WFE();  
  // Make sure any pending events are cleared
  __SEV();
  __WFE();

  // Now we issue callbacks (after we have handled IRQs)
  callback_run();
}

void whenTimerInited(void)
{
  // Now we need a timer for sending this
  ble_callback_chain_register();
  aes_callback_chain_register();
}

void whenClockInited(void) 
{
  timer_init(whenTimerInited);
}

int main(void) 
{
  #ifdef LOG
  SEGGER_RTT_printf(0, "0> CORE: Booted up\r\n");
  #endif

  // Init timers
  clock_init(whenClockInited);

  // Init crypto
  random_init(aes_callback_chain_issue);
  aes_init();

  // Generate PDU advertising packet
  uint8_t* adv_pdu = ble_get_adv_pdu();
  ble_pdu_init(adv_pdu);

  // Add beacon data
  static const uint8_t beacon_temp_only[31] = 
  {
    0x02,
    0x01, 0x04,
    0x1B,
    0xFF, 0x59, 0x00, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA
  };
  memcpy(&adv_pdu[3 + M_BD_ADDR_SIZE], &(beacon_temp_only[0]), sizeof(beacon_temp_only));
  adv_pdu[1] = M_BD_ADDR_SIZE + sizeof(beacon_temp_only);

  // Power management
  NRF_POWER->TASKS_LOWPWR = 1;
  NRF_POWER->DCDCEN = 1;

  // Turn off RAM we don't need
  NRF_POWER->RAM[1].POWER = NRF52_ONRAM0_OFFRAM0;
  NRF_POWER->RAM[2].POWER = NRF52_ONRAM0_OFFRAM0;
  NRF_POWER->RAM[3].POWER = NRF52_ONRAM0_OFFRAM0;

  #ifdef LOG
  SEGGER_RTT_printf(0, "0> CORE: Power settings configured (LOWPWR and DCDC enable)\r\n");
  #endif
  
  while(1)
  {     
    sleep_for_interrupt();
  }
}
