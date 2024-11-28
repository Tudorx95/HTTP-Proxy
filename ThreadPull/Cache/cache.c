#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../Shared_Mem/utils.h"

CacheManagementUnit *cache = NULL;

CacheManagementUnit *create_cache()
{
    CacheManagementUnit *cmu = (CacheManagementUnit *)malloc(sizeof(CacheManagementUnit));
    cmu->head = NULL;
    cmu->tail = NULL;
    cmu->size = 0;
    pthread_mutex_init(&cmu->lock, NULL); // pentru sincronizarea accesului
    return cmu;
}

CacheNode *create_node(const char *client_id, const char *message)
{
    CacheNode *node = (CacheNode *)malloc(sizeof(CacheNode));
    node->client_id = strdup(client_id);
    node->message = strdup(message);
    node->next = NULL;
    node->prev = NULL;
    return node;
}

void store_message(CacheManagementUnit *cmu, const char *client_id, const char *message)
{

    // blocam mutexul inainte de a modifica cahe-ul, asigurand ca doar un thread are acces la cache
    pthread_mutex_lock(&cmu->lock);

    CacheNode *current = cmu->head;
    while (current)
    {
        if (strcmp(current->client_id, client_id) == 0)
        {
            free(current->message);
            current->message = strdup(message);
            pthread_mutex_unlock(&cmu->lock);
            printf("Updated cache for client_id: %s with new message: %s\n", client_id, message);
            return;
        }
        current = current->next;
    }

    if (cmu->size == MAX_CACHE_SIZE)
    {
        // se elibereaza cel mai vechi nod
        CacheNode *oldest = cmu->head;
        cmu->head = cmu->head->next;
        if (cmu->head)
        {
            cmu->head->prev = NULL;
        }
        free(oldest->client_id);
        free(oldest->message);
        free(oldest);
        cmu->size--;
        printf("Cache size exceeded, removed oldest item from cache\n");
    }

    CacheNode *new_node = create_node(client_id, message);
    if (!cmu->head)
    {
        // atunci cand lista e goala
        cmu->head = new_node;
        cmu->tail = new_node;
    }
    else
    {
        cmu->tail->next = new_node;
        new_node->prev = cmu->tail;
        cmu->tail = new_node;
    }
    cmu->size++;

    // deblocarea mutexului dupa ce s-a terminat modificarea cache-ului
    pthread_mutex_unlock(&cmu->lock);
    printf("Stored new message in cache for client_id: %s: %s\n", client_id, message);
}

// cautarea unui mesaj stocat pentru un anumit client in cache
char *retrieve_message(CacheManagementUnit *cmu, const char *client_id)
{
    pthread_mutex_lock(&cmu->lock);

    CacheNode *current = cmu->head;
    while (current)
    {
        if (strcmp(current->client_id, client_id) == 0)
        {
            char *result = strdup(current->message);
            pthread_mutex_unlock(&cmu->lock);
            printf("Cache hit for client_id: %s, returning message: %s\n", client_id, result);
            return result;
        }
        current = current->next;
    }

    pthread_mutex_unlock(&cmu->lock);
    printf("Cache miss for client_id: %s\n", client_id);
    return NULL;
}

void free_cache(CacheManagementUnit *cmu)
{
    pthread_mutex_lock(&cmu->lock);
    CacheNode *current = cmu->head;
    while (current)
    {
        CacheNode *temp = current;
        current = current->next;
        free(temp->client_id);
        free(temp->message);
        free(temp);
    }

    pthread_mutex_unlock(&cmu->lock);
    pthread_mutex_destroy(&cmu->lock);
    free(cmu);
}

void *cacheConstructor(void *buffer)
{
    char *message = (char *)buffer;
    char *cached_response = retrieve_message(cache, message);
    if (cached_response)
    {
        printf("Cache HIT: Found message in cache for request: %s\n", message);
        printf("Cached response: %s\n", cached_response);
        free(cached_response);
    }
    else
    {
        printf("Cache MISS: Message not found in cache for request: %s\n", message);
        store_message(cache, message, "Simulated response: This is the cached content for GET / HTTP/1.1");
        // printf("Stored new message in cache: %s\n", message);
    }
    return NULL;
}