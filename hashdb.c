#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "common_threads.h"
#include "rwlock.h" // Include the reader-writer locks header
#include <string.h>

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
int numRecords; // Number of records in the hash table

uint32_t jenkins_one_at_a_time_hash(const uint8_t* key, size_t length) {
  size_t i = 0;
  uint32_t hash = 0;
  while (i != length) {
    hash += key[i++];
    hash += hash << 10;
    hash ^= hash >> 6;
  }
  hash += hash << 3;
  hash ^= hash >> 11;
  hash += hash << 15;
  return hash;
}

void *processCommands(char *token) {
    
    // Check the command type and perform the corresponding operation
    if (strcmp(token, "insert") == 0) {
        // Extract name and salary from the command
        char *name = strtok(NULL, ",");
        uint32_t salary = atoi(strtok(NULL, ","));
        // Call insertRecord function with the extracted name and salary
        insertRecord(name, (uint32_t)salary);
    } else if (strcmp(token, "delete") == 0) {
        // Extract name from the command
        char *name = strtok(NULL, ",");
        // Call deleteRecord function with the extracted name
        deleteRecord(name);
    } else if (strcmp(token, "search") == 0) {
        // Extract name from the command
        char *name = strtok(NULL, ",");
        // Call searchRecord function with the extracted name
        searchRecord(name);
    } else if (strcmp(token, "print") == 0) {
        // Call printRecords function
        printRecords();
    }
    
    return NULL;
}

void readCommandsFromFile() {
    rwlock_init(&lock);

    FILE *file = fopen("commands.txt", "r");

    printf("Reading commands from file...\n");

    if (file == NULL) {
        return;
    }

    char line[1024];
    
    while (fgets(line, sizeof(line), file)) {
        char *token = strtok(line, ",");
        printf("Token: %s\n", token);

        if (strcmp(token, "threads") == 0) {
            token = strtok(NULL, ",");
            numThreads = atoi(token);
            hashTableSize = atoi(strtok(NULL, ","));
            printf("Hash Table Size: %d\n", hashTableSize);
            hashTable = (hashRecord **)malloc(hashTableSize * sizeof(hashRecord *));
            if (hashTable == NULL) {
                printf("Error allocating memory.\n");
                exit(EXIT_FAILURE);
            }
            for (int i = 0; i < hashTableSize; i++) {
                hashTable[i] = NULL;
            }
        } else {
            pthread_t thread;
            // Pass the command line to processCommands function
            Pthread_create(&thread, NULL, processCommands, token);
            Pthread_join(thread, NULL);
        }
    }

    fclose(file);
    printf("Finished reading commands.\n");
}

void insertRecord(char *name, uint32_t salary) {
    printf("Inserting record: %s %d\n", name, salary);
    // Compute the hash value of the given name
    uint32_t hashValue = jenkins_one_at_a_time_hash((const uint8_t*)name, strlen(name));

    // Check if resizing is needed
    if (hashTableSize < numRecords) {
        resizeHashTable(hashTableSize + 1);
    }

    // Acquire the write lock to ensure thread safety
    rwlock_acquire_writelock(&lock);


    // Allocate memory for the new record
    hashRecord *newRecord = (hashRecord *)malloc(sizeof(hashRecord));
    if (newRecord == NULL) {
        // Handle memory allocation failure
        fprintf(stderr, "Error: Failed to allocate memory for new record\n");
        rwlock_release_writelock(&lock); // Release the lock before returning
        return;
    }

    // Populate the new record
    newRecord->hash = hashValue;
    strcpy(newRecord->name, name);
    newRecord->salary = salary;
    newRecord->next = NULL;

    // Insert the new record into the hash table
    if (hashTable[hashValue] == NULL) {
        hashTable[hashValue] = newRecord;
    } else {
        // Traverse the linked list and insert at the end
        hashRecord *curr = hashTable[hashValue];
        while (curr->next != NULL) {
            curr = curr->next;
        }
        curr->next = newRecord;
    }

    numRecords++;
    // Release the write lock after insertion
    rwlock_release_writelock(&lock);
}

void deleteRecord(char *name) {
    // Implement deletion logic with proper synchronization

    // Computes the hash value of the key and obtains a writer lock.
    uint32_t hashValue = jenkins_one_at_a_time_hash(name, strlen(name));
    rwlock_acquire_writelock(&lock);
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
    rwlock_release_writelock(&lock);
}

void searchRecord(char *name) {
    // Implement search logic with proper synchronization

    // Computes the hash value of the key acquires a reader lock.
    uint32_t hashValue = jenkins_one_at_a_time_hash(name, strlen(name));
    rwlock_acquire_readlock(&lock);
    hashRecord *curr = hashTable[hashValue % hashTableSize];

    // Searches the linked list for the key. If the key is found, it returns the value.
    // Otherwise, it returns NULL.
    // Finally, it releases the read lock and returns.
    while (curr != NULL) {
        if (strcmp(curr->name, name) == 0) {
            printf("Record found: %s %d\n", curr->name, curr->salary);
            rwlock_release_readlock(&lock);
            return;
        }
        curr = curr->next;
    }

    // Print the record or "No Record Found" if the return is NULL.
    printf("No Record Found\n");
}

void printRecords() {
    // Implement printing logic with proper synchronization
}

void resizeHashTable(int newSize) {
    // Acquire a write lock to ensure thread safety
        printf("Resizing hash table to %d\n", newSize);

    rwlock_acquire_writelock(&lock);

    printf("Resizing hash table to %d\n", newSize);

    // Create a new hash table with the specified size
    hashRecord **newTable = (hashRecord **)malloc(newSize * sizeof(hashRecord *));
    if (newTable == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for new hash table\n");
        rwlock_release_writelock(&lock);
        return;
    }

    // Initialize the new hash table
    for (int i = 0; i < newSize; i++) {
        newTable[i] = NULL;
    }

    // Rehash all existing records into the new table
    for (int i = 0; i < hashTableSize; i++) {
        hashRecord *curr = hashTable[i];
        while (curr != NULL) {
            hashRecord *next = curr->next;
            uint32_t newIndex = curr->hash % newSize;
            curr->next = newTable[newIndex];
            newTable[newIndex] = curr;
            curr = next;
        }
    }

    // Free the memory of the old hash table
    free(hashTable);

    // Update hashTable and hashTableSize with the new values
    hashTable = newTable;
    hashTableSize = newSize;

    // Release the write lock
    rwlock_release_writelock(&lock);
}
