#include "devtools.h"
#include <stdio.h>

int DEBUG_NUMBER = 0;
int print_debugging_point() {
    printf("DEBUGGING POINT %d\n", DEBUG_NUMBER);
    DEBUG_NUMBER++;
    return DEBUG_NUMBER - 1;
}