#include "greedy.h"

#include <stdbool.h>



// comparison function for the priority queue
bool uint32_t_greater(const uint32_t a, const uint32_t b)
{
    return a > b;
}



DynamicArray greedy(Graph* g)
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

    DynamicArray ds;
    if(!da_init(&ds, 64)) {
        perror("greedy: da_init failed");
        exit(EXIT_FAILURE);
    }


    while(undominated_vertices > 0) {
        assert(!pq_is_empty(pq));
        KeyValPair kv = pq_pop(pq);
        Vertex* v = kv.val;
        // uint32_t undominated_neighbors = kv.key; // not needed so far
        da_add(&ds, v);
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
    return ds;
}
