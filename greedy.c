#include "greedy.h"

#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>



size_t g_reconstruct_counter = 0;



// return the new ds size
size_t _make_minimal(Graph* g, size_t current_ds_size)
{
    assert(g != NULL);
    for(size_t i_vertices = 0; i_vertices < g->n; i_vertices++) {
        Vertex* v = g->vertices[i_vertices];
        if(v->is_in_ds && v->dominated_by_number > 1) {
            bool v_redundant = true;
            for(uint32_t i_v = 0; i_v < v->degree; i_v++) {
                if(v->neighbors[i_v]->dominated_by_number < 2) {
                    assert(v->neighbors[i_v]->dominated_by_number >= 1); // otherwise ds would not be a dominating set
                    v_redundant = false;
                    break;
                }
            }
            if(v_redundant) {
                v->is_in_ds = false;
                current_ds_size--;
                v->dominated_by_number--;
                for(uint32_t i_v = 0; i_v < v->degree; i_v++) {
                    v->neighbors[i_v]->dominated_by_number--;
                }
            }
        }
    }
    return current_ds_size;
}



// comparison function for the priority queue
bool uint32_t_greater(const uint32_t a, const uint32_t b)
{
    return a > b;
}



size_t greedy(Graph* g)
{
    uint32_t undominated_vertices = 0; // the total number of undominated vertices remaining in the graph
    PQueue* pq = pq_new(uint32_t_greater);
    if(!pq) {
        perror("greedy: pq_new failed");
        exit(EXIT_FAILURE);
    }
    for(uint32_t vertices_idx = 0; vertices_idx < g->n; vertices_idx++) {
        Vertex* v = g->vertices[vertices_idx];
        assert(!v->is_in_pq);               // this is the same data as v->is_removed
        uint32_t undominated_neighbors = 0; // including itself if v is undominated
        if(v->dominated_by_number == 0) {
            undominated_vertices++;
            undominated_neighbors++;
        }
        for(uint32_t i = 0; i < v->degree; i++) {
            if(v->neighbors[i]->dominated_by_number == 0) {
                undominated_neighbors++;
            }
        }
        pq_insert(pq, (KeyValPair) {.key = undominated_neighbors, .val = v});
    }

    size_t current_ds_size = 0;
    while(undominated_vertices > 0) {
        assert(!pq_is_empty(pq));
        KeyValPair kv = pq_pop(pq);
        Vertex* v = kv.val;
        // uint32_t undominated_neighbors = kv.key; // not needed so far
        assert(!v->is_in_ds);
        v->is_in_ds = true;
        current_ds_size++;
        uint32_t v_is_newly_dominated = 0;
        v->dominated_by_number++;
        if(v->dominated_by_number == 1) {
            v_is_newly_dominated = 1;
            undominated_vertices--;
        }

        for(uint32_t i_v = 0; i_v < v->degree; i_v++) {
            Vertex* u1 = v->neighbors[i_v];
            u1->dominated_by_number++;
            uint32_t delta_undominated_neighbors_u1 = v_is_newly_dominated;
            if(u1->dominated_by_number == 1) {    // if v is the first one to dominate u1
                delta_undominated_neighbors_u1++; // u1 lost itself as an undominated neighbor
                undominated_vertices--;
                for(uint32_t i_u1 = 0; i_u1 < u1->degree; i_u1++) {
                    Vertex* u2 = u1->neighbors[i_u1];
                    // because u1 is now dominated, u2 has one undominated neighbor fewer than before
                    if(u2->is_in_pq) {
                        pq_decrease_priority(pq, u2, pq_get_key(pq, u2) - 1);
                    }
                }
            }
            // u1 lost 0, 1, or 2 undominated neighbors, depending on if u1 and v were undominated before
            if(u1->is_in_pq && delta_undominated_neighbors_u1 > 0) {
                pq_decrease_priority(pq, u1, pq_get_key(pq, u1) - delta_undominated_neighbors_u1);
            }
        }
    }
    pq_free(pq);
    current_ds_size = _make_minimal(g, current_ds_size);
    return current_ds_size;
}



// returns a random double x, with +0.0 <= x <= 1.0, uniform distribution
static double _random(void)
{
    return rand() / (double)RAND_MAX; // TODO: use a better rng. RAND_MAX is only guaranteed to be at least 32767 and rand() is also not that fast.
}



// returns the resulting ds size
static size_t _remove_randomly_from_ds(Graph* g, double removal_probability, size_t current_ds_size)
{
    for(size_t i_vertices = 0; i_vertices < g->n; i_vertices++) {
        Vertex* v = g->vertices[i_vertices];
        if(v->is_in_ds) {
            if((_random() < removal_probability)) {
                v->dominated_by_number--;
                for(uint32_t i_v = 0; i_v < v->degree; i_v++) {
                    v->neighbors[i_v]->dominated_by_number--;
                }
                v->is_in_ds = false;
                current_ds_size--;
            }
        }
    }
    return current_ds_size;
}



typedef struct Queue {
    struct Queue* next;
    Vertex* val;
} Queue;



// returns the resulting ds size
static size_t _local_deconstruction(Graph* g, const size_t current_ds_size)
{
    for(size_t i_vertices = 0; i_vertices < g->n; i_vertices++) { // inefficient, TODO
        g->vertices[i_vertices]->queued = false;
    }

    size_t start_index = (size_t)(_random() * g->n);
    start_index = start_index >= g->n ? g->n - 1 : start_index;

    Queue* q_head = calloc(1, sizeof(Queue));
    if(!q_head) {
        exit(42);
    }
    q_head->val = g->vertices[start_index];
    Queue* q_tail = q_head;

    size_t count_removed = 0;
    while(q_head != NULL && count_removed < 25) {
        Vertex* v = q_head->val;

        if(v->is_in_ds) {
            v->dominated_by_number--;
            for(uint32_t i_v = 0; i_v < v->degree; i_v++) {
                v->neighbors[i_v]->dominated_by_number--;
            }
            v->is_in_ds = false;
            count_removed++;
        }

        for(uint32_t i_v = 0; i_v < v->degree; i_v++) {
            Vertex* u = v->neighbors[i_v];
            if(!u->queued) {
                u->queued = true;
                Queue* new_q_elem = calloc(1, sizeof(Queue));
                if(!new_q_elem)
                    exit(42);
                new_q_elem->val = u;
                q_tail->next = new_q_elem;
                q_tail = new_q_elem;
            }
        }

        Queue* to_free = q_head;
        q_head = q_head->next;
        free(to_free);
    }

    while(q_head != NULL) {
        Queue* to_free = q_head;
        q_head = q_head->next;
        free(to_free);
    }


    return current_ds_size - count_removed;
}



// ugly, just temporary
// returns the resulting ds size
size_t greedy_remove_and_refill(Graph* g, double removal_probability, size_t current_ds_size)
{
    assert(g != NULL);
    assert(removal_probability > 0.0 && removal_probability < 1.0);
    uint32_t undominated_vertices = 0; // the total number of undominated vertices remaining in the graph

    if(++g_reconstruct_counter % 3 == 0) {
        assert(fprintf(stderr, "local deconstruction \t"));
        current_ds_size = _local_deconstruction(g, current_ds_size);
    }
    else {
        assert(fprintf(stderr, "random deconstruction\t"));
        current_ds_size = _remove_randomly_from_ds(g, removal_probability, current_ds_size);
    }

    PQueue* pq = pq_new(uint32_t_greater);
    if(!pq) {
        perror("greedy: pq_new failed");
        exit(EXIT_FAILURE);
    }
    for(uint32_t vertices_idx = 0; vertices_idx < g->n; vertices_idx++) {
        Vertex* v = g->vertices[vertices_idx];
        uint32_t undominated_neighbors = 0; // including itself if v is undominated
        if(v->dominated_by_number == 0) {
            undominated_vertices++;
            undominated_neighbors++;
        }
        for(uint32_t i = 0; i < v->degree; i++) {
            if(v->neighbors[i]->dominated_by_number == 0) {
                undominated_neighbors++;
            }
        }
        v->is_in_pq = false;
        if(undominated_neighbors > 0) {
            pq_insert(pq, (KeyValPair) {.key = undominated_neighbors, .val = v});
        }
    }


    while(undominated_vertices > 0) {
        assert(!pq_is_empty(pq));
        KeyValPair kv = pq_pop(pq);
        Vertex* v = kv.val;
        // uint32_t undominated_neighbors = kv.key; // not needed so far
        assert(!v->is_in_ds);
        v->is_in_ds = true;
        current_ds_size++;
        uint32_t v_is_newly_dominated = 0;
        v->dominated_by_number++;
        if(v->dominated_by_number == 1) {
            v_is_newly_dominated = 1;
            undominated_vertices--;
        }

        for(uint32_t i_v = 0; i_v < v->degree; i_v++) {
            Vertex* u1 = v->neighbors[i_v];
            u1->dominated_by_number++;
            uint32_t delta_undominated_neighbors_u1 = v_is_newly_dominated;
            if(u1->dominated_by_number == 1) {    // if v is the first one to dominate u1
                delta_undominated_neighbors_u1++; // u1 lost itself as an undominated neighbor
                undominated_vertices--;
                for(uint32_t i_u1 = 0; i_u1 < u1->degree; i_u1++) {
                    Vertex* u2 = u1->neighbors[i_u1];
                    // because u1 is now dominated, u2 has one undominated neighbor fewer than before
                    if(u2->is_in_pq) {
                        pq_decrease_priority(pq, u2, pq_get_key(pq, u2) - 1);
                    }
                }
            }
            // u1 lost 0, 1, or 2 undominated neighbors, depending on if u1 and v were undominated before
            if(u1->is_in_pq && delta_undominated_neighbors_u1 > 0) {
                pq_decrease_priority(pq, u1, pq_get_key(pq, u1) - delta_undominated_neighbors_u1);
            }
        }
    }
    pq_free(pq);
    current_ds_size = _make_minimal(g, current_ds_size);
    return current_ds_size;
}
