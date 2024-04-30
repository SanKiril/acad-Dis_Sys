#!/bin/bash

rm -f server
clear
gcc -o server server.c devtools.c
./server -p 5000
