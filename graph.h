#ifndef _GRAPH_H
#define _GRAPH_H

#include <stdbool.h>
#include <stddef.h>
#include <inttypes.h>
#include <stdio.h>



typedef enum { UNDOMINATED, DOMINATED, REMOVED } Status;


typedef struct Vertex {
    size_t id; // the name of the vertex. Must be unique and must not be 0.
    struct Vertex* list_prev;
    struct Vertex* list_next;
    size_t degree;             // this is the length of the array neighbors
    struct Vertex** neighbors; // array of pointers to the neighbors
    Status status;             // default is UNDOMINATED
    size_t neighbor_tag; /* For the reduction algorithm to be used as a temporary marker.
                            This value must never be the id of an existing but non-neighboring vertex.
                            0 is a valid value, because vertex ids must not be 0. */
} Vertex;


typedef struct {
    size_t n; // number of vertices remaining
    size_t m; // number of edges remaining
    // fixed vertices that were removed from the graph do not count towards n and m
    Vertex* vertices; // list of vertices in the graph
    Vertex* fixed; // list of vertices that are known to be optimal choices for any dominating set
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
