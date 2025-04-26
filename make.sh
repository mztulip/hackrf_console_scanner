#/bin/bash
# v2024.02.1 hackrf
# https://github.com/greatscottgadgets/hackrf/tags
gcc -std=gnu99  hackrf_console_scanner.c filter.c -I/home/tulip/hackrf-master/host/libhackrf/src -lm /home/tulip/hackrf-master/host/build/libhackrf/src/libhackrf.so -lpthread -o scan_x64
