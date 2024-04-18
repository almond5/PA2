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

typedef struct arg_struct
{
    char command[10];
    char name[50];
    uint32_t salary;
} ThreadArg;

rwlock_t lock;
hashRecord *hashTable;
int hashTableSize;
int numThreads;
int numAcquisitions;
int numReleases;

FILE *ofp;

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
    uint32_t hashValue = jenkins_one_at_a_time_hash((const uint8_t *)name, strlen(name));
    fprintf(ofp, "\nINSERT,%u,%s,%u", hashValue, name, salary);
    rwlock_acquire_writelock(&lock);
    fprintf(ofp, "\nWRITE LOCK ACQUIRED");
    numAcquisitions++;

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
                fprintf(ofp, "\nWRITE LOCK RELEASED");
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
    fprintf(ofp,"\nWRITE LOCK RELEASED");
    numReleases++;
}

void deleteRecord(char *name)
{
    fprintf(ofp, "\nDELETE,%s", name);
    uint32_t hashValue = jenkins_one_at_a_time_hash(name, strlen(name));

    rwlock_acquire_writelock(&lock);
    fprintf(ofp, "\nWRITE LOCK ACQUIRED");
    numAcquisitions++;

    hashRecord *curr = hashTable;

    if (curr == NULL)
    {
        rwlock_release_writelock(&lock);
        fprintf(ofp, "\nWRITE LOCK RELEASED");
        numReleases++;
        return;
    }

    if (curr->hash == hashValue)
    {
        hashTable = curr->next;
        free(curr);
        hashTableSize--;
        rwlock_release_writelock(&lock);
        fprintf(ofp, "\nWRITE LOCK RELEASED");
        numReleases++;
        return;
    }

    while (curr->next != NULL)
    {
        if (curr->next->hash == hashValue)
        {
            hashRecord *temp = curr->next;
            curr->next = curr->next->next;
            free(temp);
            hashTableSize--;
            break;
        }

        curr = curr->next;
    }

    rwlock_release_writelock(&lock);
    fprintf(ofp, "\nWRITE LOCK RELEASED");
    numReleases++;
}

hashRecord* searchRecord(char *name)
{
    fprintf(ofp, "\nSEARCH,%s", name);
    uint32_t hashValue = jenkins_one_at_a_time_hash(name, strlen(name));

    rwlock_acquire_readlock(&lock);
    fprintf(ofp, "\nREAD LOCK ACQUIRED");
    numAcquisitions++;

    hashRecord *curr = hashTable;

    while (curr != NULL)
    {
        if (curr->hash == hashValue)
        {
            rwlock_release_readlock(&lock);
            fprintf(ofp, "\nREAD LOCK RELEASED");
            numReleases++;
            return curr;
        }

        curr = curr->next;
    }

    rwlock_release_readlock(&lock);
    fprintf(ofp, "\nREAD LOCK RELEASED");
    numReleases++;
    return NULL;
}

void printRecords()
{
    rwlock_acquire_readlock(&lock);
    fprintf(ofp, "\nREAD LOCK ACQUIRED");
    numAcquisitions++;

    hashRecord *curr = hashTable;

    while (curr != NULL)
    {
        fprintf(ofp, "\n%u,%s,%u", curr->hash, curr->name, curr->salary);
        curr = curr->next;
    }

    rwlock_release_readlock(&lock);
    fprintf(ofp, "\nREAD LOCK RELEASED");
    numReleases++;
}

void *processCommands(void *args)
{
    ThreadArg *threadArgs = (ThreadArg *)args;
    char *token = threadArgs->command;
    char *name = threadArgs->name;
    uint32_t salary = threadArgs->salary;

    if (strcmp(token, "insert") == 0)
    {
        insertRecord(name, (uint32_t)salary);
    }
    else if (strcmp(token, "delete") == 0)
    {
        deleteRecord(name);
    }
    else if (strcmp(token, "search") == 0)
    {
        hashRecord* curr = searchRecord(name);
        if (curr == NULL)
            fprintf(ofp, "\nNo Record Found");
        else
            fprintf(ofp, "\n%u,%s,%u", curr->hash, curr->name, curr->salary);
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
    int i = 0;

    FILE *ifp = fopen("commands.txt", "r");

    ofp = fopen("output.txt", "w");

    if (ifp == NULL)
        return;

    char line[1024];
    pthread_t *threads;

    while (fgets(line, sizeof(line), ifp))
    {
        char *token = strtok(line, ",");

        if (strcmp(token, "threads") == 0)
        {
            token = strtok(NULL, ",");
            numThreads = atoi(token);
            threads = malloc(numThreads * sizeof(pthread_t));
            fprintf(ofp, "Running %d threads", numThreads);
        }
        else
        {
            ThreadArg *args = malloc(sizeof(ThreadArg));
            strcpy(args->command, token);
            strcpy(args->name, strtok(NULL, ","));
            args->salary = atoi(strtok(NULL, ","));
            Pthread_create(&threads[i], NULL, processCommands, args);
            Pthread_join(threads[i++], NULL)
        }
    }

    fprintf(ofp, "\nNumber of acquisitions: %d", numAcquisitions);
    fprintf(ofp, "\nNumber of releases: %d", numReleases);
    printRecords();

    fclose(ifp);
    fclose(ofp);
}