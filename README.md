# Resource Manager Library

Hey there! Welcome to a simple yet powerful **Resource Manager Library** project. This project is designed for you to create a thread-safe, deadlock-detecting, and resource managing library for a multithreaded program. Your application will work in harmony with the library to allocate and manage a set of simulated resources.

## What it does

Once a program is started, you can define the number of resource types and the instances of each type that will be managed by the library. This library will efficiently handle resource allocation and management for threads running concurrently in your program. 

Each thread can make requests, use resources, and release them as many times as it needs. However, when resources are not available or if it's not safe to allocate, a thread will be blocked. 

The library is smart! It can not only detect deadlocks but also avoids them when necessary.

## Core Functions

This library will provide the following main functions:

- `int rm_init (int N, int M, int existing[M], int avoid)`: Initializes the resource manager. Takes the number of threads, resource types, the existing array of resources, and a flag to indicate whether deadlock avoidance is required.

- `int rm_claim (int claim[M])`: A thread claims the resources it might need in the future.

- `int rm_thread_ended()`: Indicates that a thread has ended and its resources can be released.

- `int rm_request (int request[M])`: A thread requests for certain resources.

- `int rm_release (int release[M])`: A thread releases resources it has been holding.

- `int rm_detection()`: Checks for potential deadlocks in the system.

- `void rm_print_state (char headermsg[])`: Prints the current state of resources.

### Get ready to deep dive into the world of multi-threading and resource management! Let's bring our application threads and resources to harmony! Enjoy coding. 💻🚀

---

Note: This README assumes that you are familiar with the basics of multi-threading, resource allocation, and deadlock management. If not, check out some resources to get started. Happy learning!
