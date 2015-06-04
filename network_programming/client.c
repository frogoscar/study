// Client

#include<stdio.h> //printf
#include<stdlib.h> // exit()
#include<string.h>    //strlen
#include<sys/socket.h>    //socket
#include<arpa/inet.h> //inet_addr

#define MAXDATASIZE 300

int main(int argc , char *argv[])
{
    int sock, numBytes;
    struct sockaddr_in server;
    char message[1000] , server_reply[2000];
    char buf[MAXDATASIZE];

    //Create socket
    sock = socket(AF_INET , SOCK_STREAM , 0);
    if (sock == -1)
    {
	perror("Could not create socket");
        return 1;
    }
    puts("Socket created");

    //server.sin_addr.s_addr = inet_addr("fleet.kuantic.com");
    server.sin_family = AF_INET;
    server.sin_port = htons(65420);

    //Connect to remote server
    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("connect failed. Error");
        return 1;
    }

    puts("Connected\n");

    // receive data from server test
    if ((numBytes = recv(sock, buf, MAXDATASIZE - 1, 0)) == -1) {
      perror("recv()");
      exit(1);
    }
    else
      printf("Client - recv() is OK...\n");

    buf[numBytes] = '\0';
    printf("Client received: %s\n", buf);

    //keep communicating with server
    while(1)
    {
        printf("Enter message : ");
        scanf("%s" , message);

        //Send some data
        if( send(sock , message , strlen(message) , 0) < 0)
        {
            puts("Send failed");
            return 1;
        }

        //Receive a reply from the server
        if( recv(sock , server_reply , 2000 , 0) < 0)
        {
            puts("recv failed");
            break;
        }

        puts("Server reply :");
        puts(server_reply);
    }

    printf("Client closing socket\n");
    close(sock);
    return 0;
}
