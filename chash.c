#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "common.h"
#include "common_threads.h"

// Function prototypes from hashdb.c
extern void readCommandsFromFile(char *filename);

// Main function
int main() {
    // Read commands from file
    readCommandsFromFile("commands.txt");

    return 0;
}
