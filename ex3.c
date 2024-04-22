#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#define MAXLENGTH  100

typedef struct {
    char** data;
    int front;
    int rear;
    int count;
    int maxSize;
    sem_t empty;
    sem_t full;
    pthread_mutex_t mutex;
} BoundedQueue;

typedef struct {
    int id;
    int numOfProducts;
    int queueSize;
    BoundedQueue* boundedQueue;
} Producer;

Producer** producers;
int numOfProducers = 0;
int coEditorsSize = 0;

BoundedQueue* createBounded(int maxSize){
    //creates the bounded queue
    BoundedQueue* bq= (BoundedQueue*) malloc(sizeof(BoundedQueue));
    if(bq == NULL){
        return NULL;
    }
    //init the values
    bq->maxSize = maxSize;
    sem_init(&bq->empty,0, maxSize);
    sem_init(&bq->full, 0, 0);
    pthread_mutex_init(&(bq->mutex),NULL);
    bq->count = 0;
    bq->front = 0;
    bq->rear = -1;
    //creates kind of an array in the size of maxsize
    bq->data = (char**) malloc(bq->maxSize * sizeof(char*));
    if(bq->data == NULL){
        printf("createdBounded error");
        sem_destroy(&(bq->empty));
        sem_destroy(&(bq->empty));
        pthread_mutex_destroy(&(bq->mutex));
        free(bq);
        exit(1);
    }
    return bq;
}

void boundedEnqueue(BoundedQueue* bq, char* s){
    sem_wait(&bq->empty);
//    sem_wait(&bq->mutex);
    pthread_mutex_lock(&(bq->mutex));
    //move the pointer in a circular way
    bq->rear = (bq->rear + 1) % bq->maxSize;
    //insert the data
    bq->data[bq->rear] = s;
    bq->count ++;
    pthread_mutex_unlock(&(bq->mutex));
    sem_post(&bq->full);
}

char* boundedDequeue (BoundedQueue* bq){
    char* s;
    sem_wait(&bq->full);
    pthread_mutex_lock(&(bq->mutex));
    s = bq->data[bq->front];
    bq->front = (bq->front + 1) % bq->maxSize;
    bq->count --;
    pthread_mutex_unlock(&(bq->mutex));
    sem_post(&bq->empty);
    return s;
}

typedef struct {
    char** data;
    sem_t full;
    pthread_mutex_t mutex;
    int start;
    int end;
    int count;
}UnBoundedQueue;

UnBoundedQueue** coEditorsArray;

UnBoundedQueue* createUnbounded(){
    UnBoundedQueue* ubq = malloc(sizeof(UnBoundedQueue));
    if(ubq == NULL){
        free(ubq);
        return NULL;
    }
    ubq->data = NULL;
    ubq->count = 0;
    sem_init(&ubq->full, 0, 0);
//    sem_init(&ubq->mutex, 0, 1);
    pthread_mutex_init(&ubq->mutex,NULL);
    ubq->start = 0;
    ubq->end = 0;
    return ubq;
}

void unboundedEnqueue(UnBoundedQueue* ubq, char* s){
    pthread_mutex_lock(&ubq->mutex);
    ubq->data = (char**) realloc(ubq->data, ((ubq->end+ 1) * sizeof(char*)));
    if(ubq->data == NULL){
        printf("unbounded realloc error");
        exit(1);
    }
    ubq->data[ubq->count] = s;
    (ubq->count)++;
    (ubq->end)++;
    pthread_mutex_unlock(&(ubq->mutex));
    sem_post(&(ubq->full));
}

char* unboundedDequeue (UnBoundedQueue* ubq){
    sem_wait(&ubq->full);
    pthread_mutex_lock(&ubq->mutex);

    char* s = ubq->data[ubq->start];
    if(s==NULL){
        return "";
    }
    ubq->start++;
    ubq->count --;

    pthread_mutex_unlock(&ubq->mutex);

    return s;

}

void createProducer(int pID, int pNumOf, int pQueueSize){
    Producer *newProducer = malloc(sizeof (Producer));
    if(newProducer == NULL){
        printf("create producer error");
        exit(1);
    }
    newProducer->id = pID;
    newProducer->numOfProducts = pNumOf;
    newProducer->queueSize = pQueueSize;
    newProducer->boundedQueue = createBounded(newProducer->queueSize);
    if(newProducer->boundedQueue == NULL){
        exit(1);
    }

    producers = (Producer**) realloc(producers, (numOfProducers +1) * sizeof (Producer*));
    if(producers == NULL){
        //free newProducer
        free(newProducer->boundedQueue->data);
        pthread_mutex_destroy(&(newProducer->boundedQueue->mutex));
        sem_destroy(&(newProducer->boundedQueue->full));
        sem_destroy(&(newProducer->boundedQueue->empty));
        free(newProducer);
        exit(1);
    }
    producers[numOfProducers] = newProducer;
    numOfProducers++;
}

void openFile(const char* filePath) {
    //opens the file path in read mode
    FILE *file = fopen(filePath, "r");
    if (file == NULL) {
        printf("open file error");
        exit(1);
    }
    int pID, pNumOf, pQueueSize;
    while (fscanf(file, "%d\n%d\n%d\n\n", &pID, &pNumOf, &pQueueSize) == 3) {
        createProducer(pID - 1, pNumOf, pQueueSize);
    }
    coEditorsSize = pID;
    fclose(file);
}

void createArticles(Producer *producer) {
    int weather = 0;
    int sports = 0;
    int news = 0;

    for (int j = 0; j < producer->numOfProducts; j++) {
        char *s = (char *) malloc(sizeof(char) * MAXLENGTH);
        if(s == NULL){
            exit(1);
        }

        int randomNumber = (rand() % 3);

        switch (randomNumber) {
            case 0: {
                sprintf(s, "Producer %d SPORTS %d", producer->id, sports);
                sports++;
                break;
            }
            case 1:
                sprintf(s, "Producer %d WEATHER %d", producer->id, weather);
                weather++;
                break;
            case 2:
                sprintf(s, "Producer %d NEWS %d", producer->id, news);
                news++;
                break;
            default:
                break;
        }

        boundedEnqueue(producer->boundedQueue, s);
    }
    boundedEnqueue(producer->boundedQueue, "DONE");
}

void dispatcher() {
    for (int current = 0, ended = 0; ended != numOfProducers; current = (current + 1) % numOfProducers) {
        if (producers[current]->boundedQueue->maxSize == 0) {
            continue;
        }

        char *article;
        article = boundedDequeue(producers[current]->boundedQueue);

        if (article == NULL || (strlen(article) == 0)) {
            continue;
        }
        if (strcmp(article, "DONE") == 0) {
            producers[current]->boundedQueue->maxSize = 0;
            ended++;
            continue;
        }
        if (strstr(article, "SPORTS") != NULL) {
            unboundedEnqueue(coEditorsArray[0], article);
        } else if (strstr(article, "WEATHER") != NULL) {
            unboundedEnqueue(coEditorsArray[1], article);
        } else if (strstr(article, "NEWS") != NULL) {
            unboundedEnqueue(coEditorsArray[2], article);
        }
    }
    for (int i = 0; i < 3; i++) {
        unboundedEnqueue(coEditorsArray[i], "DONE");
    }
}
BoundedQueue*screenManger;

void *createScreenManager(int maxSize) {
    screenManger = createBounded(maxSize);
    return NULL;
}

void coEditors(void *args) {
    UnBoundedQueue *coEditor = ((UnBoundedQueue *) args);
    while (1) {

        char *article = unboundedDequeue(coEditor);

        if (strcmp(article, "DONE") == 0) {
            boundedEnqueue(screenManger, article);
            break;
        }
        usleep(100000);
        boundedEnqueue(screenManger, article);
    }

}

void printToScreen() {
    int count = 0;
    int ended = 0;
    while (ended != 3) {
        char *article = boundedDequeue(screenManger);
        if (article == NULL) {
            continue;
        }
        if (strcmp(article, "DONE") == 0) {
            ended++;
            continue;
        }
        printf("%s\n", article);
        count++;
        free(article);
    }
    printf("Num of articles: %d", count);
}


int main(int argc, const char* argv[]) {
    if (argc != 2) {
        printf("Error in: arguments");
        return 1;
    }

    const char* filePath = argv[1];

    openFile(filePath);

    coEditorsArray = malloc(3* sizeof(UnBoundedQueue*));
    if(coEditorsArray == NULL){
        exit(1);
    }

    for(int i = 0 ; i < 3 ; i++){
        UnBoundedQueue* queue = createUnbounded();
        coEditorsArray[i] = queue;
    }

    createScreenManager(coEditorsSize);

    //creates the threads
    pthread_t producerThreads[numOfProducers];
    pthread_t dispatcherThread;
    pthread_t coEditorsThreads[3];
    pthread_t screenManagerThread;


    for ( int i = 0 ; i < numOfProducers ; i ++){

        pthread_create(&producerThreads[i], NULL, (void *) createArticles, (void*) producers[i]);
    }

    pthread_create(&dispatcherThread, NULL, (void*) dispatcher, NULL);
    pthread_join(dispatcherThread, NULL);

    for(int i = 0 ; i < 3 ; i++){
        pthread_create(&coEditorsThreads[i], NULL, (void*) coEditors , (void*) coEditorsArray[i]);
    }

    pthread_create(&screenManagerThread, NULL, (void*) printToScreen, NULL);
    pthread_join(screenManagerThread, NULL);

    for(int i = 0 ; i < 3 ; i++){
        pthread_join(coEditorsThreads[i], NULL);
    }
    for (int i = 0 ; i < numOfProducers ; i++){
        pthread_join(producerThreads[i], NULL);
    }

    for(int i = 0 ; i < numOfProducers ; i++){
        pthread_mutex_destroy(&(producers[i]->boundedQueue->mutex));
        sem_destroy(&(producers[i]->boundedQueue->full));
        sem_destroy(&(producers[i]->boundedQueue->empty));
        free(producers[i]->boundedQueue->data);
        free(producers[i]->boundedQueue);
        free(producers[i]);
    }
    free(producers);

    free(coEditorsArray[0]->data);
    pthread_mutex_destroy(&(coEditorsArray[0]->mutex));
    sem_destroy(&(coEditorsArray[0]->full));
    free(coEditorsArray[0]);

    free(coEditorsArray[1]->data);
    pthread_mutex_destroy(&(coEditorsArray[1]->mutex));
    sem_destroy(&(coEditorsArray[1]->full));
    free(coEditorsArray[1]);

    free(coEditorsArray[2]->data);
    pthread_mutex_destroy(&(coEditorsArray[2]->mutex));
    sem_destroy(&(coEditorsArray[2]->full));
    free(coEditorsArray[2]);

    free(coEditorsArray);

    free(screenManger->data);
    free(screenManger);

    return 0;
}