## CMake Command

### Windows
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE="./CMake/arm-none-eabi.cmake" -DTOOLCHAIN_PREFIX="C:/Program Files (x86)/GNU Arm Embedded Toolchain/10 2021.10" -DNRF5_MDK_PATH="./sdk/nrf_mdk_8_46_0_gcc_bsdlicense" -DNRF5_TARGET="nrf52820_xxaa" -DNRF5_SOFTDEVICE_VARIANT="blank" -G "Unix Makefiles" -DCMAKE_BUILD_TYPE="Debug" -DCMSIS_PATH="./sdk/CMSIS_5-5.8.0"  -DNRF5_LINKER_SCRIPT="linker.ld" -DNRF5_STACK_SIZE="1024" -DNRF5_HEAP_SIZE="1024"