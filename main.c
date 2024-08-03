#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT 4224

struct http_req {
  char *req_line;
  char *method;
  char *path;
  char **headers;
};

struct http_req parse_req(char buffer[]) {
  struct http_req parsed;

  char *end_of_req = strstr(buffer, "\r\n");
  // request is broken
  if (end_of_req == NULL) {
    parsed.method = parsed.path = parsed.req_line = NULL;
    return parsed;
  }

  int req_size =
      end_of_req -
      buffer; // trying to find the difference in the pointer, because end of
              // req is right after buffer in the stack frame

  parsed.req_line = malloc(req_size + 1);
  strncpy(parsed.req_line, buffer, req_size);
  parsed.req_line[req_size] = '\0';

  char *method = strtok(buffer, " ");

  char *path = strtok(NULL, " ");

  strtok(NULL, "\r\n");

  parsed.headers = malloc(30 * sizeof(char *));

  int c = 0;
  char *header = strtok(NULL, "\r\n");
  while (header != NULL && c < 30) {
    parsed.headers[c] = header;
    c++;
    header = strtok(NULL, "\r\n");
  }

  parsed.method = method;
  parsed.path = path;

  return parsed;
}

void handle_conn(int connfd) {

  char buffer[1024];
  bzero(buffer, 1024);

  int len = read(connfd, buffer, 1024);
  if (len < 0) {
    return;
  }

  struct http_req parsed = parse_req(buffer);
  printf("\e[0;34m%s", parsed.req_line);

  char fpath[256] = "./public";
  FILE *fptr = fopen(strcat(fpath, parsed.path), "r");

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

  int code = 200;
  char resp[1024];
  snprintf(resp, sizeof(resp), "HTTP/1.1 %i OK\r\nContent-Length:%li\r\n\r\n%s",
           code, strlen(parsed.headers[0]), parsed.headers[0]);
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