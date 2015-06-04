#include "server_epoll_pthread_pool.h"

static CLIENT_DATA *client;
static CTHREADPOOL *pool = NULL;

void poolInit (int maxThreadNum)
{
    int i=0;
     pool = (CTHREADPOOL *) malloc (sizeof (CTHREADPOOL));

     pthread_mutex_init (&(pool->queueLock), NULL);
     pthread_cond_init (&(pool->queueReady), NULL);
     pool->queueHead = NULL;

     pool->maxThreadNum = maxThreadNum;
     pool->curQueueSize = 0;

     pool->shutdown = 0;

     pool->threadid = (pthread_t *) malloc (maxThreadNum * sizeof (pthread_t));
     for (i = 0; i < maxThreadNum; i++)
     {
         pthread_create (&(pool->threadid[i]), NULL, threadRoutine,
                 NULL);
     }
}
int poolAddWorker (void *(*process) (void *arg), void *arg)
{
    CTHREADWORKER *member;
    CTHREADWORKER *newworker =
         (CTHREADWORKER *) malloc (sizeof (CTHREADWORKER));
     newworker->process = process;
     newworker->arg = arg;
     newworker->next = NULL;

     pthread_mutex_lock (&(pool->queueLock));
     member = pool->queueHead;
         //insert item into queue
     if (member != NULL)
     {
        while (member->next != NULL)
             member = member->next;
         member->next = newworker;
     }
     else
     {
         pool->queueHead = newworker;
     }
     assert (pool->queueHead != NULL);
     pool->curQueueSize++;
     pthread_mutex_unlock (&(pool->queueLock));
     pthread_cond_signal (&(pool->queueReady));
     return 0;
}
int poolDestroy ()
{
    CTHREADWORKER *head = NULL;
    int i;
    if (pool->shutdown)
        return -1;
     pool->shutdown = 1;

     pthread_cond_broadcast (&(pool->queueReady));

    for (i = 0; i < pool->maxThreadNum; i++)
         pthread_join (pool->threadid[i], NULL);
     free (pool->threadid);

    while (pool->queueHead != NULL)
     {
         head = pool->queueHead;
         pool->queueHead = pool->queueHead->next;
         free (head);
     }
     pthread_mutex_destroy(&(pool->queueLock));
     pthread_cond_destroy(&(pool->queueReady));

     free (pool);
     pool=NULL;
    return 0;
}
void * threadRoutine (void *arg)
{
    CTHREADWORKER *worker;
     while (1)
     {

         pthread_mutex_lock (&(pool->queueLock));
     while (pool->curQueueSize == 0 && !pool->shutdown)
      {
         pthread_cond_wait (&(pool->queueReady), &(pool->queueLock));
          }
        if (pool->curQueueSize == 0 && pool->shutdown)
         {
             pthread_mutex_unlock (&(pool->queueLock));
             pthread_exit (NULL);
         }

         assert (pool->curQueueSize != 0);
         assert (pool->queueHead != NULL);

         pool->curQueueSize--;
         worker = pool->queueHead;
         pool->queueHead = worker->next;
         pthread_mutex_unlock (&(pool->queueLock));
         (*(worker->process)) (worker->arg);
         free (worker);
         worker = NULL;
     }
    return NULL;
}
void *
myprocess (void *arg)
{
        char buf[BUFFER_SIZE];
        memset(buf, '\0', sizeof(buf));
        int sockfd = *(int *)arg;
        for(int j=1;j
        {
                if(sockfd == client[j].sockfd)
                {
                        strcpy(buf, client[j].chatRecord);
                        break;
                }
        }
        printf("Get client with sockfd %d, the content is %s\n", sockfd, buf);
        for(int j=1;j
        {
                if(sockfd == client[j].sockfd)
                        continue;
                if(send(client[j].sockfd, buf, strlen(buf), 0) < 0)
                        printf("Send to sockfd %d failed\n", client[j].sockfd);
        }
}
int setnonblocking(int fd)
{
        int old_option = fcntl(fd, F_GETFL);
        int new_option = old_option|O_NONBLOCK;
        fcntl(fd, F_SETFL, new_option);
        return old_option;
}
void addfd(int epollfd,int fd,int enable_et)
{
        struct epoll_event event;
        event.data.fd = fd;
        event.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP;
        if(enable_et)
                event.events |= EPOLLET;
        epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
        setnonblocking(fd);
}
int main(int argc, char *argv[])
{
        if(argc <=2)
        {
                printf("usage %s ip_address port_number\n", argv[0]);
                return 1;
        }
        const char *ip = argv[1];
        int port = atoi(argv[2]);
        int ret = 0;
        struct sockaddr_in address;
        bzero(&address, sizeof(address));
        address.sin_family = AF_INET;
        inet_pton(AF_INET,ip, &address.sin_addr);
        address.sin_port = htons(port);

        int listenfd = socket(PF_INET, SOCK_STREAM, 0);
        assert(listenfd >=0);
        ret = bind(listenfd, (struct sockaddr*)&address, sizeof(address));
        assert(ret!=-1);
        ret = listen(listenfd, 5);
        assert(ret != -1);

        client = (CLIENT_DATA *)malloc(sizeof(CLIENT_DATA)*MAX_EVENT_NUMBER);
        int client_count = 0;
        int user_count = 0;
        struct epoll_event events[MAX_EVENT_NUMBER];

        if(pool!=NULL)
        poolDestroy ();
        int epollfd = epoll_create(5);
        assert(epollfd != -1);
        addfd(epollfd, listenfd, 1);
        poolInit (THREAD_NUM);
        while(1)
        {
                int ret = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, 1);
                if(ret <0)
                {
                        printf("Epoll failure\n");
                        break;
                }
                char buf[BUFFER_SIZE];
                for(int i=0;i
                {
                        int sockfd = events[i].data.fd;
                        if(sockfd == listenfd)
                        {
                                struct sockaddr_in client_address;
                                socklen_t client_addrlength = sizeof(client_address);
                                int connfd = accept(listenfd, (struct sockaddr*)&client_address, &client_addrlength);
                                addfd(epollfd, connfd, 1);
                                client_count++;
                                user_count++;
                                client[client_count].sockfd = connfd;
                                client[client_count].mark = 1; //add a new client
                                memset(client[client_count].chatRecord, '\0', BUFFER_SIZE);
                                printf("New client with fd %d is coming, the total client are %d\n", connfd, user_count);

                        }
                       else if((events[i].events & EPOLLIN) == EPOLLIN)
                        {
                                memset(buf, '\0', BUFFER_SIZE);
                                int ret = recv(sockfd, buf, BUFFER_SIZE-1, 0);
                                if(ret <0)
                                {
                                        if((errno == EAGAIN)||(errno == EWOULDBLOCK))
                                        {
                                                //printf("read later\n");
                                                break;
                                        }

                                        client[client_count].mark = -1;
                                        printf("There is some issue for client %d\n", sockfd);
                                        memset(client[client_count].chatRecord, '\0', sizeof(client[client_count].chatRecord));

                                        user_count--;
                                        close(sockfd);
                                        break;
                                }
                                else if(ret == 0)
                                {
                                        printf("Client %d left\n", sockfd);
                                        client[client_count].mark = -1;
                                        memset(client[client_count].chatRecord, '\0', sizeof(client[client_count].chatRecord));
                                        user_count--;
                                        close(sockfd);

                                }
                                else
                                {
                                        for(int i=1;i<=client_count;i++)
                                                if(client[i].sockfd == sockfd)
                                                {
                                                        strcpy(client[i].chatRecord, buf);
                                                        printf("Client with sockfd %d, the content is %s\n", i, buf);
                                                }
                                        poolAddWorker (myprocess, &sockfd);
                                }
                        }
                }
        }
        poolDestroy ();
        close(listenfd);
        free(client);
        free(pool);
        return 0;
}
