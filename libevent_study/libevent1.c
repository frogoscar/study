#include <stdio.h>
#include <arpa/inet.h>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>

static void buff_input_cb(struct bufferevent *bev, void *ctx){
  char buf[4096];
  int len;
  struct evbuffer *input = bufferevent_get_input(bev);

  len = evbuffer_remove(input, buf, sizeof(buf));
  buf[len] = '\0';

  printf("Message: %s\n", buf);
}

static void buff_ev_cb(struct bufferevent *bev, short events, void *cts){
  if(events & BEV_EVENT_CONNECTED){
    printf("Server is connected!\n");
  } else if(events & BEV_EVENT_ERROR){
    printf("Connect server error!\n");
  }
}

int main(int argc, char *argv[]){

  const int NUMCONNECT = 1000;
  const char *SVRADDR = "213.244.28.46";
  const uint32_t PORT = 65420;

  struct event_base *evbase;
  struct bufferevent *bev[NUMCONNECT];
  struct sockaddr_in sin[NUMCONNECT];
  evbase = event_base_new();

  int i;

  for(i=0; i<NUMCONNECT-1; i++){
    sin[i].sin_family = AF_INET;
    sin[i].sin_addr.s_addr = inet_addr(SVRADDR);
    sin[i].sin_port = htons(PORT);

    bev[i] = bufferevent_socket_new(evbase, -1, BEV_OPT_CLOSE_ON_FREE);

    bufferevent_setcb(bev[i], buff_input_cb, NULL, buff_ev_cb, evbase);
    bufferevent_socket_connect(bev[i],(struct sockaddr *)&sin[i], sizeof(struct sockaddr_in));
  }

  event_base_dispatch(evbase);

  while(1);

  return 0;
}
