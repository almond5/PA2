#include <cstdint>
#include <cstdlib>
#include <pthread.h>
#include "common.h"
#include "common_threads.h"
#include "rwlocks.h" // Include the reader-writer locks header

// Define the hash table node structure
typedef struct hash_struct {
    uint32_t hash;
    char name[50];
    uint32_t salary;
    struct hash_struct *next;
} hashRecord;

// Define global variables and data structures
rwlock_t lock; // Reader-writer lock for synchronization
hashRecord **hashTable; // Pointer to hash table array
int hashTableSize = 0; // Current size of the hash table
int numThreads; // Number of threads to create

// Function prototypes from chash.c
extern void *processCommands(void *args);

// Function to read commands from file
void readCommandsFromFile(char *filename) {
    // Implement reading commands from the file and parsing
    // Set numThreads accordingly
    // Once commands are parsed, create threads to process them
    pthread_t threads[numThreads];
    for (int i = 0; i < numThreads; i++) {
        Pthread_create(&threads[i], NULL, processCommands, NULL);
    }

    // Join threads
    for (int i = 0; i < numThreads; i++) {
        Pthread_join(threads[i], NULL);
    }

    // Cleanup
    free(hashTable);
}

// Function to process commands
void *processCommands(void *args) {
    // Implement command processing based on the parsed commands
    return NULL;
}

// Other functions for hash table operations
void insertRecord(char *name, uint32_t salary) {
    // Implement insertion logic with proper synchronization

    //  First computes the hash value of the given name (the key)
    uint32_t hashValue = hash(name);

    //  Acquires the write lock that protects the list and searches the linked list for the hash. 
    rwlock_acquire_write(&lock);    
    hashRecord *newRecord = (hashRecord *)malloc(sizeof(hashRecord));
    newRecord->hash = hashValue;
    strcpy(newRecord->name, name);
    newRecord->salary = salary;
    newRecord->next = NULL;

    // If the hash is found, it updates the value. 
    // Otherwise, it creates a new node and inserts it in the list.
    if (hashTable[hashValue % hashTableSize] == NULL) {
        hashTable[hashValue % hashTableSize] = newRecord;
    } else {
        hashRecord *curr = hashTable[hashValue % hashTableSize];
        while (curr->next != NULL) {
            curr = curr->next;
        }
        curr->next = newRecord;
    }

    // Finally, it releases the write lock and returns.
    rwlock_release_write(&lock);
}

void deleteRecord(char *name) {
    // Implement deletion logic with proper synchronization

    // Computes the hash value of the key and obtains a writer lock.
    uint32_t hashValue = hash(name);
    rwlock_acquire_write(&lock);
    hashRecord *prev = NULL;
    hashRecord *curr = hashTable[hashValue % hashTableSize];

    //  If the key is found, it removes the node from the list and frees the memory.  
    // Otherwise, it does nothing. 
    while (curr != NULL) {
        if (strcmp(curr->name, name) == 0) {
            if (prev == NULL) {
                hashTable[hashValue % hashTableSize] = curr->next;
            } else {
                prev->next = curr->next;
            }
            free(curr);
            break;
        }
        prev = curr;
        curr = curr->next;
    }

    // Finally, it releases the write lock and returns.
    rwlock_release_write(&lock);
}

void searchRecord(char *name) {
    // Implement search logic with proper synchronization
}

void printRecords() {
    // Implement printing logic with proper synchronization
}

void resizeHashTable(int newSize) {
    // Implement resizing logic with proper synchronization
}