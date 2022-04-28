cmake_minimum_required(VERSION 3.14)
project(your-door-nrf LANGUAGES C ASM)

include("nrf5")
add_executable(${CMAKE_PROJECT_NAME}
  "src/main.c"
)

nrf5_target(${CMAKE_PROJECT_NAME})
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
  # Common
  nrf5_mdk
)