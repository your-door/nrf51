## CMake Command

### Mac
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE="./CMake/arm-none-eabi.cmake" -DTOOLCHAIN_PREFIX="/opt/homebrew/opt/arm-none-eabi-gcc/gcc" -DNRF5_SDK_PATH="./sdk/nRF5_SDK_17.1.0_ddde560" -DNRF5_TARGET="nrf52820" -DNRF5_SOFTDEVICE_VARIANT="s112" -DNRF5_SDKCONFIG_PATH="./include/" -DCMAKE_BUILD_TYPE="Release"

### Windows
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE="./CMake/arm-none-eabi.cmake" -DTOOLCHAIN_PREFIX="C:/Program Files (x86)/GNU Arm Embedded Toolchain/10 2021.10" -DNRF5_SDK_PATH="./sdk/nRF5_SDK_17.1.0_ddde560" -DNRF5_TARGET="nrf52820" -DNRF5_SOFTDEVICE_VARIANT="s112" -DNRF5_SDKCONFIG_PATH="./include/" -G "Unix Makefiles" -DCMAKE_BUILD_TYPE="Release"