#include "rm.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

int num_threads;
int num_resource_types;
int existing_resources[MAXR];
bool deadlock_avoidance;

int rm_init(int p_count, int r_count, int r_exist[], int avoid) {
    if (p_count <= 0 || p_count > MAXP || r_count <= 0 || r_count > MAXR || (avoid != 0 && avoid != 1)) {
        return -1;
    }

    num_threads = p_count;
    num_resource_types = r_count;
    for (int i = 0; i < r_count; i++) {
        if (r_exist[i] < 0) {
            return -1;
        }
        existing_resources[i] = r_exist[i];
    }

    deadlock_avoidance = avoid == 1;

    return 0;
}

int rm_thread_started(int tid) {
    // Implement the function to handle a new thread starting
    return 0;
}

int rm_thread_ended() {
    // Implement the function to handle a thread ending
    return 0;
}

int rm_claim(int claim[]) {
    // Implement the function to handle resource claim for deadlock avoidance
    return 0;
}

int rm_request(int request[]) {
    // Implement the function to handle resource request
    return 0;
}

int rm_release(int release[]) {
    // Implement the function to handle resource release
    return 0;
}

int rm_detection() {
    // Implement the function to detect deadlocks
    return 0;
}

void rm_print_state(char headermsg[]) {
    // Implement the function to print the current state of the resource manager
}
