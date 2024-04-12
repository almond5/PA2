#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "common_threads.h"
#include "rwlock.h"
#include <string.h>

typedef struct hash_struct
{
    uint32_t hash;
    char name[50];
    uint32_t salary;
    struct hash_struct *next;
} hashRecord;

rwlock_t lock;
hashRecord *hashTable; 
int hashTableSize;     
int numThreads;        
int numAcquisitions;
int numReleases;

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

void insertRecord(char *name, uint32_t salary)
{
    rwlock_acquire_writelock(&lock);
    printf("WRITE LOCK ACQUIRED\n");
    numAcquisitions++;

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
        {
            if (curr->hash == hashValue)
            {
                curr->salary = salary;
                strcpy(curr->name, name);
                rwlock_release_writelock(&lock);
                printf("WRITE LOCK RELEASED\n");
                numReleases++;
                return;
            }

            curr = curr->next;
        }

        curr->next = malloc(sizeof(hashRecord));
        curr = curr->next;
        curr->hash = hashValue;
        strcpy(curr->name, name);
        curr->salary = salary;
        curr->next = NULL;
        hashTableSize++;
    }

    rwlock_release_writelock(&lock);
    printf("WRITE LOCK RELEASED\n");
    numReleases++;
}

void deleteRecord(char *name)
{
    rwlock_acquire_readlock(&lock);
    printf("READ LOCK ACQUIRED\n");
    numAcquisitions++;

    // find the record to delete
    uint32_t hashValue = jenkins_one_at_a_time_hash(name, strlen(name));

    rwlock_release_readlock(&lock);
    printf("READ LOCK RELEASED\n");
    numReleases++;

    rwlock_acquire_writelock(&lock);
    printf("WRITE LOCK ACQUIRED\n");
    numAcquisitions++;

    // delete the record    

    rwlock_release_writelock(&lock);
    printf("WRITE LOCK RELEASED\n");
    numReleases++;
}

void searchRecord(char *name)
{
    rwlock_acquire_readlock(&lock);
    printf("READ LOCK ACQUIRED\n");
    numAcquisitions++;

    uint32_t hashValue = jenkins_one_at_a_time_hash(name, strlen(name));

    printf("Searching for record with name: %s\n", name);

    rwlock_release_readlock(&lock);
    printf("READ LOCK RELEASED\n");
    numReleases++;
}

void printRecords()
{
    rwlock_acquire_readlock(&lock);
    printf("READ LOCK ACQUIRED\n");
    numAcquisitions++;

    FILE *fp = fopen("output.txt", "w");

    hashRecord *curr = hashTable;

    while (curr != NULL)
    {
        fprintf(fp, "%d, %s, %d\n", curr->hash, curr->name, curr->salary);
        curr = curr->next;
    }

    rwlock_release_readlock(&lock);
    printf("READ LOCK RELEASED\n");
    numReleases++;
}

void printRecordsConsole()
{
    while (hashTable != NULL)
    {
        printf("%d, %s, %d\n", hashTable->hash, hashTable->name, hashTable->salary);
        hashTable = hashTable->next;
    }
}

void *processCommands(void *arg)
{
    char *token = (char *)arg;

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

    return NULL;
}

void readCommandsFromFile()
{
    rwlock_init(&lock);
    hashTableSize = 0;     
    numThreads = 0;        
    numAcquisitions = 0;
    numReleases = 0;

    FILE *file = fopen("commands.txt", "r");

    if (file == NULL)
    {
        printf("Error opening file.\n");
        return;
    }

    char line[1024];

    while (fgets(line, sizeof(line), file))
    {
        char *token = strtok(line, ",");

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

    printf("\nNumber of acquisitions: %d\n", numAcquisitions);
    printf("Number of releases: %d\n", numReleases);
    printf("Final Table: \n");
    printRecordsConsole();
}