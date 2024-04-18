#ifndef HASHDB_H
#define HASHDB_H

#include <stdint.h>
#include <pthread.h>

typedef struct hash_struct {
    uint32_t hash;
    char name[50];
    uint32_t salary;
    struct hash_struct* next;
} hashRecord;

typedef struct arg_struct
{
    char command[10];
    char name[50];
    uint32_t salary;
} ThreadArg;

extern pthread_rwlock_t lock;
extern hashRecord* hashTable;
extern int hashTableSize;
extern int numThreads;

void readCommandsFromFile(char* filename);
void* processCommands(char* token);
void insertRecord(char* name, uint32_t salary);
void deleteRecord(char* name);
hashRecord* searchRecord(char* name);
void printRecords();
void sortRecords();
uint32_t jenkins_one_at_a_time_hash(char* key, size_t len);

#endif
