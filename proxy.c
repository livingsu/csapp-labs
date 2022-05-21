/*
 * name: Liu Zhuo
 * ID:518021910888
 */ 
/*
 * proxy.c - ICS Web proxy
 *
 *
 */

#include "csapp.h"
#include <stdarg.h>
#include <sys/select.h>

/*
 * Annotate DEBUG_PRINT to cancel print debug info
 * If not annotated, different threads' printf will cause a dead lock
 */
// #define DEBUG_PRINT
sem_t mutex;

typedef struct vargp {
    int connfd;
    struct sockaddr_in clientaddr;
} vargp_t;

/*
 * Function prototypes
 */
int parse_uri(char *uri, char *target_addr, char *path, char *port);
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr, char *uri, size_t size);

// custom wrappper for rio
ssize_t Rio_readn_w(int fd, void *usrbuf, size_t n);
ssize_t Rio_readnb_w(rio_t *rp, void *usrbuf, size_t n);
ssize_t Rio_readlineb_w(rio_t *rp, void *usrbuf, size_t maxlen);
int Rio_writen_w(int fd, void *usrbuf, size_t n);

// proxy functions
void doit(int fd, struct sockaddr_in *clientaddr);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);
void read_requesthdrs(rio_t *rp, char *buf);
void *thread(void *vargp);

/*
 * main - Main routine for the proxy program
 */
int main(int argc, char **argv)
{
    /* Check arguments */
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port number>\n", argv[0]);
        exit(0);
    }

    sem_init(&mutex, 0, 1);
    Signal(SIGPIPE, SIG_IGN);

    int listenfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    pthread_t tid;
    vargp_t *args;

    
    listenfd = Open_listenfd(argv[1]);
    while (1) {
        args = Malloc(sizeof(vargp_t));
        clientlen = sizeof(struct sockaddr_in);
        args->connfd = Accept(listenfd, (SA *)&(args->clientaddr), &clientlen);
        Getnameinfo((SA *)&(args->clientaddr), clientlen, hostname, MAXLINE,
                    port, MAXLINE, 0);
#ifdef DEBUG_PRINT
        printf("Accepted connection from (%s %s).\n", hostname, port);
#endif
        Pthread_create(&tid, NULL, thread, args);
    }
    Close(listenfd);

    exit(0);
}

void *thread(void *vargp){
    // detach the thread
    Pthread_detach(Pthread_self());
    vargp_t *args = (vargp_t *)vargp;
    doit(args->connfd, &(args->clientaddr));
    Close(args->connfd);
    Free(args);
    return NULL;
}

ssize_t Rio_readn_w(int fd, void *ptr, size_t nbytes) {
    ssize_t n;

    if ((n = rio_readn(fd, ptr, nbytes)) < 0) {
        fprintf(stderr, "Rio_readn error");
        return 0;
    }
    return n;
}

ssize_t Rio_readnb_w(rio_t *rp, void *usrbuf, size_t n) {
    ssize_t rc;

    if ((rc = rio_readnb(rp, usrbuf, n)) < 0) {
        fprintf(stderr, "Rio_readnb error");
        return 0;
    }
    return rc;
}

ssize_t Rio_readlineb_w(rio_t *rp, void *usrbuf, size_t maxlen) {
    ssize_t rc;

    if ((rc = rio_readlineb(rp, usrbuf, maxlen)) < 0) {
        fprintf(stderr, "Rio_readlineb error");
        return 0;
    }
    return rc;
}

int Rio_writen_w(int fd, void *usrbuf, size_t n) {
    if (rio_writen(fd, usrbuf, n) != n) {
        fprintf(stderr, "Rio_writen error");
        return -1;
    }
    return 0;
}

void doit(int fd, struct sockaddr_in *clientaddr) {
    // client
    rio_t client_rio;
    char buf[MAXLINE];
    char method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    // server
    rio_t server_rio;
    char serverbuf[MAXLINE];
    char hostname[MAXLINE], pathname[MAXLINE], port[MAXLINE];
    int serverfd;
    size_t n = 0, server_size = 0, content_length = 0;

    // read client request line`
    Rio_readinitb(&client_rio, fd);
    if (Rio_readlineb_w(&client_rio, buf, MAXLINE) == 0) {
#ifdef DEBUG_PRINT
        printf("client request empty error\n");
#endif
        return;
    }
    if (sscanf(buf, "%s %s %s", method, uri, version) != 3) {
#ifdef DEBUG_PRINT
        printf("client request HTTP format error\n");
#endif
        return;
    }
#ifdef DEBUG_PRINT
    printf("get uri: %s %s %s\n", method, uri, version);
#endif

    if (parse_uri(uri, hostname, pathname, port) == -1) {
#ifdef DEBUG_PRINT
        printf("parse uri error\n");
#endif
        return;
    }
#ifdef DEBUG_PRINT
    printf("parse uri: %s %s %s\n", hostname, pathname, port);
#endif

    // connected to server
    if ((serverfd = Open_clientfd(hostname, port)) < 0) {
#ifdef DEBUG_PRINT
        printf("connected to server[%s:%s] error\n", hostname, port);
#endif
        return;
    }

    /* send client's request to server */
    Rio_readinitb(&server_rio, serverfd);
    // request header
    sprintf(serverbuf, "%s /%s %s\r\n", method, pathname, version);
    while ((n = Rio_readlineb_w(&client_rio, buf, MAXLINE)) != 0) {
#ifdef DEBUG_PRINT
        // printf("client request header: %s\n", buf);
#endif
        sprintf(serverbuf, "%s%s", serverbuf, buf);
        // read Content-Length
        if (!strncasecmp(buf, "Content-Length:", 15))
            sscanf(buf + 15, "%zu", &content_length);
        if (!strncmp(buf, "\r\n", 2)) break;
    }
    // no header data, return
    if (n == 0) return;
#ifdef DEBUG_PRINT
    printf("request body content-length: %zu\n", content_length);
#endif
    // send request header
    if (Rio_writen_w(serverfd, serverbuf, strlen(serverbuf)) == -1) {
#ifdef DEBUG_PRINT
        printf("send request header to server failed\n");
#endif
        return;
    }
    // request body when method is POST
    if (!strcasecmp(method, "POST")) {
        while (content_length > 0) {
            int len = 1;
            if (Rio_readnb_w(&client_rio, buf, len) == 0) {
#ifdef DEBUG_PRINT
                printf("read request body failed\n");
#endif
                return;
            }
            if (Rio_writen_w(serverfd, buf, len) == -1) {
#ifdef DEBUG_PRINT
                printf("send request body failed\n");
#endif
                return;
            }
            content_length -= len;
        }
    }

    /* send server's response to client */
    // response header
    while ((n = Rio_readlineb(&server_rio, serverbuf, MAXLINE)) != 0) {
        server_size += n;
        if (Rio_writen_w(fd, serverbuf, n) == -1) {
#ifdef DEBUG_PRINT
            printf("send response header failed\n");
#endif
            return;
        }
        // read Content-Length
        if (!strncasecmp(serverbuf, "Content-Length:", 15))
            sscanf(serverbuf + 15, "%zu", &content_length);
#ifdef DEBUG_PRINT
        printf("response header get %zu bytes\n", n);
#endif
        if (!strncmp(serverbuf, "\r\n", 2)) break;
    }
    // no header data, return
    if (n == 0) return;

#ifdef DEBUG_PRINT
    printf("response body content-length: %zu\n", content_length);
#endif
    // response body
    while (content_length > 0) {
        int len = 1;
        if (Rio_readnb_w(&server_rio, buf, len) == 0) {
#ifdef DEBUG_PRINT
            printf("read response body error\n");
#endif
            break;
        }
        if (Rio_writen_w(fd, buf, len) == -1) {
#ifdef DEBUG_PRINT
            printf("send response body error\n");
#endif
            break;
        }
        server_size += len;
        content_length -= len;
    }

    // generate log and send
    format_log_entry(buf, clientaddr, uri, server_size);

    P(&mutex);
    printf("%s\n", buf);
    V(&mutex);
    Close(serverfd);
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg) {
    char buf[MAXLINE], body[MAXBUF];

    /* Build the HTTP response body */
    sprintf(body, "Proxy Error %s %s\n", cause, longmsg);

    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}

// read headers to buf
void read_requesthdrs(rio_t *rp, char *buf) {
    char tmpbuf[MAXLINE];
    while (Rio_readlineb(rp, tmpbuf, MAXLINE) != 0) {
        printf("tmpbuf: %s\n", tmpbuf);
        sprintf(buf, "%s%s", buf, tmpbuf);
        if (!strcmp(tmpbuf, "\r\n")) break;
    }
#ifdef DEBUG_PRINT
    printf("read request header: %s\n", buf);
#endif
    return;
}

/*
 * parse_uri - URI parser
 *
 * Given a URI from an HTTP proxy GET request (i.e., a URL), extract
 * the host name, path name, and port.  The memory for hostname and
 * pathname must already be allocated and should be at least MAXLINE
 * bytes. Return -1 if there are any problems.
 */
int parse_uri(char *uri, char *hostname, char *pathname, char *port)
{
    char *hostbegin;
    char *hostend;
    char *pathbegin;
    int len;

    if (strncasecmp(uri, "http://", 7) != 0) {
        hostname[0] = '\0';
        return -1;
    }

    /* Extract the host name */
    hostbegin = uri + 7;
    hostend = strpbrk(hostbegin, " :/\r\n\0");
    if (hostend == NULL)
        return -1;
    len = hostend - hostbegin;
    strncpy(hostname, hostbegin, len);
    hostname[len] = '\0';

    /* Extract the port number */
    if (*hostend == ':') {
        char *p = hostend + 1;
        while (isdigit(*p))
            *port++ = *p++;
        *port = '\0';
    } else {
        strcpy(port, "80");
    }

    /* Extract the path */
    pathbegin = strchr(hostbegin, '/');
    if (pathbegin == NULL) {
        pathname[0] = '\0';
    }
    else {
        pathbegin++;
        strcpy(pathname, pathbegin);
    }

    return 0;
}

/*
 * format_log_entry - Create a formatted log entry in logstring.
 *
 * The inputs are the socket address of the requesting client
 * (sockaddr), the URI from the request (uri), the number of bytes
 * from the server (size).
 */
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr,
                      char *uri, size_t size)
{
    time_t now;
    char time_str[MAXLINE];
    char host[INET_ADDRSTRLEN];

    /* Get a formatted time string */
    now = time(NULL);
    strftime(time_str, MAXLINE, "%a %d %b %Y %H:%M:%S %Z", localtime(&now));

    if (inet_ntop(AF_INET, &sockaddr->sin_addr, host, sizeof(host)) == NULL)
        unix_error("Convert sockaddr_in to string representation failed\n");

    /* Return the formatted log entry string */
    sprintf(logstring, "%s: %s %s %zu", time_str, host, uri, size);
}
