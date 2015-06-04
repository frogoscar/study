#ifndef _server_epoll_pthread_pool_h_
#define _server_epoll_pthread_pool_h_
#include < sys / types.h >
#include < sys / socket.h >
#include < netdb.h >
#include < string.h >
#include < stdio.h >
#include < errno.h >
#include < unistd.h >
#include < time.h >
#include < fcntl.h >
#include < sys / epoll.h >
#include < stdlib.h >
#include < pthread.h >
#include < assert.h >
#define BUFFER_SIZE 100
#define MAX_EVENT_NUMBER 1024
#define THREAD_NUM 5

typedef struct worker
{
    void *(*process) (void *arg);
    void *arg;
    struct worker *next;
} CTHREADWORKER;


typedef struct cthreadpool
{
     pthread_mutex_t queueLock;
     pthread_cond_t queueReady;
     CTHREADWORKER *queueHead;
     int shutdown;
     pthread_t *threadid;
     int maxThreadNum;
     int curQueueSize;
} CTHREADPOOL;

typedef struct client_data
{
        int sockfd;
        int mark;
        int chatRecord[BUFFER_SIZE];
}CLIENT_DATA;

int poolAddWorker (void *(*process) (void *arg), void *arg);
void *threadRoutine (void *arg);
void poolInit (int maxThreadNum);
int poolDestroy ();
void *myprocess (void *arg);

#endif
