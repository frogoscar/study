#include <stdio.h>
#include <string.h>
#ifndef WIN32
#include <arpa/inet.h>
#endif // !WIN32
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>

#pragma comment(lib, "ws2_32.lib")


#define SVRADDR "213.244.28.46"
#define PORT 65420

static void buff_input_cb(struct bufferevent *bev, void *ctx)
{
        char buf[4096];
        int len;
        struct evbuffer *input = bufferevent_get_input(bev);

        len = evbuffer_remove(input, buf, sizeof(buf));
        buf[len] = '\0';

        printf("Message: %s\n", buf);
}

static void buff_ev_cb(struct bufferevent *bev, short events, void *ctx)
{

        if (events & BEV_EVENT_CONNECTED) {
                printf("Server is connected!\n");
        } else if (events & BEV_EVENT_ERROR) {
                printf("Connect server error!\n");
        }
}

int main()
{
        int sockfd;
        struct event_base *base;
        struct bufferevent *event;

#ifdef WIN32
        WSADATA wsa_data;
        WSAStartup(0x0201, &wsa_data);
#endif

        struct sockaddr_in addr;

        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(PORT);

#ifdef WIN32
        addr.sin_addr.s_addr = inet_addr(SVRADDR);
#else
        if (inet_pton(AF_INET, SVRADDR, &addr.sin_addr) <= 0) {
                printf("inet_pton\n");
                return 1;
        }
#endif

        if ((base = event_base_new()) == NULL)
        {
                return 1;
        }

        if ((event = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE)) == NULL)
        {
                return 1;
        }

        if ((sockfd = bufferevent_socket_connect(event, (struct sockaddr *)&addr, sizeof(addr))) < 0)
        {
                return 1;
        }

        bufferevent_setcb(event, buff_input_cb, NULL, buff_ev_cb, base);
        bufferevent_enable(event, EV_READ);

        event_base_dispatch(base);

        return 0;
}
