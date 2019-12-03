#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

void* sillyThreadFunc(void* arg);

int main(void)
{
    pthread_t tid;

    printf("Starting the main thread\n");

    //Nap for 2 seconds
    sleep(2);

    // create a second thread
    pthread_create(&tid, NULL, sillyThreadFunc, NULL);

    // pretend to work for a bit, but instead nap for another 4 seconds
    sleep(4);

    printf("Ending the main thread\n");
    
    return 0;
}

void* sillyThreadFunc(void* arg)
{
    printf("Starting the Silly Function thread\n");

    printf("Threads are awesome!!1!\n");
    // pretend to work for a bit - nap for 2 seconds
    sleep(2);

    printf("Ending the Silly Function thread\n");

    return NULL;
}
