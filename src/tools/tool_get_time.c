/*
 * Get Time Tool Implementation
 */

#include <stdio.h>
#include <time.h>
#include "tool_registry.h"

int tool_get_time(const char *params, char *result, size_t result_size) {
    (void)params; // Unused
    
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    
    char time_str[256];
    strftime(time_str, sizeof(time_str), 
             "Current date and time: %Y-%m-%d %H:%M:%S %Z (Week %U, Day %j)", 
             tm_info);
    
    strncpy(result, time_str, result_size - 1);
    return 0;
}