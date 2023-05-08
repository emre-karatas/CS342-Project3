#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include "rm.h"
#include <stdbool.h>
#include <string.h>

int DA; // indicates if deadlocks will be avoided or not
int N; // number of processes
int M; // number of resource types
int ExistingRes[MAXR]; // Existing resources vector

int Available[MAXR]; // Available resources vector
pthread_t threads[MAXP];
bool active_threads[MAXP];

pthread_mutex_t resource_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t resource_cond = PTHREAD_COND_INITIALIZER;

// Additional matrices for deadlock avoidance
int Allocation[MAXP][MAXR];
int MaxDemand[MAXP][MAXR];
int Need[MAXP][MAXR];
int Request[MAXP][MAXR];

int rm_thread_started(int tid) 
{
    if (tid < 0 || tid >= N) 
    {
        return -1;
    }

    pthread_mutex_lock(&resource_mutex);

    pthread_t current_thread = pthread_self();
    threads[tid] = current_thread;
    active_threads[tid] = true;

    pthread_mutex_unlock(&resource_mutex);

    return 0;
}


int rm_init(int p_count, int r_count, int r_exist[], int avoid) 
{
    if (p_count <= 0 || p_count > MAXP || r_count <= 0 || r_count > MAXR || (avoid != 0 && avoid != 1)) 
    {
        return -1;
    }

    DA = avoid;
    N = p_count;
    M = r_count;
    printf("INITIALIZATION\n");
    printf("N: %d\n",N);
    printf("M: %d\n",M);
    
    for (int i = 0; i < M; i++) 
    {
        if (r_exist[i] < 0) 
        {
            return -1;
        }
        ExistingRes[i] = r_exist[i];
    }

    return 0;
}



int rm_thread_ended() 
{
    pthread_t current_thread = pthread_self();
    int tid = -1;

    pthread_mutex_lock(&resource_mutex);

    for (int i = 0; i < N; i++) 
    {
        if (pthread_equal(threads[i], current_thread)) 
        {
            tid = i;
            break;
        }
    }

    if (tid == -1 || !active_threads[tid]) 
    {
        pthread_mutex_unlock(&resource_mutex);
        return -1;
    }

    active_threads[tid] = false;

    pthread_mutex_unlock(&resource_mutex);

    return 0;
}

int rm_claim(int claim[]) 
{
    pthread_t current_thread = pthread_self();
    int tid = -1;

    pthread_mutex_lock(&resource_mutex);

    for (int i = 0; i < N; i++) 
    {
        if (pthread_equal(threads[i], current_thread)) 
        {
            tid = i;
            break;
        }
    }
    printf("=======CURRENT TID: %d\n ===================", tid);
    if (tid == -1 || !active_threads[tid]) 
    {
        pthread_mutex_unlock(&resource_mutex);
        return -1;
    }

    for (int i = 0; i < M; i++) 
    {
        if (claim[i] < 0 || claim[i] > ExistingRes[i]) 
        {
            pthread_mutex_unlock(&resource_mutex);
            return -1;
        }
        MaxDemand[tid][i] = claim[i];
        Need[tid][i] = claim[i] - Allocation[tid][i];
    }

    pthread_mutex_unlock(&resource_mutex);

    return 0;
}



int resources_available(int request[M]) 
{
    for (int i = 0; i < M; i++) 
    {
        if (request[i] > Available[i]) 
        {
            return 0;
        }
    }
    return 1;
}


int is_safe(int tid, int request[]) 
{
    int Work[M], Finish[N];
    memcpy(Work, ExistingRes, sizeof(Work));

    for (int i = 0; i < N; i++) 
    {
        Finish[i] = 0;
    }

    // Check if the request can be satisfied by the available resources
    for (int i = 0; i < M; i++) 
    {
        if (request[i] > Work[i]) 
        {
            return 0;
        }
    }

    // Temporarily allocate the requested resources
    for (int i = 0; i < M; i++) 
    {
        Work[i] -= request[i];
        Need[tid][i] -= request[i];
        Allocation[tid][i] += request[i];
    }

    // Check for a safe sequence using Banker's algorithm
    while (1) 
    {
        int found = 0;
        for (int i = 0; i < N; i++) 
        {
            if (!Finish[i]) 
            {
                int can_finish = 1;
                for (int j = 0; j < M; j++) 
                {
                    if (Need[i][j] > Work[j]) 
                    {
                        can_finish = 0;
                        break;
                    }
                }

                if (can_finish) 
                {
                    for (int j = 0; j < M; j++) 
                    {
                        Work[j] += Allocation[i][j];
                    }
                    Finish[i] = 1;
                    found = 1;
                    break;
                }
            }
        }

        if (!found) 
        {
            break;
        }
    }

    // Revert the temporary allocation
    for (int i = 0; i < M; i++) 
    {
        Work[i] += request[i];
        Need[tid][i] += request[i];
        Allocation[tid][i] -= request[i];
    }

    // Check if all threads can finish
    for (int i = 0; i < N; i++) 
    {
        if (!Finish[i]) 
        {
            return 0;
        }
    }

    return 1;
}

int rm_request(int request[]) 
{
    pthread_t current_thread = pthread_self();
    int tid = -1;

    pthread_mutex_lock(&resource_mutex);

    for (int i = 0; i < N; i++) 
    {
        if (pthread_equal(threads[i], current_thread)) 
        {
            tid = i;
            break;
        }
    }

    if (tid == -1 || !active_threads[tid]) 
    {
        pthread_mutex_unlock(&resource_mutex);
        return -1;
    }

    for (int i = 0; i < M; i++) 
    {
        if (request[i] < 0 || request[i] > ExistingRes[i]) 
        {
            pthread_mutex_unlock(&resource_mutex);
            return -1;
        }
    }

    if (DA) 
    {
        if (!is_safe(tid, request)) 
        {
            pthread_cond_wait(&resource_cond, &resource_mutex);
        }
    }

    for (int i = 0; i < M; i++) 
    {
        Allocation[tid][i] += request[i];
        ExistingRes[i] -= request[i];
        Need[tid][i] -= request[i];
    }

    pthread_mutex_unlock(&resource_mutex);

    return 0;
}


int rm_release(int release[]) 
{
    pthread_t current_thread = pthread_self();
    int tid = -1;

    pthread_mutex_lock(&resource_mutex);

    // Find the thread ID of the calling thread
    for (int i = 0; i < N; i++) 
    {
        if (pthread_equal(threads[i], current_thread)) 
        {
            tid = i;
            break;
        }
    }

    // Check if the thread ID is valid and the thread is active
    if (tid == -1 || !active_threads[tid]) 
    {
        pthread_mutex_unlock(&resource_mutex);
        return -1;
    }

    // Verify if the release request is valid
    for (int i = 0; i < M; i++) 
    {
        if (release[i] < 0 || release[i] > Allocation[tid][i]) 
        {
            pthread_mutex_unlock(&resource_mutex);
            return -1;
        }
    }

    // Release the resources and update the resource data structures
    for (int i = 0; i < M; i++) 
    {
        Allocation[tid][i] -= release[i];
        ExistingRes[i] += release[i];
        Need[tid][i] += release[i];
    }

    // Signal all waiting threads to check if they can continue now
    pthread_cond_broadcast(&resource_cond);

    pthread_mutex_unlock(&resource_mutex);

    return 0;
}


int rm_detection() 
{
    int deadlocked_count = 0;
    int deadlock_detected[N];
    int work[M];
    int finish[N];

    pthread_mutex_lock(&resource_mutex);

    // Initialize the work and finish arrays
    for (int i = 0; i < M; i++) 
    {
        work[i] = ExistingRes[i];
    }

    for (int i = 0; i < N; i++) 
    {
        finish[i] = 0;
        deadlock_detected[i] = 0;
    }

    // Apply the deadlock detection algorithm
    int found;
    do 
    {
        found = 0;
        for (int i = 0; i < N; i++) 
        {
            if (!finish[i] && !deadlock_detected[i]) 
            {
                int request_satisfiable = 1;
                for (int j = 0; j < M; j++) 
                {
                    if (Need[i][j] > work[j]) 
                    {
                        request_satisfiable = 0;
                        break;
                    }
                }

                if (request_satisfiable) 
                {
                    found = 1;
                    finish[i] = 1;

                    for (int j = 0; j < M; j++) 
                    {
                        work[j] += Allocation[i][j];
                    }
                }
            }
        }
    } while (found);

    // Count the deadlocked threads
    for (int i = 0; i < N; i++) 
    {
        if (!finish[i]) 
        {
            deadlocked_count++;
            deadlock_detected[i] = 1;
        }
    }

    pthread_mutex_unlock(&resource_mutex);

    return deadlocked_count;
}

void rm_print_state(char headermsg[]) 
{
    pthread_mutex_lock(&resource_mutex);

    printf("###########################\n");
    printf("%s\n", headermsg);
    printf("###########################\n");

    printf("Exist:\n");
    printf("  ");
    for (int i = 0; i < M; i++) 
    {
        printf("R%d ", i);
    }
    printf("\n");
    printf("  ");
    for (int i = 0; i < M; i++) 
    {
        printf("%d ", ExistingRes[i]);
    }
    printf("\n");

    printf("Available:\n");
    printf("  ");
    for (int i = 0; i < M; i++) 
    {
        printf("R%d ", i);
    }
    printf("\n");
    printf("  ");
    for (int i = 0; i < M; i++) 
    {
        printf("%d ", Available[i]);
    }
    printf("\n");

    printf("Allocation:\n");
    printf("  ");
    for (int i = 0; i < M; i++) 
    {
        printf("R%d ", i);
    }
    printf("\n");
    for (int i = 0; i < N; i++) 
    {
        printf("T%d: ", i);
        for (int j = 0; j < M; j++) 
        {
            printf("%d ", Allocation[i][j]);
        }
        printf("\n");
    }

    printf("Request:\n");
    printf("  ");
    for (int i = 0; i < M; i++) 
    {
        printf("R%d ", i);
    }
    printf("\n");
    for (int i = 0; i < N; i++) 
    {
        printf("T%d: ", i);
        for (int j = 0; j < M; j++) 
        {
            printf("%d ", Request[i][j]);
        }
        printf("\n");
    }

    printf("MaxDemand:\n");
    printf("  ");
    for (int i = 0; i < M; i++) 
    {
        printf("R%d ", i);
    }
    printf("\n");
    for (int i = 0; i < N; i++) 
    {
        printf("T%d: ", i);
        for (int j = 0; j < M; j++) 
        {
            printf("%d ", MaxDemand[i][j]);
        }
        printf("\n");
    }

    printf("Need:\n");
    printf("  ");
    for (int i = 0; i < M; i++) 
    {
        printf("R%d ", i);
    }
     printf("\n");
    for (int i = 0; i < N; i++) 
    {
        printf("T%d: ", i);
        for (int j = 0; j < M; j++) 
        {
            printf("%d ", Need[i][j]);
        }
        printf("\n");
    }

    printf("###########################\n");

    pthread_mutex_unlock(&resource_mutex);
}



