#include "pqueue.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>

#include "assert_allow_float_equal.h"



#define PQ_INIT_SIZE        64 // start with enough space for x key value pairs
#define PQ_REALLOC_FACTOR   2  // multiply the size by x if more space is needed
#define PQ_REALLOC_DIVISOR  2  // divide the size by x if space is no longer needed
#define PQ_DEALLOCATE_LIMIT 4  // dealloc space if less than 1/x of the allocated space is needed



struct PQueue {
    KeyValPair* nodes;
    size_t n;
    size_t allocated_n;
    PQKeyCompareFunc keycmp;
};



// must not be called on the root node
static inline size_t _pq_parent(size_t index)
{
    return (index - 1) / 2;
}

static inline size_t _pq_lchild(size_t index)
{
    return 2 * index + 1;
}

static inline size_t _pq_rchild(size_t index)
{
    return 2 * index + 2;
}



static void _pq_incr_allocated_n(PQueue* q)
{
    size_t factor = PQ_REALLOC_FACTOR;
    size_t new_n = factor * q->allocated_n;
    size_t new_size_bytes = new_n * sizeof(KeyValPair);
    KeyValPair* new_ptr = realloc(q->nodes, new_size_bytes);
    if(!new_ptr) {
        fprintf(stderr, "pq: Reallocating %lu bytes to increase the capacity to %lu key value pairs failed.\n",
                (unsigned long)new_size_bytes, (unsigned long)new_n);
        // careful if you decide to not exit here: q->nodes is still allocated
        exit(EXIT_FAILURE);
    }
    q->nodes = new_ptr;
    q->allocated_n = new_n;
}



static void _pq_decr_allocated_n(PQueue* q)
{
    if(q->allocated_n <= 64) {
        return; // don't even bother
    }
    size_t divisor = PQ_REALLOC_DIVISOR;
    size_t new_n = q->allocated_n / divisor;
    size_t new_size_bytes = new_n * sizeof(KeyValPair);
    KeyValPair* new_ptr = realloc(q->nodes, new_size_bytes);
    if(!new_ptr) { // I don't expect a realloc to a smaller size to ever fail but it is allowed
        return;
    }
    q->nodes = new_ptr;
    q->allocated_n = new_n;
}



static inline void _pq_swap(PQueue* const q, const size_t node_a, const size_t node_b)
{
    assert(node_a < q->n && node_b < q->n);
    KeyValPair tmp = q->nodes[node_a];
    q->nodes[node_a] = q->nodes[node_b];
    q->nodes[node_b] = tmp;
    q->nodes[node_a].val->pq_kv_idx = (uint32_t)node_a; // update the indices in the vertex structs
    q->nodes[node_b].val->pq_kv_idx = (uint32_t)node_b;
}



static void _pq_heapify_node(PQueue* q, size_t node)
{
    const size_t lchild = _pq_lchild(node);
    const size_t rchild = _pq_rchild(node);
    if(lchild >= q->n) { // if node is a leaf
        return;
    }
    else if(rchild >= q->n) { // if only the left child exists
        if(q->keycmp(q->nodes[lchild].key, q->nodes[node].key)) {
            _pq_swap(q, node, lchild);
            _pq_heapify_node(q, lchild);
        }
        return;
    }
    // at this point, both child nodes exist
    else if(q->keycmp(q->nodes[lchild].key, q->nodes[rchild].key)) { // if lchild's key has greater priority than rchild's key
        if(q->keycmp(q->nodes[lchild].key, q->nodes[node].key)) {
            _pq_swap(q, node, lchild);
            _pq_heapify_node(q, lchild);
            return;
        }
    }
    else { // if rchild's key has greater priority than lchild's key
        if(q->keycmp(q->nodes[rchild].key, q->nodes[node].key)) {
            _pq_swap(q, node, rchild);
            _pq_heapify_node(q, rchild);
            return;
        }
    }
}



PQueue* pq_new(PQKeyCompareFunc compare)
{
    PQueue* q = malloc(sizeof(PQueue));
    if(!q) {
        return NULL;
    }
    q->keycmp = compare;
    q->nodes = malloc(PQ_INIT_SIZE * sizeof(KeyValPair));
    if(!q->nodes) {
        free(q);
        return NULL;
    }
    q->allocated_n = PQ_INIT_SIZE;
    q->n = 0;
    return q;
}



// Free any internal pointers belonging to the PQueue struct and q itself.
// This does not free pointers that are still saved in the queue.
void pq_free(PQueue* q)
{
    free(q->nodes);
    q->nodes = NULL;
    free(q);
}



bool pq_is_empty(const PQueue* q)
{
    return q->n == 0;
}



void pq_insert(PQueue* q, const KeyValPair new)
{
    assert(q != NULL);
    assert(!(new.val->is_in_pq));
    new.val->is_in_pq = true;
    if(q->n == q->allocated_n) {
        _pq_incr_allocated_n(q);
    }
    size_t idx_new = q->n;
    q->n++;
    q->nodes[idx_new] = new;
    q->nodes[idx_new].val->pq_kv_idx = (uint32_t)idx_new; // set the index saved in the vertex struct
    while(idx_new != 0 && q->keycmp(new.key, q->nodes[_pq_parent(idx_new)].key)) {
        size_t idx_parent = _pq_parent(idx_new);
        _pq_swap(q, idx_new, idx_parent);
        idx_new = idx_parent;
    }
}



// must not be called on an empty PQueue
KeyValPair pq_peek(const PQueue* q)
{
    assert(q->n > 0);
    return q->nodes[0];
}



// must not be called on an empty PQueue
KeyValPair pq_pop(PQueue* q)
{
    assert(q->n > 0);
    KeyValPair result = q->nodes[0];
    q->n--;
    if(q->n != 0) {
        q->nodes[0] = q->nodes[q->n];
        q->nodes[0].val->pq_kv_idx = 0;
        _pq_heapify_node(q, 0);
    }
    if(q->n < q->allocated_n / PQ_DEALLOCATE_LIMIT) {
        _pq_decr_allocated_n(q);
    }
    result.val->is_in_pq = false;
    return result;
}



// get the current priority of a vertex
pq_keytype pq_get_key(const PQueue* q, const Vertex* v)
{
    assert(q != NULL && v != NULL);
    assert(v->is_in_pq);
    assert(v->pq_kv_idx < q->n);
    return q->nodes[v->pq_kv_idx].key;
}



// v must be contained in q
// May ONLY be used to decrease the priority of a vertex, i.e. make it come out later than it would without changing.
void pq_decrease_priority(PQueue* q, Vertex* v, const pq_keytype new_key)
{
    assert(q != NULL);
    assert(v != NULL);
    assert(v->is_in_pq);
#ifndef NDEBUG
    pq_keytype old_key = q->nodes[v->pq_kv_idx].key; // this variable is only used for asserts
    assert(!(q->keycmp(new_key, old_key)));
    assert(q->keycmp(old_key, new_key));
#endif

    const size_t idx = v->pq_kv_idx;
    assert(idx < q->n);
    assert(q->nodes[idx].val == v);
    q->nodes[idx].key = new_key;
    _pq_heapify_node(q, idx);

    assert_allow_float_equal(q->nodes[v->pq_kv_idx].key == new_key); // does not prove that v->pq_kv_idx is set correctly, but it is definitely not correct if this fails
}
