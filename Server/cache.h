#ifndef CACHE_H
#define CACHE_H

#define CACHE_CAPACITY 10  


typedef struct CacheEntry {
    char *query;           
    char *result;         
    struct CacheEntry *prev; 
    struct CacheEntry *next; 
} CacheEntry;


typedef struct Cache {
    CacheEntry *head;      
    CacheEntry *tail;     
    int size;             
} Cache;



Cache *createCache();
void addToCache(Cache *cache, const char *query, const char *result);
CacheEntry *findInCache(Cache *cache, const char *query);
void clearCache(Cache *cache);

#endif // CACHE_H
