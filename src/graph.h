#ifndef _GRAPH_H
#define _GRAPH_H

#include <stdbool.h>
#include <stddef.h>
#include <inttypes.h>
#include <stdio.h>

#include "dynamic_array.h"



// fields are ordered by their expected size, to minimize padding as much as possible
typedef struct Vertex {
    struct Vertex** neighbors; // array of pointers to the neighbors
    double vote;
    uint32_t id;     // the name of the vertex. Must be unique and must not be 0.
    uint32_t degree; // this is the length of the array neighbors
    uint32_t dominated_by_number; // the number of neighbors in the ds this vertex is dominated by. For use by the greedy solver.
    uint32_t queued; // used by local deconstruction to check if a vertex has been queued in the current BFS run yet
    union {
        uint32_t neighbor_tag; /*   For the reduction algorithm to be used as a temporary marker.
                                    This value must never be the id of an existing but non-neighboring vertex.
                                    0 is a valid value, because vertex ids must not be 0. */
        uint32_t pq_kv_idx;    /*   May only be accessed by the internals of the priority queue.
                                    The index this vertex has inside the priority queue. */
    };
    union {
        bool is_removed; // for use during the reduction phase
        bool is_in_pq; // May be read by anyone but must not be changed by anything but the priority queue internals
    };
    bool is_in_ds; // saves if this vertex has been chosen for the domination set in the current solution
} Vertex;


typedef struct Graph {
    Vertex** vertices;  // array of vertices in the graph
    DynamicArray fixed; // list of vertices that are known to be optimal choices for any dominating set.
    uint32_t n;         // number of vertices remaining
    uint32_t m;         // number of edges remaining
    // fixed vertices that were removed from the graph do not count towards n and m
} Graph;



// free graph, all of its vertices and their internals
// g must not be NULL
void graph_free(Graph* g);



// caller is responsible for freeing using graph_free(...)
Graph* graph_parse(FILE* file);



// debug function
// graph_name is optional and can be NULL
// dominated vertices will green, fixed verticed will be cyan, and removed vertices will be red
void graph_print_as_dot(Graph* g, bool include_fixed, const char* graph_name);



#endif
