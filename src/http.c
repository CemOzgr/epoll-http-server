#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/http.h"

int http_request_parse(const char *plain_request, HttpRequest *result) {
    HttpVerb http_verb;
    HttpVersion version = HTTP09;
    char *temp, *line, *line_save, *token, *verb, *resource;

    if (result == NULL) return -1;
    if (plain_request == NULL) return HTTP_EMPTY_REQUEST;

    temp = strdup(plain_request);
    line = strtok_r(temp, "\r\n", &line_save);

    while (line != NULL) {
        verb = strtok_r(line, " ", &token);

        if (verb == NULL || !strcmp(verb, "GET")) {
            http_verb = GET;
        }
        else {
            free(temp);
            return HTTP_INVALID_REQUEST;
        }

        resource = strtok_r(NULL, " ", &token);
        result->resource = strdup(resource);
        
        if (result->resource == NULL) {
            free(temp);
            return -1;
        }

        line = strtok_r(NULL, "\r\n", &line_save);
    }

    free(temp);

    return HTTP_PARSE_SUCCESSFUL;
}

void http_request_destroy(HttpRequest *request) {
    if (request == NULL) return;

    if (request->resource != NULL) {
        free((void*)request->resource);
        request->resource = NULL;
    }
    
    free(request);
}