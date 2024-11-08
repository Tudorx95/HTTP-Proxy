#ifndef CACHE_UTILS_H
#define CACHE_UTILS_H

#include <pthread.h>

#define CLIENT_LEN_MESS 5000
#define MAX_CACHE_SIZE 100

typedef struct CacheNode {
    char *client_id;
    char *message;
    struct CacheNode *next;
    struct CacheNode *prev;
} CacheNode;

typedef struct { 
    CacheNode *head;        //inceputul si sfarsitul listei dublu inlantuite 
    CacheNode *tail;  
    int size;               //nr curent de elemente 
    pthread_mutex_t lock;   //accesul sigur din mai multe thread-uri
} CacheManagementUnit;

extern CacheManagementUnit *cache;

CacheManagementUnit* create_cache();
void store_message(CacheManagementUnit *cmu, const char *client_id, const char *message);
char* retrieve_message(CacheManagementUnit *cmu, const char *client_id);
void free_cache(CacheManagementUnit *cmu);

#endif