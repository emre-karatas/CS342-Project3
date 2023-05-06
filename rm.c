#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include "rm.h"

int DA; // indicates if deadlocks will be avoided or not
int N; // number of processes
int M; // number of resource types
int ExistingRes[MAXR]; // Existing resources vector

pthread_mutex_t resource_mutex = PTHREAD_MUTEX_INITIALIZER;

// Additional matrices for deadlock avoidance
int Allocation[MAXP][MAXR];
int Maximum[MAXP][MAXR];
int Need[MAXP][MAXR];

int rm_thread_started(int tid) {
    int ret = 0;
    if (tid >= 0 && tid < N) {
        ret = 1;
    }
    return ret;
}

int rm_thread_ended() {
    int ret = 0;
    // Implement logic to clean up when a thread ends, if necessary
    return ret;
}

int rm_claim(int claim[]) {
    int ret = 0;
    pthread_mutex_lock(&resource_mutex);
    for (int i = 0; i < M; i++) {
        if (claim[i] < 0 || claim[i] > ExistingRes[i]) {
            ret = -1;
            break;
        }
        Maximum[0][i] = claim[i];
        Need[0][i] = claim[i] - Allocation[0][i];
    }
    pthread_mutex_unlock(&resource_mutex);
    return ret;
}

int rm_init(int p_count, int r_count, int r_exist[], int avoid) {
    int i;
    int ret = 0;
    DA = avoid;
    N = p_count;
    M = r_count;
    // initialize (create) resources
    for (i = 0; i < M; ++i)
        ExistingRes[i] = r_exist[i];
    // resources initialized (created)
    return ret;
}

int rm_request(int request[]) {
    int ret = 0;
    pthread_mutex_lock(&resource_mutex);
    for (int i = 0; i < M; i++) {
        if (request[i] < 0 || request[i] > ExistingRes[i]) {
            ret = -1;
            break;
        }
        Allocation[0][i] += request[i];
        ExistingRes[i] -= request[i];
    }
    pthread_mutex_unlock(&resource_mutex);
    return ret;
}

int rm_release(int release[]) {
    int ret = 0;
    pthread_mutex_lock(&resource_mutex);
    for (int i = 0; i < M; i++) {
        if (release[i] < 0 || release[i] > Allocation[0][i]) {
            ret = -1;
            break;
        }
        Allocation[0][i] -= release[i];
        ExistingRes[i] += release[i];
    }
    pthread_mutex_unlock(&resource_mutex);
    return ret;
}

int rm_detection() {
    int ret = 0;
    // Implement deadlock detection algorithm, if required
    return ret;
}

void rm_print_state(char hmsg[]) {
    printf("%s\n", hmsg);
    // Implement print state function, if necessary
}
