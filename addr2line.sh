#!/bin/bash

xtensa-esp32-elf-addr2line -pfiaC -e .pio/build/esp32dev/firmware.elf "$@"
