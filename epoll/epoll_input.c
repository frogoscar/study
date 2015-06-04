#include <sys/epoll.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#define MAX_BUF      1000   /* Maximum bytes fetched by a single read() */
#define MAX_EVENTS      5   /* Maximum number of event to be returned from a single epoll_wait() call */

int main(int argc, char *argv){
  int epfd, ready, fd, s, j, numOpenFds;
  struct epoll_event ev;
  struct epoll_event evlist[MAX_EVENTS];
  char buf[MAX_BUF];

  if (argc < 2 || strcmp(argv[1], "--help") == 0){
    printf("Usage: %s file\n", argv[0]);
    return -1;
  }

  // 1. Create an epoll instance
  epfd = epoll_create1(0);
  if(epfd == -1){
    printf("Error: epoll_create() failed\n");
    return -1;
  }

  /* Open each file on command line, and add it to the "interest list" for the epoll instance */

  for(j=1; j < argc; j++){
    fd = open(argv[j], O_RDONLY);
    if (fd == -1)
      perror("open");
    printf("Opened \"%s\" on fd %d\n", argv[j], fd);

    ev.events = EPOLLIN;   /* Only interested in input events */
    ev.data.fd = fd;

    // Add the resulting file descriptor to the interest list of the epoll instance
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) == -1){
      perror("epoll_ctl");
    }
  }

  numOpenFds = argc - 1;

  // Execute a loop that calls epoll_wait() to monitor the insterest list of the epoll instance and
  while (numOpenFds > 0){
    /* Fetch up to MAX_EVENTS items from the ready list */
    printf("About to epoll_wait()\n");
    ready = epoll_wait(epfd, evlist, MAX_EVENTS, -1); // number of ready FDs
    if(ready == -1){
      if (errno == EINTR)
	continue;        /* Restart if interrupted by signal */
      else{
	perror("epoll_wait");
	return -1;
      }
    }

    printf("Ready: %d\n", ready);

    // Deal with returned list of events
    for (j = 0; j < ready; j++){
      printf(" fd=%d; events: %s%s%s\n", evlist[j].data.fd,
	     (evlist[j].events & EPOLLIN)  ? "EPOLLIN "  : "",
	     (evlist[j].events & EPOLLHUP) ? "EPOLLHUP " : "",
	     (evlist[j].events & EPOLLERR) ? "EPOLLERR " : "");

      if (evlist[j].events & EPOLLIN) {
	s = read(evlist[j].data.fd, buf, MAX_BUF);
	if (s = -1){
	  perror("read");
	  return -1;
	}
	printf("    read %d bytes: %.*s\n", s, s, buf);
      }
      else if (evlist[j].events & (EPOLLHUP | EPOLLERR)) {
	/* If EPOLLIN and EPOLLHUP were both set, then there might
	   be more than MAX_BUF bytes to read. Therefore, we close
	   the file descriptor only if EPOLLIN was not set.
	   We'll read further bytes after the next epoll_wait(). */
	printf(" closing fd %d\n", evlist[j].data.fd);
	if (close(evlist[j].data.fd) == -1){
	  perror("close");
	  return -1;
	}
	numOpenFds--;
      }
    }
  }

  printf("All file descriptors closed, bye!\n");
  return 0;
}
