
#include "csapp.h"
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

#define MAX_CACHE (MAX_CACHE_SIZE / MAX_OBJECT_SIZE)

#define MAX_FDS 100
#define MAX_THREADS 10

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

struct Uri {
  char host[MAXLINE]; // hostname
  char port[MAXLINE]; // 端口
  char path[MAXLINE]; // 路径
};

struct ClientFdPool {
  int *fd;
  int n;
  int f, l;
};

struct CacheLine {
  char data[MAX_OBJECT_SIZE];
  char uri[MAXLINE];
  int valid;
  int read_cnt;
  int lru;
  sem_t mutex;
  sem_t w;
};

struct Cache {
  struct CacheLine data[MAX_CACHE];
  int num;
};

struct Cache cache;

void init_cache();
int get_cache(char *tag);
void update_cache();
void write_cache(char *tag, char *data);

sem_t mutex;
sem_t full, empty;

struct ClientFdPool fds;

void init_fds(struct ClientFdPool * fds, int s);
void put_fd(struct ClientFdPool * fds, int fd);
int take_fd(struct ClientFdPool * fds);

void* thread_do(void *argp);
void doproxy(char *port, int fd);
void build_requesthdrs(int fd, char *buf, rio_t *rp, struct Uri *new_uri);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);
void build_uri(char *buf, char *uri, struct Uri *new_uri);   
void build_host_header(char *buff, struct Uri *new_uri);              

int main(int argc, char **argv) {
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;
  listenfd = Open_listenfd(argv[1]);
  
  init_fds(&fds, MAX_FDS);
  init_cache();
  Sem_init(&mutex, 0, 1);
  Sem_init(&full, 0, 0);
  Sem_init(&empty, 0, MAX_FDS);
  pthread_t tid[MAX_THREADS];
  for (int i = 0; i < MAX_THREADS; i++) {
    Pthread_create(&tid[i], NULL, thread_do, argv[1]);
  }
  for (int i = 0; i < MAX_THREADS; i++) {
    Pthread_detach(tid[i]);
  }

  printf("start %d thread\n", MAX_THREADS);
  fflush(stdout);
  while (1) {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr,
                    &clientlen); // line:netp:tiny:accept
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
                0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    fflush(stdout);
    //doproxy(argv[1], connfd); // line:netp:tiny:doit
    //Close(connfd);   // line:netp:tiny:
    put_fd(&fds, connfd);
  }
  return 0;
}

void* thread_do(void *argp) {
  int fd;
  pthread_t tid = pthread_self();
  while (1) {
    fd = take_fd(&fds);
    printf("thread %ld take fd %d\n", tid, fd);
    doproxy((char*)argp, fd);
    Close(fd);
  }
  return NULL;
}

void doproxy(char *port, int fd) {
  char buf[MAXLINE];
  char method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  rio_t rio;
  struct Uri new_uri;
  /* Read request line and headers */
  Rio_readinitb(&rio, fd);
  if (!Rio_readlineb(&rio, buf, MAXLINE)) // line:netp:doit:readrequest
    return;
  printf("%s", buf);
  sscanf(buf, "%s %s %s", method, uri, version); // line:netp:doit:parserequest
  if (strcasecmp(method, "GET")) { // line:netp:doit:beginrequesterr
    clienterror(fd, method, "501", "Not Implemented",
                "Tiny does not implement this method");
    return;
  }
  build_uri(buf, uri, &new_uri);
  build_requesthdrs(fd, buf, &rio, &new_uri);
  if (!strcmp(new_uri.port, port)) {
    strcpy(buf, "server path or port wrong:");
    strcat(buf, new_uri.host);
    strcpy(buf, ":");
    strcpy(buf, new_uri.port);
    clienterror(fd, "Wrong!", "404", "Not found", buf);
    return;
  }
  int cidx;
  if ((cidx = get_cache(uri)) != -1) {
    P(&cache.data[cidx].mutex);
    cache.data[cidx].read_cnt++;
    if (cache.data[cidx].read_cnt == 1) {
      P(&cache.data[cidx].w);
    }
    V(&cache.data[cidx].mutex);
    Rio_writen(fd, cache.data[cidx].data, strlen(cache.data[cidx].data));
    P(&cache.data[cidx].mutex);
    cache.data[cidx].read_cnt--;
    if (cache.data[cidx].read_cnt == 0) {
      V(&cache.data[cidx].w);
    }
    V(&cache.data[cidx].mutex);
    return;
  }
  int sfd = Open_clientfd(new_uri.host, new_uri.port);
  if (sfd < 0) {
    strcpy(buf, "server path or port wrong:");
    strcat(buf, new_uri.host);
    strcpy(buf, ":");
    strcpy(buf, new_uri.port);
    clienterror(fd, "Wrong!", "404", "Not found", buf);
    return;
  }
  rio_t server_rio;
  
  Rio_readinitb(&server_rio, sfd);
  Rio_writen(sfd, buf, strlen(buf));
  char data_cache[MAX_OBJECT_SIZE];
  size_t t = 0;
  size_t n;
  while ((n = Rio_readlineb(&server_rio, buf, MAXLINE)) > 0) {
    //printf("proxy received %d bytes,then send\n", (int)n);
    t += n;
    if (t < MAX_OBJECT_SIZE) {
      strncat(data_cache, buf, n);
    }
    Rio_writen(fd, buf, n);
  }
  
  printf("proxy received %ld bytes total\n", t);
  Close(sfd);
  if (t < MAX_OBJECT_SIZE) {
    write_cache(uri, data_cache);
  }
  // rio_t server_rio;
  // Rio_readinitb(&server_rio, sfd);

  // Rio_writen(int fd, void *usrbuf, size_t n)
}

// http://localhost:8080/hello
// /hello
void build_uri(char *buf, char *uri, struct Uri *new_uri) {
  int host = 0;
  if (strstr(uri, "http")) {
    uri += 7;
    host = 1;
  }
  char *port_ptr = index(uri, ':');
  char *ptr = index(uri, '/');
  if (!host) {
    strcpy(new_uri->port, "80");
    strcpy(new_uri->path, uri);
  } else if (port_ptr && ptr) {
    strncpy(new_uri->host, uri, port_ptr - uri);
    strncpy(new_uri->port, port_ptr + 1, ptr - port_ptr - 1);
    strcpy(new_uri->path, ptr);
  } else if (port_ptr && !ptr) {
    ptr = uri + strlen(uri);
    strncpy(new_uri->host, uri, port_ptr - uri);
    strncpy(new_uri->port, port_ptr + 1, ptr - port_ptr - 1);
    strcpy(new_uri->path, "/");
  } else if (!port_ptr && ptr) {
    strncpy(new_uri->host, uri, ptr - uri);
    strcpy(new_uri->port, "80");
    strcpy(new_uri->path, ptr);
  } else {
    strcpy(new_uri->host, uri);
    strcpy(new_uri->port, "80");
    strcpy(new_uri->path, "/");
  }
  strcpy(buf, "GET ");
  strcat(buf, new_uri->path);
  strcat(buf, " HTTP/1.0\r\n");
}

/*
 * read_requesthdrs - read HTTP request headers
 */
/* $begin read_requesthdrs */
void build_requesthdrs(int fd, char *buf, rio_t *rp, struct Uri *new_uri) {
  char buff[MAXLINE];
  do {
    Rio_readlineb(rp, buff, MAXLINE);
    printf("rec header %s", buff);
    if (strstr(buff, "Host")) {
      build_host_header(buff, new_uri);
    } else if (strstr(buff, "User-Agent")
    || strstr(buff, "Connection")
    || strstr(buff, "Proxy-Connection")) {
    } else if (strcmp(buff, "\r\n")) {
      strcat(buf, buff);
    }
  } while (strcmp(buff, "\r\n"));
  strcat(buf, user_agent_hdr);
  strcat(buf, "Connection: close\r\n");
  strcat(buf, "Proxy-Connection: close\r\n");
  if (strlen(new_uri->host) == 0) {
    clienterror(fd, "host not found", "404", "Not Found", "Wrong uri");
    return;
  } else {
    strcat(buf, "Host: ");
    strcat(buf, new_uri->host);
    strcat(buf, "\r\n");
  }
  strcat(buf, "\r\n");
  printf("after\n %s", buf);
  return;
}

void build_host_header(char *buff, struct Uri *new_uri) {
  char *ptr = index(buff, ':');
    if (*(ptr + 1) == ' ') {
      ptr += 2;
    } else {
      ptr += 1;
    }
    char *end = index(buff, '\r');
    if (strstr(ptr, ":")) {
      char *p = index(ptr, ':');
      p += 1;
      strncpy(new_uri->host, ptr, p - ptr - 1);
      strncpy(new_uri->port, p, end - p);
    } else {
      strncpy(new_uri->host, ptr, end - ptr);
    }
}

/*
 * clienterror - returns an error message to the client
 */
/* $begin clienterror */
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg) {
  char buf[MAXLINE];

  /* Print the HTTP response headers */
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n\r\n");
  Rio_writen(fd, buf, strlen(buf));

  /* Print the HTTP response body */
  sprintf(buf, "<html><title>Tiny Error</title>");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "<body bgcolor="
               "ffffff"
               ">\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "%s: %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "<p>%s: %s\r\n", longmsg, cause);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "<hr><em>The Tiny Web server</em>\r\n");
  Rio_writen(fd, buf, strlen(buf));
}

void init_fds(struct ClientFdPool * fds, int s) {
  fds->n = s;
  fds->f = 0;
  fds->l = 0;
  fds->fd = (int*)Malloc(s * sizeof(int));
}

void put_fd(struct ClientFdPool * fds, int fd) {
  P(&empty);
  P(&mutex);
  fds->fd[fds->f] = fd;
  fds->f = (fds->f + 1) % fds->n;
  V(&mutex);
  V(&full);
}

int take_fd(struct ClientFdPool * fds) {
  P(&full);
  P(&mutex);
  int fd = fds->fd[fds->l];
  fds->l = (fds->l + 1) % fds->n;
  V(&mutex);
  V(&empty);
  return fd;
}

void init_cache() {
  cache.num = MAX_CACHE;
  for (int i = 0; i < cache.num; i++) {
    cache.data[i].lru = 1;
    cache.data[i].valid = 0;
    cache.data[i].read_cnt = 0;
    Sem_init(&cache.data[i].mutex, 0, 1);
    Sem_init(&cache.data[i].w, 0, 1);
  }
}

int get_cache(char *tag) {
  update_cache();
  for (int i = 0; i < cache.num; i++) {
    if (cache.data[i].valid && strcmp(cache.data[i].uri, tag) == 0) {
      cache.data[i].lru--;
      return i;
    }
  }
  return -1;
}

void update_cache() {
  for (int i = 0; i < cache.num; i++) {
    if (cache.data[i].valid) {
      cache.data[i].lru++;
    }
  }
}

void write_cache(char *tag, char *data) {
  update_cache();
  int idx = -1;
  for (int i = 0; i < cache.num; i++) {
    if (!cache.data[i].valid) {
      idx = i;
    }
  }
  if (idx == -1) {
    int max = -1;
    for (int i = 0; i < cache.num; i++) {
      if (cache.data[i].lru > max) {
        max  = cache.data[i].lru;
        idx = i;
      }
    }
  }

  P(&cache.data[idx].mutex);
  P(&cache.data[idx].w);
  strcpy(cache.data[idx].uri, tag);
  strcpy(cache.data[idx].data, data);
  cache.data[idx].valid = 1;
  cache.data[idx].lru--;
  V(&cache.data[idx].w);
  V(&cache.data[idx].mutex);
}