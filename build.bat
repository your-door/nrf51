@echo off
cmake --build build
cmake --build build --target erase_all
cmake --build build --target flash_softdevice
cmake --build build --target flash