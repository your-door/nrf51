cmake_minimum_required(VERSION 3.14)
project(your-door-nrf LANGUAGES C ASM)

foreach(i RANGE 0 15)
  string(RANDOM LENGTH 2 ALPHABET 0123456789abcdef number)
  list(APPEND numbers_key "0x${number}")
endforeach()

list(JOIN numbers_key ", " DEVICE_KEY)
message(STATUS "Device Key: ${DEVICE_KEY}")

foreach(i RANGE 0 9)
  string(RANDOM LENGTH 2 ALPHABET 0123456789abcdef number)
  list(APPEND numbers_id "0x${number}")
endforeach()

list(JOIN numbers_id ", " DEVICE_ID)
message(STATUS "Device ID: ${DEVICE_ID}")

configure_file(src/settings.h.in src/settings.h @ONLY)

include("nrf5")
add_executable(${CMAKE_PROJECT_NAME}
  "src/main.c"
  "src/clock.c"
  "src/timer.c"
  "src/ble.c"
  "src/random.c"
  "src/aes.c"
  "src/ble_callback_chain.c"
  "src/aes_callback_chain.c"
  "src/pwr_mgmt.c"
  "src/reboot_counter.c"
  "src/rtt/SEGGER_RTT.c"
  "src/rtt/SEGGER_RTT_printf.c"
)

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC
  "src"
  "${CMAKE_CURRENT_BINARY_DIR}/src"
)

nrf5_target(${CMAKE_PROJECT_NAME})
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
  # Common
  nrf5_mdk
)