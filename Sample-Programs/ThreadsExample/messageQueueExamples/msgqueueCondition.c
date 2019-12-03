#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

//****************************** Message queue stuff ******************************
typedef struct {
    int sender;
    int value;
} Message;

int fudge = 0;

//Message node
typedef struct message_node {
    Message msg;
    struct message_node* next;
} MessageNode;

//Message queue - a singly linked list
//Remove from head, add to tail
typedef struct {
    MessageNode* head;
    MessageNode* tail;
    pthread_mutex_t mutex;
    
    //Add a condition variable
    pthread_cond_t cond;
} MessageQueue;

//Create a queue and initilize its mutex and condition variable
MessageQueue* createMessageQueue()
{
    MessageQueue* q = (MessageQueue*)malloc(sizeof(MessageQueue));
    q->head = q->tail = NULL;
    pthread_mutex_init(&q->mutex, NULL);
    
    //Initialize the condition variable
    pthread_cond_init(&q->cond, NULL);
    return q;
}

//"Send" a message - append it onto the queue
void sendMessage(MessageQueue* q, int sender, int value)
{
    MessageNode* node = (MessageNode*)malloc(sizeof(MessageNode));
    node->msg.sender = sender;
    node->msg.value = value;
    node->next = NULL;

    // critical section
    pthread_mutex_lock(&q->mutex);
    if (q->tail != NULL) {
        q->tail->next = node;       // append after tail
        q->tail = node;
    } else {
        q->tail = q->head = node;   // first node
    }
    //Signal the consumer thread woiting on this condition variable
    pthread_cond_signal(&q->cond);
    pthread_mutex_unlock(&q->mutex);
    fprintf(stderr, "Worker %d enqueues the message, signals cond variable, unlocks mutex, and goes to sleep\n", sender);
    sleep(2);

}

//"Receive" a message - remove it from the queue
int getMessage(MessageQueue* q, Message* msg_out)
{
    int success = 0;
    
    // critical section
    pthread_mutex_lock(&q->mutex);
    
    //Wait for a signal telling us that there's something on the queue
    //If we get woken up but the queue is still null, we go back to sleep
    while(q->head == NULL){
        fprintf(stderr,"Message queue is empty and getMessage goes to sleep (pthread_cond_wait)\n");
        pthread_cond_wait(&q->cond, &q->mutex);
        fprintf(stderr,"getMessage is woken up - someone signalled the condition variable\n");
    }
    
    //By the time we get here, we know q->head is not null, so it's all good
    MessageNode* oldHead = q->head;
    *msg_out = oldHead->msg;    // copy out the message
    q->head = oldHead->next;
    if (q->head == NULL) {
        q->tail = NULL;         // last node removed
    }
    free(oldHead);
    success = 1;
    
    //Release lock
    pthread_mutex_unlock(&q->mutex);

    return success;
}
//*****************************************************************************

//*************************** Thread function stuff ***************************
void* workerFunc(void* arg);

#define NUM_WORKERS 2

//Each thread needs multiple arguments, so we create a dedicated struct
typedef struct {
    int workerId;
    MessageQueue* q;
} ThreadArgs;
//*****************************************************************************

int main(void)
{
    pthread_t tid[NUM_WORKERS];
    ThreadArgs args[NUM_WORKERS];
    int i;
    MessageQueue* q = createMessageQueue();

    // create worker threads
    for (i = 0; i < NUM_WORKERS; i++) {
        args[i].workerId = i + 1;
        args[i].q = q;
        pthread_create(&tid[i], NULL, workerFunc, &args[i]);
    }
    
    for (;;) {
        Message msg;
        if (getMessage(q, &msg)) {
            fprintf(stderr,"getMessage removes message %d from worker %d\n", msg.value, msg.sender);
        }
    }
    
    // wait for worker threads to terminate
    for (i = 0; i < NUM_WORKERS; i++) {
        pthread_join(tid[i], NULL);
    }
    
    return 0;
}

void* workerFunc(void* arg)
{
    ThreadArgs* args = (ThreadArgs*)arg;
    int i;

    for (i = 0; i < 10; i++) {
        sendMessage(args->q, args->workerId, i);
    }
    
    return NULL;
}
