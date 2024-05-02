#include "../include/task_queue.h"
#include "../include/thread_pool.h"
#include "../include/signaling.h"
#include "../include/server.h"
#include "../include/client_table.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#define SERVER_BACKLOG 10
#define SERVER_DATA_LENGTH 1024
#define SERVER_INITIAL_CAPACITY 8

#define EPOLL_INFINITE -1
#define EPOLL_MAX_EVENTS 64

typedef struct Server {
    int epfd;
    int listener_socket;
    Signal *signal;
    ThreadPool* thread_pool;
    TaskQueue* task_queue;
    ClientTable *client_table;
} Server;

typedef struct RequestContext {
    Server *server;
    Client *client;
} RequestContext;

int accept_connection(Server *server, int socket);
int handle_request(void *arg);
void dispatch_request(Server *server, int socket);
void *get_in_adrr(struct sockaddr *adrr);
int get_listener_socket(const char *port);
void register_read_event_listener(int epfd, int fd);

Server *server_initialize(int thread_pool_size) {
    Signal *signal = signal_initialize();
    TaskQueue *queue = task_queue_create(signal);
    ThreadPool *pool = thread_pool_initialize(thread_pool_size, queue, signal);
    ClientTable *table = client_table_initialize(SERVER_DATA_LENGTH, SERVER_INITIAL_CAPACITY, 6);

    if (queue == NULL || pool == NULL) {
        fprintf(stderr, "failed to initialize server\n");
        exit(EXIT_FAILURE);
    }

    Server *server = malloc(sizeof(Server));

    server->signal = signal;
    server->task_queue = queue;
    server->thread_pool = pool;
    server->client_table = table;

    return server;
}

void server_destroy(Server *server) {
    printf("Server: shutting down\n");
    
    task_queue_destroy(server->task_queue);
    thread_pool_destroy(server->thread_pool);
    signal_destroy(server->signal);
    client_table_destroy(server->client_table);

    server->task_queue = NULL;
    server->thread_pool = NULL;

    close(server->listener_socket);
    close(server->epfd);

    free(server);

    printf("Server: shutdown completed\n");
}

void server_run(Server *server, const char *port, int shutdown_fd) {
    int new_socket; 

    socklen_t sin_size;
    char address_string[INET6_ADDRSTRLEN];
    struct sockaddr_storage their_address;

    struct epoll_event events[EPOLL_MAX_EVENTS];

    char client_buffer[SERVER_DATA_LENGTH];

    server->listener_socket = get_listener_socket(port);

    server->epfd = epoll_create1(0);
    register_read_event_listener(server->epfd, shutdown_fd);
    register_read_event_listener(server->epfd, server->listener_socket);

    printf("Server: listening connections\n");

    for (;;) {
        int number_of_events = epoll_wait(server->epfd, events, EPOLL_MAX_EVENTS, EPOLL_INFINITE);

        if (number_of_events == -1) {
            perror("epoll_wait");
            continue;
        }

        for (int i = 0; i<number_of_events; i++) {
            int fd = events[i].data.fd;

            if (fd == server->listener_socket) {
                new_socket = accept_connection(server, fd);
                if (new_socket == -1) continue;

                register_read_event_listener(server->epfd, new_socket);
                client_table_add(server->client_table, new_socket, SERVER_DATA_LENGTH);
            } else if(fd == shutdown_fd){
                close(shutdown_fd);
                server_destroy(server);

                return;
            } else {
                dispatch_request(server, fd);
            }
        }
    }
}

int get_listener_socket(const char *port) {
    int yes = 1;
    int addr_status, socket_fd;
    struct addrinfo hints, *servinfo, *p;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((addr_status = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(addr_status));
        exit(EXIT_FAILURE);
    }

    for (p=servinfo; p != NULL; p = p->ai_next) {
        socket_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

        if (socket_fd == -1) {
            perror("socket");
            continue;
        }

        if(setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1) {
            perror("setsockopt");
            exit(EXIT_FAILURE);
        }

        if (bind(socket_fd, p->ai_addr, p->ai_addrlen) != -1) break;

        perror("bind");
        close(socket_fd);
    }

    freeaddrinfo(servinfo);

    if (p == NULL) {
        close(socket_fd);
        
        fprintf(stderr, "could not bind\n");
        exit(EXIT_FAILURE);
    }

    if (listen(socket_fd, SERVER_BACKLOG) == -1) {
        close(socket_fd);
        
        perror("listen");
        exit(EXIT_FAILURE);
    }

    return socket_fd;
}

int accept_connection(Server *server, int socket) {
    socklen_t sin_size;
    char address_string[INET6_ADDRSTRLEN];
    struct sockaddr_storage their_address;

    sin_size = sizeof their_address;
    int new_socket = accept(socket, (struct sockaddr *)&their_address, &sin_size);

    if (new_socket == -1) {
        perror("accept");
        return -1;
    }

    inet_ntop(their_address.ss_family,
                get_in_adrr((struct sockaddr *)&their_address),
                address_string, sizeof address_string);
    
    printf("Server: recieved conection from: %s\n", address_string);


    return new_socket;
}

void *get_in_adrr(struct sockaddr *adrr) {
    if (adrr->sa_family == AF_INET) {
        return &(((struct sockaddr_in *)adrr)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)adrr)->sin6_addr);
}

int handle_request(void *arg) {
    RequestContext *context = (RequestContext *)arg;

    Server *server = context->server;
    Client *client = context->client;

    int size = sizeof(char) * client->buffer_size;
    size_t bytes_read;

    memset(client->buffer, 0, size);

    bytes_read = recv(client->socket, client->buffer, size-1, 0);
    client->buffer[bytes_read] = '\0';

    if (bytes_read == -1) {
        perror("read");
        return -1;
    } 
    
    if (bytes_read == 0) {
        close(client->socket);

        client_table_remove(server->client_table, client->socket);

        return 0;
    }
    
    register_read_event_listener(server->epfd, client->socket);

    if (send(client->socket, client->buffer, client->buffer_size, 0) == -1) {
        perror("send");
    }

    free(arg);

    return 0;
}

void dispatch_request(Server *server, int socket) {
    Client *client = client_table_get(server->client_table, socket);
    if (client == NULL) return;

    RequestContext *context = malloc(sizeof(RequestContext));
    context->client = client;
    context->server = server;

    Task task;
    task.function = handle_request;
    task.arg = context;

    epoll_ctl(server->epfd, EPOLL_CTL_DEL, socket, NULL);
    task_queue_enqueue(server->task_queue, &task);
}

void register_read_event_listener(int epfd, int fd) {
    epoll_data_t listener_data = {
        .fd = fd
    };
    struct epoll_event read_event = {
        .events = EPOLLIN,
        .data = listener_data
    };

    epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &read_event);
}