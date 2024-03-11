#include "cachelab.h"
#include <bits/types/FILE.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

typedef struct Cache {
    int valid;
    int tag;
    int time;  
} Cache;



int hits = 0, misses = 0, evictions = 0;
int S, E, B, s;
Cache** init_cache(int S, int E) {
    Cache** cache = (Cache**) malloc(s * sizeof(Cache*));
    for (int i = 0; i < s; i++) {
        cache[i] = (Cache*) malloc(E * sizeof(Cache));
        cache[i]->valid = 0;
        cache[i]->time = 0;
        cache[i]->tag = 0;
    }
    
    return cache;
}

void free_cache(Cache** cache) {
    for (int i = 0; i < s; i++) {
        free(cache[i]);
    }
    free(cache);
}

void update_time(Cache** cache) {
    for (int i = 0; i < s; i++) {
        for (int j = 0; j < E; j++) {
            if (cache[i][j].valid == 1) {
                cache[i][j].time++;
            }
        }
    }
}

void update_cache(unsigned long address, Cache** cache) {
    int set_index = (address >> B) & (-1U >> (64 - S));
    int tag = address >> (B + S);
    for (int i = 0; i < E; i++) {
        if (cache[set_index][i].tag == tag && cache[set_index][i].valid == 1) {
            cache[set_index][i].time = 0;
            hits++;
            return;
        }
    } 
    for (int i = 0; i < E; i++) {
        if (cache[set_index][i].valid == 0) {
            cache[set_index][i].tag = tag;
            cache[set_index][i].time = 0;
            cache[set_index][i].valid = 1;
            misses++;
            return;
        }
    }
    misses++;
    evictions++;

    int evict_index = 0;
    int time_max = INT_MIN;
    for (int i = 0; i < E; i++) {
        if (cache[set_index][i].time > time_max) {
            time_max = cache[set_index][i].time;
            evict_index = i;
        }
    }
    cache[set_index][evict_index].time = 0;
    cache[set_index][evict_index].tag = tag;
}

void parse_file(char* file, Cache** cache) {
    FILE* f = fopen(file, "r");
    if (f == NULL) {
        printf("error trace file %s\n", file);
        exit(-1);
    }
    char op;
    unsigned int address;
    int size;
    
    while (fscanf(f, " %c %xu, %d\n", &op, &address, &size) > 0) {
        switch (op) {
            case 'L':
                update_cache(address, cache);
                break;
            case 'M':
                update_cache(address, cache);
            case 'S':
                update_cache(address, cache);        
        }
        update_time(cache);
    }
    fclose(f);
    free_cache(cache);
}

int main(int args, char* argv[])
{
    int verbose = 0;
    
    char* file;
    for (int i = 0; i < args; i++) {
        if (strcmp(argv[i], "-v") == 0) {
            verbose = 1;
        } else if (strcmp(argv[i], "-s") == 0) {
            S = atoi(argv[i + 1]);
            s = 1 << S;
            i++;
        } else if (strcmp(argv[i], "-E") == 0) {
            E = atoi(argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "-b") == 0) {
            B = atoi(argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "-t") == 0) {
            file = argv[i + 1];
            i++;
        }
    }
    Cache** caches = init_cache(S, E);
    parse_file(file, caches);

    printf("%d %d %d %d %s\n", verbose, S, E, B, file);        
    printSummary(hits, misses, evictions);
    return 0;
}
