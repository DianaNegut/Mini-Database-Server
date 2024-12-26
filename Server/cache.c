#include "cache.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Cache *createCache()
{
    Cache *cache = (Cache *)malloc(sizeof(Cache));
    cache->head = NULL;
    cache->tail = NULL;
    cache->size = 0;
    return cache;
}

CacheEntry *findInCache(Cache *cache, const char *query)
{
    CacheEntry *current = cache->head;

    while (current)
    {
        if (strcmp(current->query, query) == 0)
        {
            printf("------------------------------------------------------------------------------\n");
            printf("Intrare in cache gasita!\n");
            return current;
        }
        current = current->next;
    }
    printf("------------------------------------------------------------------------------\n");
    printf("INTARARE LIPSA IN CACHE!\n");
    return NULL;
}

void addToCache(Cache *cache, const char *query, const char *result)
{
    CacheEntry *existing = findInCache(cache, query);

    // Dacă intrarea există deja, mut-o la început (LRU)
    if (existing)
    {
        if (existing->prev)
            existing->prev->next = existing->next;
        if (existing->next)
            existing->next->prev = existing->prev;

        if (cache->tail == existing)
            cache->tail = existing->prev;

        existing->prev = NULL;
        existing->next = cache->head;
        if (cache->head)
            cache->head->prev = existing;
        cache->head = existing;

        return;
    }

    // Creează o nouă intrare
    CacheEntry *newEntry = (CacheEntry *)malloc(sizeof(CacheEntry));
    newEntry->query = strdup(query);   // Copiază interogarea
    newEntry->result = strdup(result); // Copiază rezultatul
    newEntry->prev = NULL;
    newEntry->next = cache->head;

    if (cache->head)
        cache->head->prev = newEntry;
    cache->head = newEntry;

    if (!cache->tail)
        cache->tail = newEntry;

    cache->size++;

    // Dacă depășim capacitatea, eliminăm ultimul element (LRU)
    if (cache->size > CACHE_CAPACITY)
    {
        CacheEntry *toRemove = cache->tail;

        if (toRemove->prev)
            toRemove->prev->next = NULL;
        cache->tail = toRemove->prev;

        free(toRemove->query);
        free(toRemove->result);
        free(toRemove);

        cache->size--;
    }
}

void clearCache(Cache *cache)
{
    CacheEntry *current = cache->head;

    while (current)
    {
        CacheEntry *toDelete = current;
        current = current->next;

        free(toDelete->query);

        for (int i = 0; toDelete->result[i] != NULL; i++)
        {
            free(toDelete->result[i]);
        }
        free(toDelete->result);
        free(toDelete);
    }

    cache->head = NULL;
    cache->tail = NULL;
    cache->size = 0;
}
