#include "cache.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


Cache *createCache() {
    Cache *cache = (Cache *)malloc(sizeof(Cache));
    cache->head = NULL;
    cache->tail = NULL;
    cache->size = 0;
    return cache;
}


CacheEntry *findInCache(Cache *cache, const char *query) {
    CacheEntry *current = cache->head;

    while (current) {
        if (strcmp(current->query, query) == 0) {
            return current; 
        }
        current = current->next;
    }
    return NULL; 
}


void addToCache(Cache *cache, const char *query, const char *result) {
    CacheEntry *existing = findInCache(cache, query);


    if (existing) {

        if (existing->prev) existing->prev->next = existing->next;
        if (existing->next) existing->next->prev = existing->prev;

        if (cache->tail == existing) cache->tail = existing->prev;

        existing->prev = NULL;
        existing->next = cache->head;
        if (cache->head) cache->head->prev = existing;
        cache->head = existing;

        return;
    }


    CacheEntry *newEntry = (CacheEntry *)malloc(sizeof(CacheEntry));
    newEntry->query = strdup(query);
    newEntry->result = strdup(result);
    newEntry->prev = NULL;
    newEntry->next = cache->head;


    if (cache->head) cache->head->prev = newEntry;
    cache->head = newEntry;


    if (!cache->tail) cache->tail = newEntry;

    cache->size++;


    if (cache->size > CACHE_CAPACITY) {
        CacheEntry *toRemove = cache->tail;

        if (toRemove->prev) toRemove->prev->next = NULL;
        cache->tail = toRemove->prev;

        free(toRemove->query);
        free(toRemove->result);
        free(toRemove);
        cache->size--;
    }
}


void clearCache(Cache *cache) {
    CacheEntry *current = cache->head;

    while (current) {
        CacheEntry *toDelete = current;
        current = current->next;

        free(toDelete->query);
        free(toDelete->result);
        free(toDelete);
    }

    cache->head = NULL;
    cache->tail = NULL;
    cache->size = 0;
}
