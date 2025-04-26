#/bin/bash
# v2024.02.1 hackrf
# https://github.com/greatscottgadgets/hackrf/tags
gcc -std=gnu99  hackrf_console_scanner.c filter.c  -lm libhackrf.so.0.9.0 -lpthread -o scan_x64
