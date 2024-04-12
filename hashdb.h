#ifndef HASHDB_H
#define HASHDB_H

#include <stdint.h>
#include <pthread.h>

// Define the hash table node structure
typedef struct hash_struct {
    uint32_t hash;
    char name[50];
    uint32_t salary;
    struct hash_struct* next;
} hashRecord;

// Global variables and data structures
extern pthread_rwlock_t lock;
extern hashRecord** hashTable;
extern int hashTableSize;
extern int numThreads;

// Function prototypes
void readCommandsFromFile(char* filename);
void* processCommands(char* token);
void insertRecord(char* name, uint32_t salary);
void deleteRecord(char* name);
void searchRecord(char* name);
void printRecords();
void resizeHashTable(int newSize);
uint32_t jenkins_one_at_a_time_hash(char* key, size_t len);

#endif /* CONCURRENT_HASH_TABLE_H */
