#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <threads.h>
#include <unistd.h>

#define PORT 4241

struct http_req {
  char *req_line;
  char *method;
  char *path;
  char **headers;
  char *version;
  char *body;
};

struct http_resp {
  int code;
  char *c_msg; // status line msg
  char **headers;
  char *body;
  int b_size;
  int h_count;
};

char *form_resp(struct http_resp resp, char *buf) {

  snprintf(buf, 1024, "HTTP/1.1 %i %s\r\n", resp.code, resp.c_msg);
  int i = 0;
  while (i < resp.h_count) {
    strcat(buf, resp.headers[i]);
    strcat(buf, "\r\n");
    i++;
  }

  char b_size_header[80];
  snprintf(b_size_header, 80, "Content-Length:%d", resp.b_size);

  strcat(buf, b_size_header);
  strcat(buf, "\r\n");
  strcat(buf, "\r\n");
  strcat(buf, resp.body);
}

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

  char *version = strtok(NULL, "\r\n");

  parsed.headers = malloc(30 * sizeof(char *));

  int c = 0;
  char *header = strtok(NULL, "\r\n");
  while (header != NULL && c < 30) { // if the number of headers is more than
                                     // 29, the body is lost which is bad
    parsed.headers[c] = header;
    c++;
    header = strtok(NULL, "\r\n");
  }

  parsed.body = parsed.headers[c - 1];
  parsed.headers[c - 1] = NULL;

  parsed.method = method;
  parsed.path = path;
  return parsed;
}

int handle_conn(void *arg) {

  int *connfd = arg;

  char buffer[1024];
  bzero(buffer, 1024);

  int len = read(*connfd, buffer, 1024);
  if (len < 0) {
    return 0;
  }

  struct http_req parsed = parse_req(buffer);
  printf("\e[0;34m%s", parsed.req_line);

  char fpath[256] = "./public";
  char *abc = strcat(fpath, parsed.path); // sometimes seg fault here.
  FILE *fptr = fopen(abc, "r");

  if (fptr != NULL && strcmp(parsed.path, "/") != 0) {
    fseek(fptr, 0L, SEEK_END);
    int fsize = ftell(fptr);
    fseek(fptr, 0L, SEEK_SET);

    char *buffer = (char *)calloc(fsize, sizeof(char));

    fread(buffer, sizeof(char), fsize, fptr);
    int code = 200;
    char resp[1024];

    struct http_resp c;
    c.code = code;
    c.c_msg = "OK";
    char *headers[] = {"hello:bye"};

    c.headers = headers;
    c.h_count = 1;

    c.body = buffer;
    c.b_size = fsize;
    form_resp(c, resp);

    printf("\e[0;32m - %i\n", code);

    write(*connfd, resp, strlen(resp) - 1);
    close(*connfd);
    fclose(fptr);
    free(buffer);
    return 0;
  }

  int code = 404;
  char resp[1024];

  char msg[] = "Resource Not Found";
  struct http_resp c;
  c.code = code;
  c.c_msg = "Not Found";
  char *headers[] = {"hello:bye"};

  c.headers = headers;
  c.h_count = 1;

  c.body = msg;
  c.b_size = strlen(msg);
  form_resp(c, resp);

  printf("\e[0;31m - %i\n", code);
  write(*connfd, resp, sizeof(resp));
  close(*connfd);
  return 0;
}

int main(void) {

  // AF_INET for the internet domain socket
  // SOCK_STREAM for tcp sockets
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);

  if (sockfd < 0) {
    perror("socket");
    return 0;
  }

  int reuse = 1;
  // prevents that annoying error of port being used
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(&reuse));
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

  listen(sockfd, 10);

  while (1) {
    int connfd = accept(sockfd, NULL, NULL);
    thrd_t t;
    thrd_create(&t, handle_conn, &connfd);
  }

  return 0;
}