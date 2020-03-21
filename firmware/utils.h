#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

void my_error_function(int code, const char* filename, const int line);

#define ERROR_CHECK(x,y) \
    do { \
        if((x) != (y)) \
        { \
            my_error_function(x,__FILE__,__LINE__); \
        } \
    } while(0); \


void utils_init();

void utils_start_execution_timer();
uint32_t utils_stop_execution_timer();



#endif // UTILS_H
