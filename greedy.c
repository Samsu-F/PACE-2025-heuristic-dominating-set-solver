#include "greedy.h"

#include <stdbool.h>



// comparison function for the priority queue
bool size_t_greater(const size_t a, const size_t b)
{
    return a > b;
}



VertexArray greedy(Graph* g)
{
    size_t undominated_vertices = 0; // the total number of undominated vertices remaining in the graph
    PQueue* pq = pq_new(size_t_greater);
    if(!pq) {
        perror("greedy: pq_new failed");
        exit(1);
    }
    for(Vertex* v = g->vertices; v != NULL; v = v->list_next) {
        assert(v->dominated_by_number == 0);
        assert(v->status == DOMINATED || v->status == UNDOMINATED);
        size_t undominated_neighbors = 0; // including itself if v is undominated
        if(v->status == UNDOMINATED) {
            undominated_vertices++;
            undominated_neighbors++;
        }
        for(size_t i = 0; i < v->degree; i++) {
            if(v->neighbors[i]->status == UNDOMINATED) {
                undominated_neighbors++;
            }
        }
        pq_insert(pq, (KeyValPair) {.key = undominated_neighbors, .val = v});
    }

    VertexArray ds;
    ds.size = 0;
    ds.allocated_size = undominated_vertices;
    ds.arr = malloc(undominated_vertices * sizeof(Vertex*));
    if(!ds.arr) {
        perror("greedy: malloc failed");
        exit(1);
    }


    while(undominated_vertices > 0) {
        assert(!pq_is_empty(pq));
        KeyValPair kv = pq_pop(pq);
        Vertex* v = kv.val;
        // size_t undominated_neighbors = kv.key; // not needed so far
        assert(ds.size < ds.allocated_size);
        ds.arr[ds.size++] = v;
        size_t v_is_newly_dominated = 0;
        v->dominated_by_number++;
        if(v->dominated_by_number == 1 && v->status == UNDOMINATED) {
            v_is_newly_dominated = 1;
            undominated_vertices--;
        }

        for(size_t i_v = 0; i_v < v->degree; i_v++) {
            Vertex* u1 = v->neighbors[i_v];
            u1->dominated_by_number++;
            size_t delta_undominated_neighbors_u1 = v_is_newly_dominated;
            if(u1->status == UNDOMINATED && u1->dominated_by_number == 1) { // if v is the first one to dominate u1
                delta_undominated_neighbors_u1++; // u1 lost itself as an undominated neighbor
                undominated_vertices--;
                for(size_t i_u1 = 0; i_u1 < u1->degree; i_u1++) {
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
