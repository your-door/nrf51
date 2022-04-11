/*
 * nRF 51822 spec v3.3: https://infocenter.nordicsemi.com/pdf/nRF51822_PS_v3.3.pdf
 *
 * Some definitions we use around here:
 * - Max voltage diff is allowed is 1200mV. CR3032 is 3000mV and nRF 51822 spec v3.3, 7, VDD defines a cut-off of 1800mV
 */

// Used for sleeping
#include "nrf_soc.h"
#include "nrf_nvic.h"

#define ADVERTISING_INTERVAL 3000     // We advertise every 3 seconds, the higher this is the longer is the batter life. One should balance TX_POWER alongside with this since it affects reaction time of the system
#define BATTERY_UPDATE_INTERVAL 60000 // We update battery status every minute
#define SYNC_CLOCK_INTERVAL 4000      // Update low frequency clock to fix drift of it (nRF 51822 spec v3.3, 8.1.6, fTOL)
#define VBAT_MAX_IN_MV 3000           // We use a CR3032 which should be 3V when fully charged
#define TX_POWER 4                    // We should use full 4dB transmit, we should check if we can lower it to get longer battery life

// RTC stuff
#define LFCLK_FREQUENCY           (32768UL)                               // Low freq according to (nRF 51822 spec v3.3, 3.6, LFCLK). This freq is used by RTC (nRF 51822 spec v3.3, 4.3)
#define RTC_FREQUENCY             (8UL)                                   // How often do we need to update RTC ticks. This defines how accurate the timer will be
#define COUNTER_PRESCALER         ((LFCLK_FREQUENCY / RTC_FREQUENCY) - 1) // How often does the low frequency need to tick before RTC ticks once

// Start low freq clock for RTC. Adds 1.3μA according to nRF 51822 spec v3.3, 8.1.6, IRC32k
static void lfclk_config(void)
{
  NRF_CLOCK->LFCLKSRC = (CLOCK_LFCLKSRC_SRC_RC << CLOCK_LFCLKSRC_SRC_Pos); // We use the internal RC reference, its enough for us
  NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;
  NRF_CLOCK->TASKS_LFCLKSTART = 1;

  // Wait for the low frequency clock to start
  while (NRF_CLOCK->EVENTS_LFCLKSTARTED == 0) {}
  NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;
}

// Enable RTC. This consumes 0.1μA according to nRF 51822 spec v3.3, 8.14, IRTC
static void rtc_config(void)
{
  NVIC_ClearPendingIRQ(RTC0_IRQn);
  NVIC_EnableIRQ(RTC0_IRQn);                                                      // Enable Interrupt for the RTC in the core

  NRF_RTC0->PRESCALER = COUNTER_PRESCALER;                                        // Set prescaler to a TICK of RTC_FREQUENCY
  NRF_RTC0->CC[0] = (SYNC_CLOCK_INTERVAL / 1000) * COUNTER_PRESCALER;             // We use compare 0 for low freq sync

  // Enable COMPARE0 event and COMPARE0 interrupt
  NRF_RTC0->EVTENSET = RTC_EVTENSET_COMPARE0_Msk;
  NRF_RTC0->INTENSET = RTC_INTENSET_COMPARE0_Msk;
}

// Sync low frequency RC osci with high frequency main
void sync_lowfreq(void) 
{
  if (NRF_CLOCK->EVENTS_CTTO)
  {
    NRF_CLOCK->EVENTS_CTTO = 0;
        
    // Wait for 16M XOSC
    while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);        
  
    // Do calibration          
    NRF_CLOCK->TASKS_CAL = 1;

    // Wait until calibration finished
    while (NRF_CLOCK->EVENTS_DONE == 0) {}

    // Restart timer
    NRF_CLOCK->TASKS_CTSTART = 1;
    NRF_CLOCK->EVENTS_DONE = 0;         
  }
}

void RTC0_IRQHandler(void)
{
  // This will trigger every COMPARE_COUNTERTIME seconds
  if (NRF_RTC0->EVENTS_COMPARE[0])
  {
    NRF_RTC0->EVENTS_COMPARE[0] = 0;     // Reset interrupt

    // Sync low freq
    sync_lowfreq();    
  }
}

// We are a simple peripheral which can give its battery status
/*BLEPeripheral blePeripheral = BLEPeripheral();
BLEService batteryService("180F");
BLEUnsignedCharCharacteristic batteryLevelCharacteristic("2A19", BLERead);*/

// Get battery level for BLE service
unsigned char getBatteryLevel(void)
{
  // Configure ADC
  NRF_ADC->CONFIG = (ADC_CONFIG_RES_8bit << ADC_CONFIG_RES_Pos) |                               // We only need 8 bit resolution for 1200mV           (this is also the fastest and less consuming method)
                    (ADC_CONFIG_INPSEL_SupplyOneThirdPrescaling << ADC_CONFIG_INPSEL_Pos) |     // Select VDD as source and one third scaling         (nRF 51822 spec v3.3, 8.12, VREF_VDD_LIM)
                    (ADC_CONFIG_REFSEL_VBG << ADC_CONFIG_REFSEL_Pos) |                          // We use VBG as reference (1200mV), not external     (nRF 51822 spec v3.3, 8.12, VREF_VBG)
                    (ADC_CONFIG_PSEL_Disabled << ADC_CONFIG_PSEL_Pos) |                         // We disable analog inputs
                    (ADC_CONFIG_EXTREFSEL_None << ADC_CONFIG_EXTREFSEL_Pos);                    // Same for external reference voltage
  NRF_ADC->EVENTS_END = 0;
  NRF_ADC->ENABLE = ADC_ENABLE_ENABLE_Enabled;

  NRF_ADC->EVENTS_END = 0; // Stop any running conversions.
  NRF_ADC->TASKS_START = 1;

  // Wait until we have a conversion
  while (!NRF_ADC->EVENTS_END)
  {
  }

  uint16_t vbg_in_mv = 1200; // Reference voltage
  uint8_t adc_max = 255;
  uint16_t vbat_current_in_mv = (NRF_ADC->RESULT * 3 * vbg_in_mv) / adc_max;

  // Disable ADC again to preserve battery
  NRF_ADC->EVENTS_END = 0;
  NRF_ADC->TASKS_STOP = 1;
  NRF_ADC->ENABLE = ADC_ENABLE_ENABLE_Disabled;

  return (unsigned char)((vbat_current_in_mv * 100) / VBAT_MAX_IN_MV);
}

/*void bleConnectedCallback(BLECentral &bleCentral)
{
  unsigned char batteryLevel = getBatteryLevel();
  if (batteryLevel > 100)
  {
    batteryLevel = 100;
  }
 batteryLevelCharacteristic.setValue(batteryLevel);
}*/

int main(void) 
{
  lfclk_config();
  rtc_config();
}

/*void setup()
{


  // Set BLE name
  blePeripheral.setDeviceName("Fencer-1");
  blePeripheral.setLocalName("Fencer-1");

  // Add battery GATT service
  blePeripheral.addAttribute(batteryService);
  blePeripheral.addAttribute(batteryLevelCharacteristic);
  blePeripheral.setAdvertisedServiceUuid(batteryService.uuid()); // We also use it to advertise, we have nothing else :D

  blePeripheral.setAdvertisingInterval(ADVERTISING_INTERVAL);

  // We update battery level on boot
  batteryLevelCharacteristic.setValue(getBatteryLevel());

  // Start BLE peripheral
  blePeripheral.begin();
  blePeripheral.setTxPower(TX_POWER);

  // enable low power mode without interrupt
  sd_power_mode_set(NRF_POWER_MODE_LOWPWR);
}

void loop()
{
  // Enter Low power mode
  sd_app_evt_wait();
  // Exit Low power mode
  
  // Clear IRQ flag to be able to go to sleep if nothing happens in between
  sd_nvic_ClearPendingIRQ(SWI2_IRQn);

  // poll peripheral
  blePeripheral.poll();
}

void updateAdvertisingScanData(const char* localName, uint8_t txPower, uint32_t counterValue)
{
  unsigned char srData[31];
  unsigned char srDataLen = 0;
  int scanDataSize = 3;
  BLEEirData scanData[scanDataSize];

  // - Local name
  scanData[0].length = strlen(localName);
  scanData[0].type = 0x09;
  memcpy(scanData[0].data, localName, scanData[0].length);

  // - Tx Power
  scanData[1].length = 1;
  scanData[1].type = 0x0A;
  scanData[1].data[0] = txPower;

  // - Manufacturer Data
  scanData[2].length = 2 + 4;
  scanData[2].type = 0xFF;
  // Manufacturer ID
  scanData[2].data[0] = 0xFF;
  scanData[2].data[1] = 0xFF;
  // Manufacturer data content
  scanData[2].data[2] = counterValue & 0xFF;
  scanData[2].data[3] = (counterValue >> 8) & 0xFF;
  scanData[2].data[4] = (counterValue >> 16) & 0xFF;
  scanData[2].data[5] = (counterValue >> 24) & 0xFF;

  if (scanDataSize && scanData)
  {
    for (int i = 0; i < scanDataSize; i++)
    {
      srData[srDataLen + 0] = scanData[i].length + 1;
      srData[srDataLen + 1] = scanData[i].type;
      srDataLen += 2;

      memcpy(&srData[srDataLen], scanData[i].data, scanData[i].length);

      srDataLen += scanData[i].length;
    }
  }

  // - Sets only avertising scan data
  sd_ble_gap_adv_data_set(NULL, 0, srData, srDataLen);
}*/