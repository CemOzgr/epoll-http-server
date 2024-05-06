#ifndef HTTP_H
#define HTTP_H

#define HTTP_PARSE_SUCCESSFUL 0
#define HTTP_EMPTY_REQUEST 1
#define HTTP_INVALID_REQUEST 2;

typedef enum HttpVersion { HTTP09, HTTP11 } HttpVersion;
typedef enum HttpVerb { GET, POST, } HttpVerb;

typedef struct HttpRequest {
    HttpVerb verb;
    HttpVersion version;
    const char *resource;
} HttpRequest;

int http_request_parse(const char *plain_request, HttpRequest *result);
void http_request_destroy(HttpRequest *request);

#endif