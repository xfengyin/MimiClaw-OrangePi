/*
 * MimiClaw-OrangePi Main Entry Point
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#define VERSION "1.0.0"

static volatile int running = 1;

void signal_handler(int sig) {
    printf("\nShutting down...\n");
    running = 0;
}

int main(int argc, char *argv[]) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    printf("MimiClaw-OrangePi v%s starting...\n", VERSION);
    printf("AI Assistant for OrangePi Zero3\n\n");
    
    // Main loop
    while (running) {
        sleep(1);
    }
    
    printf("Goodbye!\n");
    return 0;
}
