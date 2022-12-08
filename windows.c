#include <stdint.h>
#include <stdio.h>
#include <Windows.h>
#include <pthread.h>
#include <unistd.h>

extern char isPrime(uint64_t n) __attribute((naked)) __attribute__((sysv_abi)) __attribute__((hot)) __attribute__((no_split_stack)) __attribute__((no_stack_limit));
extern uint64_t check_block(uint64_t start, uint64_t count, uint64_t array[]) __attribute((naked)) __attribute((sysv_abi)) __attribute__((hot)) __attribute__((no_split_stack)) __attribute__((no_stack_limit));

int64_t GetTimeMs64()
{
    FILETIME ft;
    LARGE_INTEGER li;
    uint64_t ret;

    /* Get the amount of 100 nano seconds intervals elapsed since January 1, 1601 (UTC) and copy it
    * to a LARGE_INTEGER structure. */
    GetSystemTimeAsFileTime(&ft);
    li.LowPart = ft.dwLowDateTime;
    li.HighPart = ft.dwHighDateTime;

    ret = li.QuadPart;
    ret -= 116444736000000000LL; /* Convert from file time to UNIX epoch time. */
    ret /= 10000; /* From 100 nano seconds (10^-7) to 1 millisecond (10^-3) intervals */

    return ret;
}
int64_t GetTimeUs64()
{
    FILETIME ft;
    LARGE_INTEGER li;
    uint64_t ret;

    /* Get the amount of 100 nano seconds intervals elapsed since January 1, 1601 (UTC) and copy it
    * to a LARGE_INTEGER structure. */
    GetSystemTimeAsFileTime(&ft);
    li.LowPart = ft.dwLowDateTime;
    li.HighPart = ft.dwHighDateTime;

    ret = li.QuadPart;
    ret -= 116444736000000000LL; /* Convert from file time to UNIX epoch time. */
    ret /= 10; 

    return ret;
}


#define maxUnsavedPrimes 1024*1024*8 // n/2 * 8 * threadCount = maxmemreq
#define threadCount 4
#define FIRST_NUMBER 0xEFFFFFF
#define HOW_MANY_NUMBERS 0x78000000

struct primeGenTaskInfo{
    uint64_t start;
    uint64_t count;
    uint64_t threadId;
    uint64_t* storage;
    uint64_t biggestPrime;
    uint64_t primesFound;
    uint64_t timeSpentLastWork;
    uint64_t timeSpentWriting;
    char busy;
};
void* findPrimesThread(struct primeGenTaskInfo* info){
    if(info->start < 2) info->start = 2;

    uint64_t startTime = GetTimeUs64();

    uint64_t c = check_block(info->start, info->count, info->storage);
    info->primesFound += c;
    info->biggestPrime = info->storage[c - 1];
    info->timeSpentLastWork = GetTimeUs64() - startTime;

    uint64_t startWriteTime = GetTimeUs64();

    char fname[64];
    sprintf(fname, "primes/primes_%I64i-%I64i.txt", info->start, info->start + info->count);
    FILE* file = fopen(fname, "w");
    for(int i=0; i<c; i++){
        fprintf(file, "%I64i\n", info->storage[i]);
    }
    fclose(file);

    info->timeSpentWriting = GetTimeUs64() - startWriteTime;

    info->busy = 0;
    return NULL;
}
int main(){
    pthread_t thread_ids[threadCount];
    struct primeGenTaskInfo infos[threadCount];

    uint64_t pos = FIRST_NUMBER;
    uint64_t remaining = HOW_MANY_NUMBERS;

    for(int i=0; i<threadCount; i++){
        infos[i].storage = malloc(maxUnsavedPrimes / 2 * sizeof(uint64_t));
        infos[i].busy = 0;

        infos[i].start = 0;
        infos[i].count = 0;
        infos[i].threadId = 0;
        infos[i].biggestPrime = 0;
        infos[i].primesFound = 0;
        infos[i].timeSpentLastWork = 0;
        infos[i].timeSpentWriting = 0;
    }

    printf("Searching Prime numbers from %I64i to %I64i (%I64i candidates)\n", pos, pos + remaining, remaining);

    uint64_t biggestPrime = 0;
    uint64_t startTimeMs = GetTimeMs64();
    uint64_t totalPrimesFound = 0;
    while(remaining > 0){
        for(uint64_t i=0; i<threadCount; i++){
            if(infos[i].busy == 0){
                uint64_t count = maxUnsavedPrimes;
                if(remaining < maxUnsavedPrimes) count = remaining;

                uint64_t workingTimeUs = infos[i].timeSpentLastWork;
                uint64_t writingTimeUs = infos[i].timeSpentWriting;
                if(infos[i].biggestPrime > biggestPrime) biggestPrime = infos[i].biggestPrime;
                totalPrimesFound += infos[i].primesFound;
                printf("[%I64i min] Starting Thread %I64i at position %I64i with a count of %I64i. Largest prime number so far: %I64i   Total primes found: %I64i\n"\
                    "    Last Work took %I64i us / %I64i ms / %I64i sec / %I64i min\n    Last Writing took %I64i us / %I64i ms\n", 
                    (GetTimeMs64() - startTimeMs) / 1000 / 60, i + 1, pos, count, biggestPrime, totalPrimesFound, workingTimeUs, workingTimeUs / 1000, 
                    workingTimeUs / 1000 / 1000, workingTimeUs / 1000 / 1000 / 60, writingTimeUs, writingTimeUs / 1000);
                infos[i].start = pos;
                infos[i].count = count;
                infos[i].threadId = i + 1;
                infos[i].biggestPrime = 0;
                infos[i].primesFound = 0;
                infos[i].timeSpentLastWork = 0;
                infos[i].timeSpentWriting = 0;
                infos[i].busy = 1;
                
                pthread_create(&thread_ids[i], NULL, findPrimesThread, &infos[i]);

                pos += count;
                remaining -= count;
            }
        }
        sleep(1);
    }
    for(int i=0; i<threadCount; i++){
        if(infos[i].busy) pthread_join(thread_ids[i], NULL);
        if(infos[i].biggestPrime > biggestPrime) biggestPrime = infos[i].biggestPrime;
        totalPrimesFound += infos[i].primesFound;
    }
    printf("Finished in %I64i min! Found %I64i prime numbers.\nBiggest Prime number is %I64i\n", (GetTimeMs64() - startTimeMs) / 1000 / 60, totalPrimesFound, biggestPrime);

    return 0;
}
