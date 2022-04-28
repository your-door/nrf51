cmake_minimum_required(VERSION 3.14)
project(your-door-nrf52 LANGUAGES C ASM)

include("nrf5")
add_executable(${CMAKE_PROJECT_NAME}
  "src/main.c"
)

nrf5_target(${CMAKE_PROJECT_NAME})
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
  # Common
  nrf5_mdk
  nrf5_section
  nrf5_atflags
  nrf5_atomic
  nrf5_soc
  nrf5_queue
  nrf5_pwr_mgmt
  # Application
  nrf5_app_timer
  nrf5_app_error
  nrf5_app_util_platform
  # BLE
  nrf5_sdh
  nrf5_ble_common
  # Crypto
  nrf5_nrfx_rng
  nrf5_ext_mbedtls
  nrf5_ext_mbedcrypto
  nrf5_crypto_mbedtls_backend
  nrf5_crypto_nrf_hw_backend
  nrf5_mem_manager # Needed for mbed TLS
  nrf5_drv_rng
  nrf5_crypto
)