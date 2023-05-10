#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdarg.h>
#include "rm.h"

#define NUMR 5      // number of resource types
#define NUMP 3        // number of threads


void pr(int tid, char astr[], int m, int r[]) 
{
    int i;
    printf("thread %d, %s, [", tid, astr);
    for (i = 0; i < m; ++i) 
    {
        if (i == (m - 1))
            printf("%d", r[i]);
        else
            printf("%d,", r[i]);
    }
    printf("]\n");
}

void setarray(int r[MAXR], int m, ...) 
{
    va_list valist;
    int i;
    va_start(valist, m);
    for (i = 0; i < m; i++) 
    {
        r[i] = va_arg(valist, int);
    }
    va_end(valist);
    return;
}

void *threadfunc(void *a);

int main(int argc, char *argv[]) 
{
    if (argc != 2) {
        printf("Usage: %s <flag>\n", argv[0]);
        exit(1);
    }

    int flag = atoi(argv[1]);
    int i;
    int count;
    int ret;

    pthread_t threads[NUMP];
    int thread_ids[NUMP];

    // Initialize the resource management library
    int existing[NUMR] = {10, 10, 10, 10, 10};
    int result = rm_init(NUMP, NUMR, existing, flag);

    if (result == -1) {
        printf("Error initializing the resource management library.\n");
        exit(1);
    }

    // Create threads
    for (int i = 0; i < NUMP; i++) {
        thread_ids[i] = i;
        pthread_create(&threads[i], NULL, threadfunc, (void *)&thread_ids[i]);
    }
    count = 0;
    while ( count < 10) 
    {
        sleep(1);
        rm_print_state("The current state");
        printf("\n");
        ret = rm_detection();
        if (ret > 0) 
        {
            printf("deadlock detected, count=%d\n", ret);
            rm_print_state("state after deadlock");
        }
        count++;
    }
    
    if (ret == 0) 
    {
        for (i = 0; i < NUMP; ++i) 
        {
            pthread_join (threads[i], NULL);
            printf("joined\n");
        }
    }

    return 0;
}

void *threadfunc(void *a) 
{
    int tid;
    int request1[MAXR];
    int request2[MAXR];
    int claim[MAXR];
    tid = *((int *)a);
    rm_thread_started(tid);

    // Set claim and request arrays depending on the thread ID
    switch (tid) {
        case 0:
            setarray(claim, NUMR, 8, 2, 5, 4, 3);
            setarray(request1, NUMR, 5, 1, 3, 2, 1);
            setarray(request2, NUMR, 3, 1, 2, 2, 2);
            break;
        case 1:
            setarray(claim, NUMR, 6, 4, 7, 2, 5);
            setarray(request1, NUMR, 4, 2, 4, 1, 3);
            setarray(request2, NUMR, 2, 2, 3, 1, 2);
            break;
        case 2:
            setarray(claim, NUMR, 5, 3, 6, 3, 4);
            setarray(request1, NUMR, 3, 1, 3, 1, 2);
            setarray(request2, NUMR, 2, 1, 2, 1, 1);
            break;
        default:
            break;
    }

    rm_claim(claim);
    pr(tid, "REQ", NUMR, request1);
    rm_request(request1);
    sleep(4);
    pr(tid, "REQ", NUMR, request2);
    rm_request(request2);
    rm_release(request1);
    rm_release(request2);
    rm_thread_ended();
    pthread_exit(NULL);
}


