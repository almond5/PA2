#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "common_threads.h"
#include "rwlock.h" // Include the reader-writer locks header
#include <string.h>

// Define the hash table node structure
typedef struct hash_struct
{
    uint32_t hash;
    char name[50];
    uint32_t salary;
    struct hash_struct *next;
} hashRecord;

// Define global variables and data structures
rwlock_t lock;         // Reader-writer lock for synchronization
hashRecord *hashTable; // Pointer to hash table array
int hashTableSize;     // Current size of the hash table
int numThreads;        // Number of threads to create

uint32_t jenkins_one_at_a_time_hash(const uint8_t *key, size_t length)
{
    size_t i = 0;
    uint32_t hash = 0;
    while (i != length)
    {
        hash += key[i++];
        hash += hash << 10;
        hash ^= hash >> 6;
    }
    hash += hash << 3;
    hash ^= hash >> 11;
    hash += hash << 15;
    return hash;
}

void *processCommands(char *token)
{
    if (strcmp(token, "insert") == 0)
    {
        char *name = strtok(NULL, ",");
        uint32_t salary = atoi(strtok(NULL, ","));
        printf("INSERT, %s, %d\n", name, salary);
        insertRecord(name, (uint32_t)salary);
    }
    else if (strcmp(token, "delete") == 0)
    {
        char *name = strtok(NULL, ",");
        printf("DELETE, %s\n", name);
        deleteRecord(name);
    }
    else if (strcmp(token, "search") == 0)
    {
        char *name = strtok(NULL, ",");
        printf("Search, %s\n", name);
        searchRecord(name);
    }
    else if (strcmp(token, "print") == 0)
    {
        printRecords();
    }
}

void readCommandsFromFile()
{
    rwlock_init(&lock);

    FILE *file = fopen("commands.txt", "r");

    printf("Reading commands from file...\n");

    if (file == NULL)
    {
        printf("Error opening file.\n");
        return;
    }

    char line[1024];

    while (fgets(line, sizeof(line), file))
    {
        char *token = strtok(line, ",");
        printf("Token: %s\n", token);

        if (strcmp(token, "threads") == 0)
        {
            token = strtok(NULL, ",");
            numThreads = atoi(token);
        }
        else
        {
            pthread_t thread;
            // Pass the command line to processCommands function
            Pthread_create(&thread, NULL, processCommands, token);
            Pthread_join(thread, NULL);
        }
    }

    fclose(file);
    printf("Finished reading commands.\n");
}

void insertRecord(char *name, uint32_t salary)
{
    rwlock_acquire_writelock(&lock);
    printf("WRITE LOCK ACQUIRED\n");

    uint32_t hashValue = jenkins_one_at_a_time_hash((const uint8_t *)name, strlen(name));

    if (hashTableSize == 0)
    {
        hashTable = malloc(sizeof(hashRecord));
        hashTable->hash = hashValue;
        strcpy(hashTable->name, name);
        hashTable->salary = salary;
        hashTable->next = NULL;
        hashTableSize++;
    }
    else
    {
        hashRecord *curr = hashTable;

        while (curr->next != NULL)
            curr = curr->next;

        curr->next = malloc(sizeof(hashRecord));
        curr = curr->next;
        curr->hash = hashValue;
        strcpy(curr->name, name);
        curr->salary = salary;
        curr->next = NULL;
        hashTableSize++;
    }

    // Release the write lock after insertion
    rwlock_release_writelock(&lock);
    printf("WRITE LOCK RELEASED\n");
}

void deleteRecord(char *name)
{
    // Implement deletion logic with proper synchronization

    // Computes the hash value of the key and obtains a writer lock.
    uint32_t hashValue = jenkins_one_at_a_time_hash(name, strlen(name));
    rwlock_acquire_writelock(&lock);
    rwlock_release_writelock(&lock);
}

void searchRecord(char *name)
{
    rwlock_acquire_readlock(&lock);
    printf("READ LOCK ACQUIRED\n");
    uint32_t hashValue = jenkins_one_at_a_time_hash(name, strlen(name));

    print("Searching for record with name: %s\n", name);
    rwlock_release_readlock(&lock);
}

void printRecords()
{
    rwlock_acquire_readlock(&lock);
    printf("Printing records...\n");
    while (hashTable != NULL)
    {
        printf("Name: %s, Salary: %d\n", hashTable->name, hashTable->salary);
        hashTable = hashTable->next;
    }
    rwlock_release_readlock(&lock);
}

void resizeHashTable(int newSize)
{
    // Acquire a write lock to ensure thread safety
    rwlock_acquire_writelock(&lock);
    printf("WRITE LOCK ACQUIRED\n");

    // Release the write lock
    rwlock_release_writelock(&lock);
    printf("WRITE LOCK RELEASED\n");
}
