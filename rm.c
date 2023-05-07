#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include "rm.h"
#include <stdbool.h>

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
int Maximum[MAXP][MAXR];
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
        Maximum[tid][i] = claim[i];
        Need[tid][i] = claim[i] - Allocation[tid][i];
    }

    pthread_mutex_unlock(&resource_mutex);

    return 0;
}


int rm_init(int N, int M, int existing[], int avoid) {
    if (N <= 0 || N > MAXP || M <= 0 || M > MAXR || (avoid != 0 && avoid != 1)) {
        return -1;
    }

    DA = avoid;
    N = N;
    M = M;
    
    for (int i = 0; i < M; i++) {
        if (existing[i] < 0) {
            return -1;
        }
        ExistingRes[i] = existing[i];
    }

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
    int temp_avail[MAXR];
    int temp_need[MAXP][MAXR];

    for (int i = 0; i < M; i++) 
    {
        temp_avail[i] = Available[i] - request[i];
        if (temp_avail[i] < 0) 
        {
            return 0;
        }
    }

    for (int i = 0; i < N; i++) 
    {
        for (int j = 0; j < M; j++) 
        {
            if (i == tid) 
            {
                temp_need[i][j] = Need[i][j] - request[j];
            } 
            else 
            {
                temp_need[i][j] = Need[i][j];
            }
        }
    }

    int finish[MAXP] = {0};
    int work[MAXR];

    for (int i = 0; i < M; i++) 
    {
        work[i] = temp_avail[i];
    }

    for (int i = 0; i < N; i++) 
    {
        for (int j = 0; j < N; j++) 
        {
            if (finish[j] == 0) 
            {
                int can_finish = 1;
                for (int k = 0; k < M; k++) 
                {
                    if (temp_need[j][k] > work[k]) 
                    {
                        can_finish = 0;
                        break;
                    }
                }
                if (can_finish) 
                {
                    finish[j] = 1;
                    for (int k = 0; k < M; k++) 
                    {
                        work[k] += Allocation[j][k];
                    }
                }
            }
        }
    }

    for (int i = 0; i < N; i++) 
    {
        if (finish[i] == 0) 
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

    int safe = 1;

    if (DA) 
    {
        safe = is_safe(tid, request);
    }

    while (!safe || !resources_available(request)) 
    {
        pthread_cond_wait(&resource_cond, &resource_mutex);

        if (DA) 
        {
            safe = is_safe(tid, request);
        } 
        else 
        {
            safe = 1;
        }
    }

    for (int i = 0; i < M; i++) 
    {
        Available[i] -= request[i];
        Allocation[tid][i] += request[i];
        Need[tid][i] -= request[i];
    }

    pthread_mutex_unlock(&resource_mutex);

    return 0;
}

int rm_release(int release[]) 
{
    pthread_mutex_lock(&resource_mutex);

    // Get the calling thread's internal ID
    pthread_t calling_thread = pthread_self();

    // Find the index of the calling thread in the threads array
    int thread_index = -1;
    for (int i = 0; i < N; i++) 
    {
        if (pthread_equal(threads[i], calling_thread)) 
        {
            thread_index = i;
            break;
        }
    }

    // If the calling thread is not found, return -1 (error)
    if (thread_index == -1) 
    {
        pthread_mutex_unlock(&resource_mutex);
        return -1;
    }

    // Check if the release request is valid
    for (int i = 0; i < M; i++) 
    {
        if (release[i] > Allocation[thread_index][i]) 
        {
            pthread_mutex_unlock(&resource_mutex);
            return -1;
        }
    }

    // Update the Allocation, Need and Available matrices
    for (int i = 0; i < M; i++) 
    {
        Allocation[thread_index][i] -= release[i];
        Need[thread_index][i] += release[i];
        Available[i] += release[i];
    }

    // Wake up the blocked threads
    pthread_cond_broadcast(&resource_cond);

    pthread_mutex_unlock(&resource_mutex);
    return 0;
}


int rm_detection() {
    pthread_mutex_lock(&resource_mutex);

    int work[M];
    int finish[N];
    int deadlocked_count = 0;

    // Initialize work array with the current Available array
    for (int i = 0; i < M; i++) 
    {
        work[i] = Available[i];
    }

    // Initialize finish array to false
    for (int i = 0; i < N; i++) 
    {
        finish[i] = 0;
    }

    int found;
    do 
    {
        found = 0;
        for (int i = 0; i < N; i++) 
        {
            if (!finish[i]) 
            {
                int can_finish = 1;
                for (int j = 0; j < M; j++) 
                {
                    if (Need[i][j] > work[j]) 
                    {
                        can_finish = 0;
                        break;
                    }
                }

                if (can_finish) 
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

    // Count the number of unfinished threads, which are considered deadlocked
    for (int i = 0; i < N; i++) 
    {
        if (!finish[i]) 
        {
            deadlocked_count++;
        }
    }

    pthread_mutex_unlock(&resource_mutex);
    return deadlocked_count;
}


void rm_print_state(char headermsg[]) 
{
    pthread_mutex_lock(&resource_mutex);

    printf("##########################\n");
    printf("%s\n", headermsg);
    printf("###########################\n");

    printf("Exist:\nR0 R1\n");
    for (int i = 0; i < M; ++i) 
    {
        printf("%d ", ExistingRes[i]);
    }
    printf("\n");

    printf("Available:\nR0 R1\n");
    for (int i = 0; i < M; ++i) 
    {
        printf("%d ", Available[i]);
    }
    printf("\n");

    printf("Allocation:\nR0 R1\n");
    for (int i = 0; i < N; ++i) 
    {
        printf("T%d: ", i);
        for (int j = 0; j < M; ++j) 
        {
            printf("%d ", Allocation[i][j]);
        }
        printf("\n");
    }

    // Assuming you have a Request matrix defined and updated in rm_request()
    printf("Request:\nR0 R1\n");
    for (int i = 0; i < N; ++i) 
    {
        printf("T%d: ", i);
        for (int j = 0; j < M; ++j) 
        {
            printf("%d ", Request[i][j]);
        }
        printf("\n");
    }

    printf("MaxDemand:\nR0 R1\n");
    for (int i = 0; i < N; ++i) 
    {
        printf("T%d: ", i);
        for (int j = 0; j < M; ++j) 
        {
            printf("%d ", Maximum[i][j]);
        }
        printf("\n");
    }

    printf("Need:\nR0 R1\n");
    for (int i = 0; i < N; ++i) 
    {
        printf("T%d: ", i);
        for (int j = 0; j < M; ++j) 
        {
            printf("%d ", Need[i][j]);
        }
        printf("\n");
    }

    printf("##########################\n");

    pthread_mutex_unlock(&resource_mutex);
}

