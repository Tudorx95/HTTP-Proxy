#include "HTTP.h"
#include "../Server/server.h"
#include "../utils.h"
#include <stdlib.h>

http_method_t identify_HTTP_Method(const char *method)
{
    if (!strcmp(method, "GET"))
        return GET;
    if (!strcmp(method, "POST"))
        return POST;
    if (!strcmp(method, "DELETE"))
        return DELETE;
    if (!strcmp(method, "OPTIONS"))
        return OPTIONS;
    if (!strcmp(method, "HEAD"))
        return HEAD;
    if (!strcmp(method, "PATCH"))
        return PATCH;
    return UNKNOWN_METHOD;
}

void add_URI(char **dest, const char *URI)
{
    *dest = (char *)malloc((strlen(URI) - 1) * sizeof(char));
    strcpy(*dest, URI);
}

http_version_t identify_version(const char *version)
{
    if (!strcmp(version, "HTTP/1.0"))
        return HTTP_1_0;
    if (!strcmp(version, "HTTP/1.1"))
        return HTTP_1_1;
    if (!strstr(version, "HTTP/2"))
        return HTTP_2;
    return HTTP_3;
}

http_header_t *populate_header(char **component, size_t count)
{
    http_header_t *header = malloc(sizeof(http_header_t) * sizeof(count));
    // here may be some problems regarding this token
    for (int i = 0; strcmp(*component, "\n"); *component = strtok(NULL, " \n"), i++)
    {
        *component = strtok(NULL, " ");
        header[i].name = malloc((strlen(*component) - 1) * sizeof(char));
        strcpy(header[i].name, *component);
        *component = strtok(NULL, "\n");
        header[i].value = malloc((strlen(*component) - 1) * sizeof(char));
        strcpy(header[i].value, *component);
    }
    return header;
}

void *identify_body(char **component)
{
    char *buff;
    int size = 0;
    *component = strtok(NULL, "\n");
    for (; strcmp(*component, "\n\0");)
    {
        size += strlen(*component);
        buff = (char *)realloc(buff, size);
        if (!buff)
            strcpy(buff, *component);
        else
            strcat(buff, *component);
    }
    return buff;
}

void *HTTP_constructor(char *message)
{
    int nb_lines = determine_lines(message);
    char *component = strtok(message, " ");
    // verify the type of HTTP message
    if (!strstr(component, "HTTP"))
    {
        // HTTP request
        http_request_t *request = (http_request_t *)malloc(sizeof(http_request_t));
        request->method = identify_HTTP_Method(component);
        component = strtok(NULL, " ");
        add_URI(&request->uri, component);
        component = strtok(NULL, "\n");
        request->version = identify_version(component);
        request->header_count = (size_t)(nb_lines - 1);
        request->headers = populate_header(&component, request->header_count);
        request->body = identify_body(&component);
        return request;
    }
    // else this is a response message
    http_response_t *response = (http_response_t *)malloc(sizeof(http_response_t));
    response->version = identify_version(component);
    component = strtok(NULL, " ");
    response->status_code = atoi(component);
    component = strtok(NULL, " ");
    response->reason_phrase = malloc((strlen(component) - 1) * sizeof(component));
    strcpy(response->reason_phrase, component);
    response->header_count = (size_t)(nb_lines - 1);
    response->headers = populate_header(&component, response->header_count);
    response->body = identify_body(&component);
    return response;
}