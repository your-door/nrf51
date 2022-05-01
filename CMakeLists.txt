cmake_minimum_required(VERSION 3.14)
project(your-door-nrf LANGUAGES C ASM)

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
  "src/callback.c"
  "src/rtt/SEGGER_RTT.c"
  "src/rtt/SEGGER_RTT_printf.c"
)

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC
  "src"
)

nrf5_target(${CMAKE_PROJECT_NAME})
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
  # Common
  nrf5_mdk
)