#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    DEBUG_LOG("Thread function started with wait_to_obtain_ms=%d, wait_to_release_ms=%d",
              thread_func_args->wait_to_obtain_ms, thread_func_args->wait_to_release_ms);
    
    // Sleep wait_to_obtain_ms before trying to lock the mutex
    usleep(thread_func_args->wait_to_obtain_ms * 1000);

    // Try to lock the mutex
    if (pthread_mutex_lock(thread_func_args->mutex) != 0) {
        ERROR_LOG("Failed to lock mutex");
        thread_func_args->thread_complete_success = false;
        pthread_exit(NULL);
    }

    DEBUG_LOG("Mutex locked, sleeping for %d ms", thread_func_args->wait_to_release_ms);
    // Sleep wait_to_release_ms while holding the mutex
    usleep(thread_func_args->wait_to_release_ms * 1000);
    DEBUG_LOG("Releasing mutex");

    // Unlock the mutex
    if (pthread_mutex_unlock(thread_func_args->mutex) != 0) {
        ERROR_LOG("Failed to unlock mutex");
        thread_func_args->thread_complete_success = false;
        pthread_exit(NULL);
    }

    thread_func_args->thread_complete_success = true;

    return thread_param; // Return the thread data structure for cleanup, and to provide the thread work result
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */

    struct thread_data *thread_func_args = NULL;

    DEBUG_LOG("start_thread_obtaining_mutex: wait_to_obtain_ms=%d, wait_to_release_ms=%d", wait_to_obtain_ms, wait_to_release_ms);

    if (thread == NULL || mutex == NULL) {
        ERROR_LOG("Invalid thread or mutex pointer");
        return false;
    }

    thread_func_args = malloc(sizeof(struct thread_data));
    if (thread_func_args == NULL) {
        ERROR_LOG("Failed to allocate memory for thread data");
        return false;
    }

    thread_func_args->mutex = mutex;
    thread_func_args->wait_to_obtain_ms = wait_to_obtain_ms;
    thread_func_args->thread_complete_success = false;
    thread_func_args->wait_to_release_ms = wait_to_release_ms;

    if (pthread_create(thread, NULL, threadfunc, (void*)thread_func_args) != 0) {
        ERROR_LOG("Failed to create thread");
        free(thread_func_args);
        return false;
    }
    DEBUG_LOG("Thread created successfully with ID: %lu", *thread);
    return true;
}

