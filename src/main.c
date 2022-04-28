/*
 * nRF 52820 spec v1.3: https://infocenter.nordicsemi.com/pdf/nRF52820_PS_v1.3.pdf
 *
 * Some definitions we use around here:
 * - Max voltage diff is allowed is 1200mV. CR3032 is 3000mV and nRF 51822 spec v3.3, 7, VDD defines a cut-off of 1800mV
 */
#include "app_error.h"
#include "app_timer.h"
#include "ble_advdata.h"
#include "ble_gap.h"
#include "nrf.h"
#include "nrf_crypto.h"
#include "nrf_crypto_rng.h"
#include "nrf_nvic.h"
#include "nrf_soc.h"
#include "nrf_sdm.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_pwr_mgmt.h"

/// Key for AES
static uint8_t DEVICE_KEY[32]  __attribute__ ((section(".uicr_key"))) = { 0x64, 0x28, 0xe6, 0xf2, 0x75, 0xfc, 0x91, 0xd9, 0x24, 0x5a, 0x67, 0xb2, 0xbd, 0xf8, 0xde, 0x77, 0x38, 0x6f, 0x9a, 0x2e, 0xdc, 0x11, 0x80, 0xcf, 0xd5, 0x92, 0x1e, 0x59, 0x58, 0xd2, 0xda, 0x34 };;
/// Unique device ID
static uint8_t DEVICE_ID[16]   __attribute__ ((section(".uicr_id")))  = { 0x6e, 0x0c, 0x5f, 0x71, 0x49, 0xa2, 0x15, 0x04, 0x18, 0x47, 0xec, 0x32, 0xe7, 0x11, 0x8b, 0x46 };

#define UPDATE_INTERVAL           5 * 60000
#define ADVERTISING_INTERVAL      MSEC_TO_UNITS(3000, UNIT_0_625_MS)        // We advertise every 3 seconds, the higher this is the longer is the batter life. One should balance TX_POWER alongside with this since it affects reaction time of the system
#define TX_POWER                  0                                         // We should use full low tx power to safe battery life, we might increase for range though

#define APP_BLE_CONN_CFG_TAG            1                                  /**< A tag identifying the SoftDevice BLE configuration. */
#define APP_DEVICE_TYPE                 0x02                               /**< 0x02 refers to Beacon. */

static uint8_t random_iv[16];                                              // Random IV for encryption

static ble_gap_adv_params_t m_adv_params;
static uint8_t              m_adv_handle = BLE_GAP_ADV_SET_HANDLE_NOT_SET; /**< Advertising handle used to identify an advertising set. */
static uint8_t              m_enc_advdata[2][BLE_GAP_ADV_SET_DATA_SIZE_MAX];  /**< Buffer for storing an encoded advertising set. */
static uint8_t              advdata_to_use = 0;

/**@brief Struct that contains pointers to the encoded advertising data. */
static ble_gap_adv_data_t m_adv_data =
{
    .adv_data =
    {
        .p_data = m_enc_advdata[0],
        .len    = BLE_GAP_ADV_SET_DATA_SIZE_MAX
    },
    .scan_rsp_data =
    {
        .p_data = NULL,
        .len    = 0
    }
};

static uint8_t m_beacon_info[16 + 8] =                    /**< Information advertised by the Beacon. */
{
    // Rest of the data is IV (16 bytes) and encrypted device id (8 bytes)
};

APP_TIMER_DEF(timer_update_iv);                                 //!< Timer for advertising individual slots.
APP_TIMER_DEF(timer_update_adv_data);                                 //!< Timer for advertising individual slots.
APP_TIMER_DEF(timer_update_ble_adv);                                 //!< Timer for advertising individual slots.

static void populate_new_iv0(void) 
{
  uint32_t err_code;

  // Generate new IV
  err_code = nrf_crypto_rng_vector_generate(random_iv, 8);
  APP_ERROR_CHECK(err_code);

  // Copy over the new IV
  memcpy(&m_beacon_info[0], random_iv, 8);

  // We need to copy over the first to the second half of the IV
  memcpy(&random_iv[8], random_iv, 8);
}

static void populate_new_iv(void * p_context)
{
  uint32_t err_code;
  populate_new_iv0();
  err_code = app_timer_start(timer_update_adv_data, APP_TIMER_TICKS(UPDATE_INTERVAL), p_context);
  APP_ERROR_CHECK(err_code);
}

static void populate_adv_data0(void) 
{
  nrf_crypto_aes_context_t encr_ctx;
  uint32_t err_code;

  // Setup AES context
  err_code = nrf_crypto_aes_init(&encr_ctx, &g_nrf_crypto_aes_cbc_256_info, NRF_CRYPTO_ENCRYPT);
  APP_ERROR_CHECK(err_code);

  // Set key for encryption  
  err_code = nrf_crypto_aes_key_set(&encr_ctx, DEVICE_KEY);
  APP_ERROR_CHECK(err_code);

  // Set IV for encryption
  err_code = nrf_crypto_aes_iv_set(&encr_ctx, random_iv);
  APP_ERROR_CHECK(err_code);

  // Now encrypt the device id with the key
  size_t len_out = 16;
  err_code = nrf_crypto_aes_finalize(&encr_ctx, (uint8_t *) DEVICE_ID, 16, &m_beacon_info[8], &len_out);
  APP_ERROR_CHECK(err_code);

  // Shut down AES
  err_code = nrf_crypto_aes_uninit(&encr_ctx);
  APP_ERROR_CHECK(err_code);
}

static void populate_adv_data(void * p_context) 
{
  uint32_t err_code;
  populate_adv_data0();
  err_code = app_timer_start(timer_update_ble_adv, APP_TIMER_TICKS(UPDATE_INTERVAL), p_context);
  APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing the Advertising functionality.
 *
 * @details Encodes the required advertising data and passes it to the stack.
 *          Also builds a structure to be passed to the stack when starting advertising.
 */
static void update_ble_advertising0(bool configure)
{
  uint32_t err_code;

  ble_advdata_t advdata;
  uint8_t       flags = BLE_GAP_ADV_FLAG_BR_EDR_NOT_SUPPORTED;

  ble_advdata_manuf_data_t manuf_specific_data;
  manuf_specific_data.company_identifier = 0x0059;
  manuf_specific_data.data.p_data = (uint8_t *) m_beacon_info;
  manuf_specific_data.data.size   = 24;

  // Build and set advertising data.
  memset(&advdata, 0, sizeof(advdata));
  advdata.name_type             = BLE_ADVDATA_NO_NAME;
  advdata.flags                 = flags;
  advdata.p_manuf_specific_data = &manuf_specific_data;

  if (configure) 
  {
    memset(&m_adv_params, 0, sizeof(m_adv_params));
    m_adv_params.properties.type = BLE_GAP_ADV_TYPE_NONCONNECTABLE_NONSCANNABLE_UNDIRECTED;
    m_adv_params.p_peer_addr     = NULL;    // Undirected advertisement.
    m_adv_params.filter_policy   = BLE_GAP_ADV_FP_ANY;
    m_adv_params.interval        = ADVERTISING_INTERVAL;
    m_adv_params.duration        = 0;       // Never time out.
  }
  
  ble_gap_adv_data_t new_adv_data = {
    .adv_data =
    {
        .p_data = m_enc_advdata[advdata_to_use],
        .len    = BLE_GAP_ADV_SET_DATA_SIZE_MAX
    },
    .scan_rsp_data =
    {
        .p_data = NULL,
        .len    = 0
    }
  };

  advdata_to_use++;
  if (advdata_to_use == 2) {
    advdata_to_use = 0;
  }

  err_code = ble_advdata_encode(&advdata, new_adv_data.adv_data.p_data, &new_adv_data.adv_data.len);
  APP_ERROR_CHECK(err_code);
  memcpy(&m_adv_data, &new_adv_data, sizeof(new_adv_data));
  err_code = sd_ble_gap_adv_set_configure(&m_adv_handle, &m_adv_data, (configure) ? &m_adv_params : (ble_gap_adv_params_t*) NULL);
  APP_ERROR_CHECK(err_code);

  if (configure) 
  {
    // Set TX power
    err_code = sd_ble_gap_tx_power_set(BLE_GAP_TX_POWER_ROLE_ADV, 0, TX_POWER);
    APP_ERROR_CHECK(err_code);
  }
}

static void update_ble_advertising(void * p_context)
{
  uint32_t err_code;
  update_ble_advertising0(false);
  err_code = app_timer_start(timer_update_iv, APP_TIMER_TICKS(UPDATE_INTERVAL), p_context);
  APP_ERROR_CHECK(err_code);
}

static void timer_init(void) 
{
  uint32_t err_code;
  err_code = app_timer_init();
  APP_ERROR_CHECK(err_code);
 
  err_code = app_timer_create(&timer_update_iv,
                              APP_TIMER_MODE_SINGLE_SHOT,
                              populate_new_iv);
  APP_ERROR_CHECK(err_code);

  err_code = app_timer_start(timer_update_iv, APP_TIMER_TICKS(UPDATE_INTERVAL), NULL);
  APP_ERROR_CHECK(err_code); 

  err_code = app_timer_create(&timer_update_adv_data,
                              APP_TIMER_MODE_SINGLE_SHOT,
                              populate_adv_data);
  APP_ERROR_CHECK(err_code);

  err_code = app_timer_create(&timer_update_ble_adv,
                              APP_TIMER_MODE_SINGLE_SHOT,
                              update_ble_advertising);
  APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
void ble_stack_init(void)
{
  uint32_t err_code;

  // This also enables the soft
  err_code = nrf_sdh_enable_request();
  APP_ERROR_CHECK(err_code);

  // Configure the BLE stack using the default settings.
  // Fetch the start address of the application RAM.
  uint32_t ram_start = 0;
  err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
  APP_ERROR_CHECK(err_code);

  // Enable BLE stack.
  err_code = nrf_sdh_ble_enable(&ram_start);
  APP_ERROR_CHECK(err_code);
}

int main(void) 
{
  uint32_t err_code;

  // Start crypto
  #if NRF_CRYPTO_BACKEND_MBEDTLS_ENABLED
    nrf_mem_init();
  #endif

  err_code = nrf_crypto_init();
  APP_ERROR_CHECK(err_code);

  // Startup BLE
  populate_new_iv0();
  populate_adv_data0();
  ble_stack_init();
  update_ble_advertising0(true);

  // Start ble advertising
  sd_ble_gap_adv_start(m_adv_handle, APP_BLE_CONN_CFG_TAG);

  // Power management
  nrf_pwr_mgmt_init();
  err_code = sd_power_dcdc_mode_set(NRF_POWER_DCDC_ENABLE);
  APP_ERROR_CHECK(err_code);
  err_code = sd_power_mode_set(NRF_POWER_MODE_LOWPWR);
  APP_ERROR_CHECK(err_code);

  // Update secret data in a timer
  timer_init();

  // Enter main loop.
  for (;;)
  {
    nrf_pwr_mgmt_run();
  }
}
