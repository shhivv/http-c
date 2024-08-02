#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT 2019

void handle_conn(int connfd) {

  char buffer[1024];
  bzero(buffer, 1024);

  int len = read(connfd, buffer, 1024);
  if (len < 0) {
    return;
  }

  char *req = strtok(buffer, "\r\n");
  printf("\e[0;34m%s", req);
  fflush(stdout);

  strtok(req, " ");
  char *path = strtok(NULL, " ");

  if (strcmp(path, "/") == 0) {
    char html[] = "Hello";
    int code = 200;
    char resp[1024];
    snprintf(resp, sizeof(resp),
             "HTTP/1.1 %i OK\r\nContent-Length:%li\r\n\r\n%s", code,
             sizeof(html) - 1, html);
    printf("\e[0;32m - %i\n", code);

    write(connfd, resp, sizeof(resp));
  }

  char fpath[256] = "./public";
  FILE *fptr = fopen(strcat(fpath, path), "r");

  if (fptr != NULL) {
    fseek(fptr, 0L, SEEK_END);
    int fsize = ftell(fptr);
    fseek(fptr, 0L, SEEK_SET);

    char *buffer = (char *)calloc(fsize, sizeof(char));

    fread(buffer, sizeof(char), fsize, fptr);
    int code = 200;
    char resp[1024];

    snprintf(resp, sizeof(resp),
             "HTTP/1.1 %i OK\r\nContent-Length:%i\r\n\r\n%s", code, fsize,
             buffer);
    printf("\e[0;32m - %i\n", code);

    write(connfd, resp, sizeof(resp));
    free(buffer);
    return;
  }

  char html[] = "Not found";
  int code = 404;
  char resp[1024];
  snprintf(resp, sizeof(resp),
           "HTTP/1.1 %i Not Found\r\nContent-Length:%li\r\n\r\n%s", code,
           sizeof(html) - 1, html);
  printf("\e[0;31m - %i\n", code);
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