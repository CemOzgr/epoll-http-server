#include "../include/server.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define THREAD_POOL_SIZE 3

int pipefd[2];

void handle_sigint(int s) {
    write(pipefd[1], "x", 1);
}

int main(int argc, char *argv[]) {
    struct sigaction sa;

    if (argc != 2) {
        fprintf(stderr, "Port is not specified\n");
        exit(EXIT_FAILURE);
    }

    Server *server = server_initialize(THREAD_POOL_SIZE);

    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    server_run(server, argv[1], pipefd[0]);

    return 0;
}