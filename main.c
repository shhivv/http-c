#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT 2022

void handle_conn(int connfd) {

  char buffer[1024];
  bzero(buffer, 1024);
  int len = read(connfd, buffer, 1024);

  char* req = strtok(buffer, "\r\n\r\n");
  printf("\e[0;34m%s\n", req);
  
  char html[] = "Hello world";
  char resp[1024];
  snprintf(resp, sizeof(resp), "HTTP/1.1 200 OK\r\nContent-Length:%li\r\n\r\n%s", sizeof(html)-1, html);

  write(connfd, resp, sizeof(resp));
}

int main(void) {

  // AF_INET for the internet domain socket
  // SOCK_STREAM for tcp sockets
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);

  if (sockfd < 0) {
    perror("socket");
    return 0;
  }

  struct sockaddr_in server_addr;

  server_addr.sin_family = AF_INET; // internet sockets
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port =
      htons(PORT); // htons lets you convert from host byte order to network
                   // byte order so big endian

  if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    perror("bind");
    return 1;
  }

  printf("Connected on %i:%i\n", INADDR_ANY, PORT);

  listen(sockfd, 5);

  while (1) {
    int connfd = accept(sockfd, NULL, NULL);
    handle_conn(connfd);
  }

  return 0;
}