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
pthread_t threads[MAXP];//   to  keep threads.
bool active_threads[MAXP]; // to keep active threads

pthread_mutex_t resource_mutex = PTHREAD_MUTEX_INITIALIZER; // global mutex declaration&initialization

pthread_cond_t resource_cond = PTHREAD_COND_INITIALIZER; // global cv declaration&initialization

// Additional matrices for deadlock avoidance
int Allocation[MAXP][MAXR];
int MaxDemand[MAXP][MAXR];
int Need[MAXP][MAXR];
int Request[MAXP][MAXR];

// thread starting function
int rm_thread_started(int tid) 
{
    int ret = 0;
    if (tid < 0 || tid >= N) 
    {
        ret=-1;
    }

    pthread_mutex_lock(&resource_mutex);

    threads[tid] = pthread_self();
    active_threads[tid] = true;

    pthread_mutex_unlock(&resource_mutex);

    return (ret);
}

// rm initializing func.
int rm_init(int p_count, int r_count, int r_exist[], int avoid) 
{
    int ret = 0;
    if (p_count <= 0 || p_count > MAXP || r_count <= 0 || r_count > MAXR || (avoid != 0 && avoid != 1)) 
    {
        ret=-1;
    }

    DA = avoid;
    N = p_count;
    M = r_count;
    
    for (int i = 0; i < M; i++) 
    {
        if (r_exist[i] < 0) 
        {
            ret=-1;
        }
        ExistingRes[i] = r_exist[i];
        Available[i] = r_exist[i];
    } 
    return (ret);
}


// rm thread ending function
int rm_thread_ended() 
{
    pthread_t current_thread = pthread_self();
    int tid = -1;
    int ret = 0;

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
        ret =-1;
    }

    active_threads[tid] = false;
    threads[tid] = 0;

    pthread_mutex_unlock(&resource_mutex);

    return (ret);
}

int rm_claim(int claim[]) 
{
    pthread_t current_thread = pthread_self();
    int tid = -1;
    
    if (pthread_mutex_lock(&resource_mutex) != 0) 
    {
        perror("pthread_mutex_lock");
        return -1;
    }

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
        MaxDemand[tid][i] = claim[i];
        Need[tid][i] = claim[i];
    }

    if (pthread_mutex_unlock(&resource_mutex) != 0) {
        perror("pthread_mutex_unlock");
        return -1;
    }

    return 0;
}




int resources_available(int request[]) 
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
    int work[M];
    int finish[N];
    int i, j;

    // Initialize work to available
    for (i = 0; i < M; i++) 
    {
        work[i] = Available[i];
    }
    for (i = 0; i < N; i++) 
    {
        finish[i] = 0;
    }

    int found;
    do
    {
    	found = 0;
    	for (i = 0; i < N; i++) 
    	{
            if (finish[i] == 0) 
            {
                int can_allocate = 1;
                for (j = 0; j < M; j++) 
                {
                    if (Need[i][j] > work[j]) 
                    {
                        can_allocate = 0;
                        break;
                    }
                }

                if (can_allocate) 
                {
                    found = 1;
                    finish[i] = 1;
                    for (j = 0; j < M; j++) 
                    {
                        work[j] += Allocation[i][j];
                    }
                }
            }
        }
    
    } while (found);
    // Check if all processes are finished
    for (i = 0; i < N; i++) 
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
        if (request[i] < 0 || request[i] > ExistingRes[i] || (DA && (request[i] > Need[tid][i]))) 
        {
            pthread_mutex_unlock(&resource_mutex);
            return -1;
        }
    }
    
    
    for (int i = 0; i < M; i++) 
    {
        Request[tid][i] = request[i];
    }
    
    int available;
    do {
        available = 1;
        for (int i = 0; i < M; i++) 
        {
            if (Available[i] < request[i]) 
            {
                available = 0;
                break;
            }
        }
        if (available == 0) 
        {
            pthread_mutex_unlock(&resource_mutex);
            pthread_cond_wait(&resource_cond, &resource_mutex); 
        }
    } while (available == 0);
    
    if (DA) 
    {
        int safe;
        int condition_signaled = 0; // new flag to track whether the condition was signaled
        do 
        {
            safe = is_safe(tid,request);
            if (safe) 
            {
                for (int i = 0; i < M; i++) 
                {
                    Available[i] -= request[i];
                    Allocation[tid][i] += request[i];
                    Need[tid][i] -= request[i];
                    Request[tid][i] = 0;
                }
                pthread_mutex_unlock(&resource_mutex);
                return 0;
            }
            else 
            {
                // unlock mutex before waiting on condition variable
                pthread_mutex_unlock(&resource_mutex);
                pthread_cond_wait(&resource_cond, &resource_mutex); 

                // set flag to true if condition variable was signaled
                if (condition_signaled) {
                    available = 0;
                }
            }
        } while (safe == 0 && available == 0);
    }
    else 
    {
        
        for (int i = 0; i < M; i++) 
        {
            Available[i] -= request[i];
            Allocation[tid][i] += request[i];
            Request[tid][i] = 0;
        }
        pthread_mutex_unlock(&resource_mutex);
        return 0;
    }
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
        Available[i] += release[i];
        if (DA) 
        {
            Need[tid][i] += release[i];
        }
    }

    // Signal all waiting threads to check if they can continue now
    pthread_cond_broadcast(&resource_cond);
    pthread_mutex_unlock(&resource_mutex);

    return 0;
}


int rm_detection() 
{
    int deadlocked_count = 0;
    int work[M];
    int finish[N];
    pthread_mutex_lock(&resource_mutex);

    // Initialize the work and finish arrays
    for (int i = 0; i < M; i++) 
    {
        work[i] = Available[i];
    }

    for (int i = 0; i < N; i++) 
    {
        finish[i] = 0;
       
    }
    
    int finished;
    for (int i = 0; i < N; i++) 
    {
        finished = 1;
        for (int j = 0; j < M; j++) 
        {
            if (Request[i][j] != 0) 
            {
                finished = 0;
                break;
            }
        }
        finish[i] = finished;
    }


    // Apply the deadlock detection algorithm
    int found;
    do 
    {
        found = 0;
        for (int i = 0; i < N; i++) 
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
      } while (found);

    // Count the deadlocked threads
    for (int i = 0; i < N; i++) 
    {
        if (finish[i] == 0) 
        {
            deadlocked_count++;
        }
    }

    pthread_mutex_unlock(&resource_mutex);

    return deadlocked_count;
}

void rm_print_state(char headermsg[]) {
    pthread_mutex_lock(&resource_mutex);

    printf("###########################\n");
    printf("%s\n", headermsg);
    printf("###########################\n");

    printf("Exist:\n");
    for (int i = 0; i < M; i++) {
        printf("R%-3d", i);
    }
    printf("\n");
    for (int i = 0; i < M; i++) {
        printf("%-4d", ExistingRes[i]);
    }
    printf("\n\n");

    printf("Available:\n");
    for (int i = 0; i < M; i++) {
        printf("R%-3d", i);
    }
    printf("\n");
    for (int i = 0; i < M; i++) {
        printf("%-4d", Available[i]);
    }
    printf("\n\n");

    printf("Allocation:\n   ");
    for (int i = 0; i < M; i++) {
        printf("R%-3d", i);
    }
    printf("\n");
    for (int i = 0; i < N; i++) {
        printf("T%d: ", i);
        for (int j = 0; j < M; j++) {
            printf("%-4d", Allocation[i][j]);
        }
        printf("\n");
    }
    printf("\n");

    printf("Request:\n   ");
    for (int i = 0; i < M; i++) {
        printf("R%-3d", i);
    }
    printf("\n");
    for (int i = 0; i < N; i++) {
        printf("T%d: ", i);
        for (int j = 0; j < M; j++) {
            printf("%-4d", Request[i][j]);
        }
        printf("\n");
    }
    printf("\n");

    printf("MaxDemand:\n   ");
    for (int i = 0; i < M; i++) {
        printf("R%-3d", i);
    }
    printf("\n");
    for (int i = 0; i < N; i++) {
        printf("T%d: ", i);
        for (int j = 0; j < M; j++) {
            printf("%-4d", MaxDemand[i][j]);
        }
        printf("\n");
    }
    printf("\n");

    printf("Need:\n   ");
    for (int i = 0; i < M; i++) {
        printf("R%-3d", i);
    }
    printf("\n");
    for (int i = 0; i < N; i++) {
        printf("T%d: ", i);
        for (int j = 0; j < M; j++) {
            printf("%-4d", Need[i][j]);
        }
        printf("\n");
    }
    printf("\n");
    printf("###########################\n");

    pthread_mutex_unlock(&resource_mutex);
}

