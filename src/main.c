#include "nrf.h"
#include "clock.h"
#include "timer.h"
#include "ble.h"
#include "main.h"
#include "random.h"
#include "aes.h"
#include "ble_callback_chain.h"
#include "aes_callback_chain.h"
#include "pwr_mgmt.h"
#include "reboot_counter.h"
#include "compiler.h"

#include <string.h>
#include <stdlib.h>

#ifdef LOG
#include "rtt/SEGGER_RTT.h"
#endif

RAM_CODE void sleep_for_interrupt()
{
  // Enter System ON sleep mode
  __WFE();  
  // Make sure any pending events are cleared
  __SEV();
  __WFE();
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

RAM_CODE void idle()
{
  while(1)
  {     
    sleep_for_interrupt();
  }
}

int main(void) 
{
  reboot_counter_init();

  #ifdef LOG
  SEGGER_RTT_printf(0, "0> CORE: Booted up with reboot counter %u\r\n", reboot_counter_get());
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
    0x01, 0x06,
    0x1B,
    0xFF, 0x59, 0x00, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA
  };
  memcpy(&adv_pdu[3 + M_BD_ADDR_SIZE], &(beacon_temp_only[0]), sizeof(beacon_temp_only));
  adv_pdu[1] = M_BD_ADDR_SIZE + sizeof(beacon_temp_only);

  // Power management
  power_management_init();

  #ifdef LOG
  SEGGER_RTT_printf(0, "0> CORE: Power settings configured (LOWPWR and DCDC enable)\r\n");
  #endif
  
  idle();
}
