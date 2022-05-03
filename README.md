# Technical Documentation

## BLE ADV Data

A tag issues a ADV with MD every 3 seconds on channel 37,38 and 39. The MD contains 8 bytes IV and 16 bytes encrypted AES data. The device addr is static inside ADV for key lookups.

BLE MD Data format:
+-------+----------+-------------------------------------------------+
| Bytes |   Field  |                   Description                   |
|       |   Name   |                                                 |
+=======+==========+=================================================+
|  0:7  |    IV    | Random IV. The tag generates 8 bytes and doubles|
|       |          | them to fill 16 bytes for the AES encryption    |
+-------+----------+-------------------------------------------------+
|  8:24 | encrypted| Data is ECB encrypted and XOR'd with the given  |
|       | data     | IV. Decrypted data format is documented below   |
+-------+----------+-------------------------------------------------+

Decrypted Data format:
+-------+----------+-------------------------------------------------+
| Bytes |   Field  |                   Description                   |
|       |   Name   |                                                 |
+=======+==========+=================================================+
|  0:9  | Device   | Unique device identifier                        |
|       | Ident    |                                                 |
+-------+----------+-------------------------------------------------+
| 10:11 | Reboot   | Counter for how often this tag has been reset   |
|       | counter  |                                                 |
+-------+----------+-------------------------------------------------+
| 12:15 | uint32   | Time in seconds since last boot                 |
|       | timestamp|                                                 |
+-------+----------+-------------------------------------------------+

## Security ideas

Due to the nrf52 only having a hardware encrypter for ECB we emulate CBC which restarts after one block (since we only have 16 bytes). 
For this we XOR the plaintext with the IV provided and ECB encrypt it afterwards which should have the same effect as AES CBC. The server which
receives the encrypted data has to check if it knows the correct key (derived from the static advertise addr), if not it should be ignored.

If it has a matching key it needs to decrypt the data and check if the device ident matches the stored ident on server side and if the reboot counter 
and timestamp are older than already known. If the latest known state is newer or equal than the presented state the event is ignored. This prevents
replay attacks due to sniffing.

The tag also rolls its IV every 30 seconds to ensure that key retrieval due to sniffing enough examples is highly unlikely.

# Build

## CMake Commands

### MAC
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE="./CMake/arm-none-eabi.cmake" -DTOOLCHAIN_PREFIX="/opt/homebrew/Cellar/arm-none-eabi-gcc/10.3-2021.07/gcc" -DNRF5_MDK_PATH="./sdk/nrf_mdk_8_46_0_gcc_bsdlicense" -DNRF5_TARGET="nrf52820_xxaa" -DNRF5_SOFTDEVICE_VARIANT="blank" -G "Unix Makefiles" -DCMAKE_BUILD_TYPE="Debug" -DCMSIS_PATH="./sdk/CMSIS_5-5.8.0"  -DNRF5_LINKER_SCRIPT="linker.ld" -DNRF5_STACK_SIZE="1024" -DNRF5_HEAP_SIZE="1024"

### Windows
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE="./CMake/arm-none-eabi.cmake" -DTOOLCHAIN_PREFIX="C:/Program Files (x86)/GNU Arm Embedded Toolchain/10 2021.10" -DNRF5_MDK_PATH="./sdk/nrf_mdk_8_46_0_gcc_bsdlicense" -DNRF5_TARGET="nrf52820_xxaa" -DNRF5_SOFTDEVICE_VARIANT="blank" -G "Unix Makefiles" -DCMAKE_BUILD_TYPE="Debug" -DCMSIS_PATH="./sdk/CMSIS_5-5.8.0"  -DNRF5_LINKER_SCRIPT="linker.ld" -DNRF5_STACK_SIZE="1024" -DNRF5_HEAP_SIZE="1024"