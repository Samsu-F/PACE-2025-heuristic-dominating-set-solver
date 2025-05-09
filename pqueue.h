#ifndef _PQUEUE_H
#define _PQUEUE_H

#include <stdbool.h>
#include <stdint.h>

#include "graph.h"


// Array based max heap implementation of a priority queue



#define PQ_KEY_TYPE uint32_t



typedef PQ_KEY_TYPE pq_keytype;


typedef struct KeyValPair {
    pq_keytype key;
    struct Vertex* val;
} KeyValPair;


typedef bool (*PQKeyCompareFunc)(const pq_keytype, const pq_keytype);

typedef struct PQueue PQueue;


// compare is a function to compare keys.
// compare(key1, key2) == true iff key1 has greater priority than key2, i.e. it will be popped sooner.
// pq_new may return NULL if not sucessfull. The returned value has to be freed using
// pq_free(...) if it is not NULL.
PQueue* pq_new(PQKeyCompareFunc compare);

// Free any internal pointers belonging to the PQueue struct and q itself.
// This does not free pointers that are still saved in the queue.
void pq_free(PQueue* q);

// Returns true iff q is empty.
bool pq_is_empty(const PQueue* q);

// inserts KeyValPair new into q
// q does NOT take ownership over any pointer in the key value pair
void pq_insert(PQueue* q, const KeyValPair new);

// Get the KeyValPair with the greatest priority without removing it from q.
// Must not be called on an empty PQueue
KeyValPair pq_peek(const PQueue* q);

// Get the KeyValPair with the greatest priority and remove it from q.
// Must not be called on an empty PQueue
KeyValPair pq_pop(PQueue* q);

// get the current priority of a vertex
pq_keytype pq_get_key(const PQueue* q, const struct Vertex* v);

// May ONLY be used to decrease the priority of a vertex, i.e. make it come out later than it would without changing.
void pq_decrease_priority(PQueue* q, struct Vertex* v, const pq_keytype new_key);



#endif
