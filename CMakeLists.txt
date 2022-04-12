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
  # Application
  nrf5_app_error
  nrf5_app_util_platform
  # BLE
  nrf5_sdh
  nrf5_ble_common
  # Crypto
  nrf5_crypto
)