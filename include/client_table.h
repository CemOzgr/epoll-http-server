#ifndef CLIENT_TABLE_H
#define CLIENT_TABLE_H

typedef struct Client {
    int socket;
    int buffer_size;
    char buffer[];
} Client;

typedef struct ClientTable {
    int max_index;
    int size;
    int offset;
    Client **clients;
} ClientTable;

ClientTable *client_table_initialize(int buffer_size, int table_size, int offset);
void client_table_destroy(ClientTable *table);
Client *client_table_get(ClientTable *table, int socket);
void client_table_add(ClientTable * table, int socket, int buffer_size);
void client_table_remove(ClientTable *table, int socket);

#endif