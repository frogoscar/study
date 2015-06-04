#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 1987

/* how many pending connections queue will hold */
#define BACKLOG 10
