#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "common.h"
#include "common_threads.h"

extern void readCommandsFromFile(char *filename);

int main()
{
    readCommandsFromFile("commands.txt");
    return 0;
}
