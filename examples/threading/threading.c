#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

#define MILISECONDS_TO_MICROSECONDS 1000

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    if(thread_param == NULL)
    {
        return NULL;
    }
    
    struct thread_data* data = (struct thread_data *)thread_param;
    usleep(data->wait_ms * MILISECONDS_TO_MICROSECONDS);
    pthread_mutex_lock(data->mutex);
    usleep(data->wait_to_release_ms * MILISECONDS_TO_MICROSECONDS);
    data->thread_complete_success = true;
    pthread_mutex_unlock(data->mutex);

    return thread_param;
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
    struct thread_data* data = (struct thread_data *)malloc(sizeof(struct thread_data));
    if (data == NULL)
    {
        return false;
    }

    // populating the data struct
    data->wait_ms = wait_to_obtain_ms;
    data->wait_to_release_ms = wait_to_release_ms;
    data->thread_complete_success = false;
    data->mutex = mutex;

    int rc = pthread_create(thread, NULL, threadfunc, data);
    if (rc != 0)
    {
        return false;
    }
    
    return true;
}

