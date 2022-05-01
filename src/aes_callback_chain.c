#include "aes_callback_chain.h"
#include "ble_callback_chain.h"
#include "settings.h"
#include "random.h"
#include "ble.h"
#include "timer.h"
#include "aes.h"

#include <string.h>
#include <stdlib.h>

#ifdef LOG
#include "rtt/SEGGER_RTT.h"
#endif

#define IV_OFFSET 7
#define IV_LENGTH 8

static void (*aes_timerEventDoneCB)();        // CB which should be called when BLE data is done to reschedule timer
static uint8_t aes_timer_slot;
static uint8_t* iv_data;

void on_aes_encrypted(uint8_t encrypted[16])
{
  uint8_t* adv_pdu = ble_get_adv_pdu();

  // Copy IV and free the generated IV directly after
  memcpy(&adv_pdu[3 + M_BD_ADDR_SIZE + IV_OFFSET], iv_data, IV_LENGTH);
  free(iv_data);

  // Copy encrypted data over to adv
  memcpy(&adv_pdu[3 + M_BD_ADDR_SIZE + IV_OFFSET + IV_LENGTH], encrypted, 16);

  // Call timer callback if present, it can be missing when manually called on boot
  if (aes_timerEventDoneCB != NULL)
  {
    aes_timerEventDoneCB(aes_timer_slot);
  }
}

void aes_callback_chain(void (*doneCB)())
{
  aes_timerEventDoneCB = doneCB;

  #ifdef LOG
  SEGGER_RTT_printf(0, "%u> AES CB: Got AES encrypt timer event\r\n", timer_get_seconds());
  #endif

  // Generate IV
  uint8_t iv[16];
  iv_data = random_get();
  memcpy(&iv, iv_data, 8);
  memcpy(&iv[8], iv_data, 8);

  #ifdef LOG
  SEGGER_RTT_printf(0, "%u> AES CB: Generated new IV: 0x%2.2x, 0x%2.2x, 0x%2.2x, 0x%2.2x, 0x%2.2x, 0x%2.2x, 0x%2.2x, 0x%2.2x\r\n", timer_get_seconds(), iv[0], iv[1], iv[2], iv[3], iv[4], iv[5], iv[6], iv[7]);
  #endif

  // Encrypt new value
  aes_encrypt(DEVICE_KEY, iv, DEVICE_ID, on_aes_encrypted);
}

void aes_callback_chain_issue()
{
    aes_callback_chain(NULL);
}

void aes_callback_chain_register()
{
    aes_timer_slot = timer_add(aes_callback_chain, 30);
}