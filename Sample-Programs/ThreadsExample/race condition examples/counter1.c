#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

void* WorkerFunc(void* arg);

// counter shared by all threads
int g_Counter = 0;

int main(void)
{
    pthread_t tid[2];

    // create worker threads
    pthread_create(&tid[0], NULL, WorkerFunc, NULL);
    pthread_create(&tid[1], NULL, WorkerFunc, NULL);

    // wait for worker threads to terminate
    pthread_join(tid[0], NULL);
    pthread_join(tid[1], NULL);
    
    // print final counter value
    printf("Counter is %d\n", g_Counter);

    return 0;
}

void* WorkerFunc(void* arg)
{
    int i;

    // increment the counter a bunch of times
    for (i = 0; i < 1000000; i++) {
        g_Counter++;
    }
    
    return NULL;
}
