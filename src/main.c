#include "nrf.h"

static void lfclk_config(void)
{
  NRF_CLOCK->LFCLKSRC = (CLOCK_LFCLKSRC_SRC_RC << CLOCK_LFCLKSRC_SRC_Pos); // We use the internal RC reference, its enough for us
  NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;
  NRF_CLOCK->TASKS_LFCLKSTART = 1;

  // Wait for the low frequency clock to start
  while (NRF_CLOCK->EVENTS_LFCLKSTARTED == 0) {}
  NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;
}

/**
 * @brief Sleep until the next interrupt from NVIC arrives
 */
static void sleep_until_interrupt(void)
{
    __WFE();
    __SEV();
    __WFE();
}

int main(void) 
{
  lfclk_config();
  NRF_RADIO->POWER = RADIO_POWER_POWER_Disabled << RADIO_POWER_POWER_Pos;

  for (;;) 
  {
    sleep_until_interrupt(); 
  }
}