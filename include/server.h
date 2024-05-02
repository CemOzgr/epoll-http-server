#ifndef SERVER_H
#define SERVER_H

typedef struct Server Server;

Server *server_initialize(int thread_pool_size);
void server_destroy(Server *server);
void server_run(Server *server, const char *port, int shutdown_fd);

#endif