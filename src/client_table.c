#include <stdlib.h>
#include <string.h>

#include "../include/client_table.h"

ClientTable *client_table_initialize(int buffer_size, int table_size, int offset) {
    ClientTable *table = malloc(sizeof(ClientTable) + sizeof(Client *) * table_size);
    if (!table) return NULL;
    
    table->size = table_size;
    table->offset = offset;
    table->clients = (Client **) (table + 1);

    for (int i=0; i<table_size; i++) {
        table->clients[i] = NULL;
    }

    return table;
}

void client_table_destroy(ClientTable *table) {
    for (int i=0; i<table->size; i++) {
        if (table->clients[i] == NULL) continue;

        free(table->clients[i]);
        table->clients[i] = NULL;
    }

    free(table);

    table->clients = NULL;
    table = NULL;
}

Client *client_table_get(ClientTable *table, int socket) {
    int index = socket - table->offset;

    return table->clients[index];
}

void client_table_add(ClientTable * table, int socket, int buffer_size) {
    int index = socket - table->offset;

    if (table->clients == NULL || table->clients[index] != NULL) return;

    Client *client = malloc(sizeof(Client) + sizeof(char) * buffer_size);
    if (client == NULL) exit(EXIT_FAILURE);

    memset(client->buffer, 0, buffer_size);

    client->buffer_size = buffer_size;
    client->socket = socket;

    table->clients[index] = client;
    if (socket > table->max_index) table->max_index = index;
}

void client_table_remove(ClientTable *table, int socket) {
    int index = socket - table->offset;

    if (table->clients == NULL || table->clients[index] == NULL) return;

    free(table->clients[index]);
    table->clients[index] = NULL;

    if (index != table->max_index) return;

    int max = 0;
    for (int i=table->max_index-1; i>=0; i--) {
        if (table->clients[i] != NULL) {
            max = i;
            break;
        }
    }

    table->max_index = max;
}