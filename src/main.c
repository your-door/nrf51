/*
 * nRF 52820 spec v1.3: https://infocenter.nordicsemi.com/pdf/nRF52820_PS_v1.3.pdf
 *
 * Some definitions we use around here:
 * - Max voltage diff is allowed is 1200mV. CR3032 is 3000mV and nRF 51822 spec v3.3, 7, VDD defines a cut-off of 1800mV
 */

// Used for sleeping
#include "nrf.h"
#include "nrf_soc.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_nvic.h"
#include "ble_advdata.h"
#include "ble_gap.h"

#define ADVERTISING_INTERVAL MSEC_TO_UNITS(3000, UNIT_0_625_MS)     // We advertise every 3 seconds, the higher this is the longer is the batter life. One should balance TX_POWER alongside with this since it affects reaction time of the system
#define SYNC_CLOCK_INTERVAL 4000                                    // Update low frequency clock to fix drift of it (nRF 51822 spec v3.3, 8.1.6, fTOL)
#define TX_POWER -40                                                // We should use full low tx power to safe battery life, we might increase for range though

// RTC stuff
#define LFCLK_FREQUENCY           (32768UL)                               // Low freq according to (nRF 51822 spec v3.3, 3.6, LFCLK). This freq is used by RTC (nRF 51822 spec v3.3, 4.3)
#define RTC_FREQUENCY             (8UL)                                   // How often do we need to update RTC ticks. This defines how accurate the timer will be
#define COUNTER_PRESCALER         ((LFCLK_FREQUENCY / RTC_FREQUENCY) - 1) // How often does the low frequency need to tick before RTC ticks once

#define APP_BLE_CONN_CFG_TAG            1                                  /**< A tag identifying the SoftDevice BLE configuration. */

#define APP_BEACON_INFO_LENGTH          0x17                               /**< Total length of information advertised by the Beacon. */
#define APP_ADV_DATA_LENGTH             0x15                               /**< Length of manufacturer specific data in the advertisement. */
#define APP_DEVICE_TYPE                 0x02                               /**< 0x02 refers to Beacon. */
#define APP_MEASURED_RSSI               0xC3                               /**< The Beacon's measured RSSI at 1 meter distance in dBm. */
#define APP_COMPANY_IDENTIFIER          0x0059                             /**< Company identifier for Nordic Semiconductor ASA. as per www.bluetooth.org. */
#define APP_MAJOR_VALUE                 0x01, 0x02                         /**< Major value used to identify Beacons. */
#define APP_MINOR_VALUE                 0x03, 0x04                         /**< Minor value used to identify Beacons. */
#define APP_BEACON_UUID                 0x01, 0x12, 0x23, 0x34, \
                                        0x45, 0x56, 0x67, 0x78, \
                                        0x89, 0x9a, 0xab, 0xbc, \
                                        0xcd, 0xde, 0xef, 0xf0             /**< Proprietary UUID for Beacon. */

static ble_gap_adv_params_t m_adv_params;                                  /**< Parameters to be passed to the stack when starting advertising. */
static uint8_t              m_adv_handle = BLE_GAP_ADV_SET_HANDLE_NOT_SET; /**< Advertising handle used to identify an advertising set. */
static uint8_t              m_enc_advdata[BLE_GAP_ADV_SET_DATA_SIZE_MAX];  /**< Buffer for storing an encoded advertising set. */

/**@brief Struct that contains pointers to the encoded advertising data. */
static ble_gap_adv_data_t m_adv_data =
{
    .adv_data =
    {
        .p_data = m_enc_advdata,
        .len    = BLE_GAP_ADV_SET_DATA_SIZE_MAX
    },
    .scan_rsp_data =
    {
        .p_data = NULL,
        .len    = 0

    }
};

static uint8_t m_beacon_info[APP_BEACON_INFO_LENGTH] =                    /**< Information advertised by the Beacon. */
{
    APP_DEVICE_TYPE,     // Manufacturer specific information. Specifies the device type in this
                         // implementation.
    APP_ADV_DATA_LENGTH, // Manufacturer specific information. Specifies the length of the
                         // manufacturer specific data in this implementation.
    APP_BEACON_UUID,     // 128 bit UUID value.
    APP_MAJOR_VALUE,     // Major arbitrary value that can be used to distinguish between Beacons.
    APP_MINOR_VALUE,     // Minor arbitrary value that can be used to distinguish between Beacons.
    APP_MEASURED_RSSI    // Manufacturer specific information. The Beacon's measured TX power in
                         // this implementation.
};

/**@brief Function for initializing the Advertising functionality.
 *
 * @details Encodes the required advertising data and passes it to the stack.
 *          Also builds a structure to be passed to the stack when starting advertising.
 */
static void advertising_init(void)
{
    ble_advdata_t advdata;
    uint8_t       flags = BLE_GAP_ADV_FLAG_BR_EDR_NOT_SUPPORTED;

    ble_advdata_manuf_data_t manuf_specific_data;

    manuf_specific_data.company_identifier = APP_COMPANY_IDENTIFIER;

    manuf_specific_data.data.p_data = (uint8_t *) m_beacon_info;
    manuf_specific_data.data.size   = APP_BEACON_INFO_LENGTH;

    // Build and set advertising data.
    memset(&advdata, 0, sizeof(advdata));

    advdata.name_type             = BLE_ADVDATA_NO_NAME;
    advdata.flags                 = flags;
    advdata.p_manuf_specific_data = &manuf_specific_data;

    // Initialize advertising parameters (used when starting advertising).
    memset(&m_adv_params, 0, sizeof(m_adv_params));

    m_adv_params.properties.type = BLE_GAP_ADV_TYPE_NONCONNECTABLE_NONSCANNABLE_UNDIRECTED;
    m_adv_params.p_peer_addr     = NULL;    // Undirected advertisement.
    m_adv_params.filter_policy   = BLE_GAP_ADV_FP_ANY;
    m_adv_params.interval        = ADVERTISING_INTERVAL;
    m_adv_params.duration        = 0;       // Never time out.

    ble_advdata_encode(&advdata, m_adv_data.adv_data.p_data, &m_adv_data.adv_data.len);
    sd_ble_gap_adv_set_configure(&m_adv_handle, &m_adv_data, &m_adv_params);
    sd_ble_gap_tx_power_set(BLE_GAP_TX_POWER_ROLE_ADV, NULL, TX_POWER);
}

/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
void ble_stack_init(void)
{
    nrf_sdh_enable_request();

    // Configure the BLE stack using the default settings.
    // Fetch the start address of the application RAM.
    uint32_t ram_start = 0;
    nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);

    // Enable BLE stack.
    nrf_sdh_ble_enable(&ram_start);
}

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

  /*
   * 
   */
  NRF_RTC0->INTENSET = RTC_INTENSET_COMPARE0_Msk;
}

// Sync low frequency RC osci with high frequency main
void sync_lowfreq(void) 
{
  if (NRF_CLOCK->EVENTS_CTTO)
  {
    NRF_CLOCK->EVENTS_CTTO = 0;
        
    // Wait for 16M XOSC
    NRF_CLOCK->TASKS_HFCLKSTART = 1;
    while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);        
  
    // Do calibration          
    NRF_CLOCK->TASKS_CAL = 1;

    // Wait until calibration finished
    while (NRF_CLOCK->EVENTS_DONE == 0);

    // Restart timer
    NRF_CLOCK->TASKS_CTSTART = 1;
    NRF_CLOCK->EVENTS_DONE = 0;    

    // Turn off HFCLK
    NRF_CLOCK->TASKS_HFCLKSTOP = 1;     
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

int main(void) 
{
  // Configure clocks and RTC
  lfclk_config();
  rtc_config();

  // Start RTC
  NRF_RTC0->TASKS_START = 1;

  // Power management
  sd_power_dcdc_mode_set(NRF_POWER_DCDC_ENABLE);
  sd_power_mode_set(NRF_POWER_MODE_LOWPWR);

  ble_stack_init();
  advertising_init();

  // Start ble advertising
  sd_ble_gap_adv_start(m_adv_handle, APP_BLE_CONN_CFG_TAG);

  // Enter main loop.
  for (;; )
  {
    sd_app_evt_wait();
  }
}
