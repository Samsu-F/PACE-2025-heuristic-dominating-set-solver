#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <string.h>

#include "graph.h"
#include "reduction.h"
#include "greedy.h"



static void _print_solution(Graph* g, size_t ds_size)
{
    assert(g != NULL && g->vertices != NULL && g->fixed.vertices != NULL);
    printf("%zu\n", g->fixed.size + ds_size);
    for(size_t fixed_idx = 0; fixed_idx < g->fixed.size; fixed_idx++) {
        Vertex* v = g->fixed.vertices[fixed_idx];
        printf("%" PRIu32 "\n", v->id);
    }
#ifndef NDEBUG
    size_t ds_vertices_found_in_g = 0; // this variable is just for an assertion
#endif
    for(size_t i_vertices = 0; i_vertices < g->n; i_vertices++) {
        Vertex* v = g->vertices[i_vertices];
        if(v->is_in_ds) {
            printf("%" PRIu32 "\n", v->id);
#ifndef NDEBUG
            ds_vertices_found_in_g++;
#endif
        }
    }
    fflush(stdout);
    assert(ds_vertices_found_in_g == ds_size);
}



int main(void)
{
    srand((unsigned)time(NULL));

    Graph* g = graph_parse(stdin);
    if(!g) {
        exit(EXIT_FAILURE);
    }

    reduce(g, 15.0f, 10.0f);
    size_t ds_size = iterated_greedy_solver(g);
    _print_solution(g, ds_size);
    graph_free(g);
}
