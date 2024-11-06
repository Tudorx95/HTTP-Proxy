#ifndef HTTP_H
#define HTTP_H

#include <stdint.h>
#include <stddef.h>
#include <openssl/ssl.h>

// Enum for HTTP methods
typedef enum
{
    GET,
    POST,
    PUT,
    DELETE,
    OPTIONS,
    HEAD,
    PATCH,
    UNKNOWN_METHOD
} http_method_t;

// Enum for HTTP versions
typedef enum
{
    HTTP_1_0,
    HTTP_1_1,
    HTTP_2,
    HTTP_3,
    UNKNOWN_VERSION
} http_version_t;

typedef struct
{
    char *name;
    char *value;
} http_header_t;

typedef struct
{
    http_method_t method;
    char *uri;
    http_version_t version;
    http_header_t *headers;
    size_t header_count;
    char *body;
} http_request_t;

// HTTP Response struct
typedef struct
{
    http_version_t version;
    int status_code;
    char *reason_phrase;
    http_header_t *headers;
    size_t header_count;
    char *body;

} http_response_t;

// methods for defining the HTTP structure
void *HTTP_constructor(char *message);
http_method_t identify_HTTP_Method(const char *method);
void add_URI(char **dest, const char *URI);
http_version_t identify_version(const char *version);
http_header_t *populate_header(char **component, size_t count);
void *identify_body(char **component);

#endif